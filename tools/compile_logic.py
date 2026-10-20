#!/usr/bin/env python3
"""Compilador da lógica do randomizer: JSON (DSL) -> tabelas C para o núcleo portátil.

Entrada:  logic/lighthouse_raw.json (gerado por import_lighthouse.py)
          logic/overrides.json      (política e correções revisadas por humanos)
Saída:    core/generated/bkr_logic_data.h
          core/generated/bkr_logic_data.c
          logic/compiled_report.json

Uso:
    python3 tools/compile_logic.py --raw logic/lighthouse_raw.json \
        --overrides logic/overrides.json --out core/generated --report logic/compiled_report.json
"""
import argparse
import copy
import json
import re
import sys
from pathlib import Path

# Opcodes: DEVEM ser idênticos a core/include/bkr/bkr_rules.h (tools/tests verifica).
OPCODES = {
    "END": 0, "TRUE": 1, "FALSE": 2, "HAS": 3, "EVENT": 4, "REGION": 5,
    "NOTE_DOOR": 6, "WORLD_COST": 7, "TRANSFORM_COST": 8, "TOKENS": 9,
    "JIGGIES": 10, "NOTES": 11, "AND": 12, "OR": 13, "NOT": 14,
}
MAX_DEPTH = 32  # BKR_RULE_MAX_DEPTH: a pilha do avaliador C é um único bkr_u32

LOC_KINDS = {"JIGGY": 1, "HONEYCOMB": 2, "TOKEN": 3, "MOLEHILL": 4}
ITEMS = {"JIGGY": 1, "HONEYCOMB": 2, "TOKEN": 3}
ITEM_MOVE_BASE = 0x20
FILE_WORLDS = {
    "MumbosMountain.cpp": "MM", "TreasureTroveCove.cpp": "TTC", "ClankersCavern.cpp": "CC",
    "BubblegloopSwamp.cpp": "BGS", "FreezeezyPeak.cpp": "FP", "GruntildasLair.cpp": "LAIR",
    "GobisValley.cpp": "GV", "ClickClockWood.cpp": "CCW", "RustyBucketBay.cpp": "RBB",
    "MadMonsterMansion.cpp": "MMM", "SpiralMountain.cpp": "SM",
}
TYPE_LABELS = {"JIGGY": "Jiggy", "HONEYCOMB": "Honeycomb", "TOKEN": "Token", "MOLEHILL": "Molehill"}
WORLD_ACRONYMS = {"MM", "TTC", "CC", "BGS", "FP", "GV", "MMM", "RBB", "CCW", "SM", "GL"}
DSL_TOKEN_RE = re.compile(r"\s*([A-Za-z_][A-Za-z0-9_]*|\d+|\(|\))")


class LogicError(Exception):
    """Erro de compilação da lógica."""


# ----------------------------------------------------------------------------- DSL

def tokenize(text):
    """Quebra uma regra da DSL em tokens; falha em caracteres desconhecidos."""
    tokens = []
    pos = 0
    text = text.strip()
    while pos < len(text):
        match = DSL_TOKEN_RE.match(text, pos)
        if not match or match.end() == pos:
            raise LogicError(f"caractere inválido na posição {pos} de {text!r}")
        tokens.append(match.group(1))
        pos = match.end()
        while pos < len(text) and text[pos].isspace():
            pos += 1
    return tokens


class Parser:
    """Analisador descendente: or > and > not > átomo."""

    def __init__(self, text):
        self.text = text
        self.tokens = tokenize(text)
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def take(self, expected=None):
        tok = self.peek()
        if tok is None or (expected is not None and tok != expected):
            raise LogicError(f"esperado {expected!r}, achado {tok!r} em {self.text!r}")
        self.pos += 1
        return tok

    def parse(self):
        node = self.parse_or()
        if self.peek() is not None:
            raise LogicError(f"tokens sobrando em {self.text!r}: {self.tokens[self.pos:]}")
        return node

    def parse_or(self):
        items = [self.parse_and()]
        while self.peek() == "or":
            self.take("or")
            items.append(self.parse_and())
        return items[0] if len(items) == 1 else ("or", items)

    def parse_and(self):
        items = [self.parse_not()]
        while self.peek() == "and":
            self.take("and")
            items.append(self.parse_not())
        return items[0] if len(items) == 1 else ("and", items)

    def parse_not(self):
        if self.peek() == "not":
            self.take("not")
            return ("not", self.parse_not())
        return self.parse_atom()

    def parse_atom(self):
        tok = self.take()
        if tok == "(":
            node = self.parse_or()
            self.take(")")
            return node
        if tok in ("true", "false"):
            return (tok,)
        if self.peek() == "(":
            self.take("(")
            arg = self.take()
            self.take(")")
            return ("call", tok, arg)
        return ("ident", tok)


class Lowerer:
    """Expande helpers e converte chamadas da DSL em primitivas numéricas."""

    def __init__(self, ctx):
        self.ctx = ctx

    def lower(self, node, depth=0):
        if depth > 40:
            raise LogicError("expansão de helpers recursiva demais")
        kind = node[0]
        if kind in ("true", "false"):
            return node
        if kind in ("and", "or"):
            return (kind, [self.lower(n, depth + 1) for n in node[1]])
        if kind == "not":
            return ("not", self.lower(node[1], depth + 1))
        if kind == "ident":
            return self.expand_helper(node[1], depth)
        if kind == "call":
            return self.lower_call(node[1], node[2], depth)
        raise LogicError(f"nó desconhecido {node}")

    def expand_helper(self, key, depth):
        helpers = self.ctx["helpers"]
        if key not in helpers:
            raise LogicError(f"helper desconhecido: {key}")
        return self.lower(Parser(helpers[key]).parse(), depth + 1)

    def lower_call(self, name, arg, depth):
        ctx = self.ctx
        if name == "has":
            if arg not in ctx["moves"]:
                raise LogicError(f"movimento desconhecido: {arg}")
            return ("op", "HAS", ctx["moves"][arg])
        if name == "event":
            ctx["referenced_events"].add(arg)
            return ("event_ref", arg)
        if name == "region":
            return ("region_ref", arg)
        if name == "note_door":
            cost = int(arg)
            if cost not in ctx["note_doors"]:
                raise LogicError(f"porta de notas desconhecida: {cost}")
            index = ctx["note_doors"].index(cost)
            ctx["referenced_events"].add(f"NOTE_DOOR_{cost}")
            return ("and", [("op", "NOTE_DOOR", index), ("event_ref", f"NOTE_DOOR_{cost}")])
        if name == "world":
            slot = ctx["puzzle_slots"][arg]
            event = ctx["puzzle_events"][arg]
            ctx["referenced_events"].add(event)
            return ("and", [("op", "WORLD_COST", slot), ("event_ref", event)])
        if name == "transform":
            info = ctx["transformations"][arg]
            skulls = [("region_ref", r) for r in info["skulls"]]
            reach = skulls[0] if len(skulls) == 1 else ("or", skulls)
            return ("and", [("op", "TRANSFORM_COST", info["id"]), reach])
        if name == "jinjos":
            events = sorted(e for e in ctx["jinjo_events"] if e.startswith(f"JINJO_{arg}_"))
            if len(events) != 5:
                raise LogicError(f"esperados 5 Jinjos em {arg}, achados {len(events)}")
            for e in events:
                ctx["referenced_events"].add(e)
            return ("and", [("event_ref", e) for e in events])
        if name in ("break", "kill"):
            return self.expand_helper(f"{name}({arg})", depth)
        if name in ("tokens", "jiggies", "notes"):
            return ("op", name.upper(), int(arg))
        raise LogicError(f"função desconhecida na DSL: {name}({arg})")


def simplify(node):
    """Dobra constantes e achata and/or aninhados."""
    kind = node[0]
    if kind == "not":
        inner = simplify(node[1])
        if inner[0] == "true":
            return ("false",)
        if inner[0] == "false":
            return ("true",)
        return ("not", inner)
    if kind in ("and", "or"):
        absorbing = "false" if kind == "and" else "true"
        neutral = "true" if kind == "and" else "false"
        items = []
        for child in (simplify(n) for n in node[1]):
            if child[0] == absorbing:
                return (absorbing,)
            if child[0] == neutral:
                continue
            if child[0] == kind:
                items.extend(child[1])
            else:
                items.append(child)
        unique = []
        for item in items:
            if item not in unique:
                unique.append(item)
        if not unique:
            return (neutral,)
        return unique[0] if len(unique) == 1 else (kind, unique)
    return node


def emit_rpn(node, event_index, region_index):
    """Gera RPN (lista de u16) e a profundidade máxima de pilha."""
    out = []

    def walk(n):
        kind = n[0]
        if kind == "true":
            out.append(OPCODES["TRUE"])
            return 1
        if kind == "false":
            out.append(OPCODES["FALSE"])
            return 1
        if kind == "op":
            out.extend([OPCODES[n[1]], n[2]])
            return 1
        if kind == "event_ref":
            if n[1] not in event_index:
                raise LogicError(f"evento não definido: {n[1]}")
            out.extend([OPCODES["EVENT"], event_index[n[1]]])
            return 1
        if kind == "region_ref":
            if n[1] not in region_index:
                raise LogicError(f"região não definida: {n[1]}")
            out.extend([OPCODES["REGION"], region_index[n[1]]])
            return 1
        if kind == "not":
            depth = walk(n[1])
            out.append(OPCODES["NOT"])
            return depth
        if kind in ("and", "or"):
            depth = walk(n[1][0])
            for i, child in enumerate(n[1][1:]):
                depth = max(depth, i + 1 + walk(child))
                out.append(OPCODES["AND" if kind == "and" else "OR"])
            return depth
        raise LogicError(f"nó não emitível: {n}")

    depth = walk(node)
    out.append(OPCODES["END"])
    if depth > MAX_DEPTH:
        raise LogicError(f"regra excede a pilha de {MAX_DEPTH}: {node}")
    return out, depth


# ----------------------------------------------------------------------- modelo

def display_name(rc_id, world, loc_type):
    """RC_MM_JIGGY_TICKERS_TOWER -> 'MM Jiggy - Tickers Tower' (o covil usa o prefixo GL)."""
    world_label = "GL" if world == "LAIR" else world
    body = rc_id[3:]
    prefix = f"{world_label}_"
    if body.startswith(prefix):
        body = body[len(prefix):]
    type_tokens = {"JIGGY": "JIGGY_", "HONEYCOMB": "EMPTY_HONEYCOMB_", "TOKEN": "MUMBO_TOKEN_",
                   "MOLEHILL": "MOLEHILL_"}[loc_type]
    if body.startswith(type_tokens):
        body = body[len(type_tokens):]
    words = " ".join(w if w in WORLD_ACRONYMS else w.capitalize() for w in body.split("_"))
    return f"{world_label} {TYPE_LABELS[loc_type]} - {words}"


def build_model(raw, ov):
    """Aplica a política do overrides.json sobre o JSON bruto (sem alterar ``raw``)."""
    raw = copy.deepcopy(raw)
    checks = raw["checks"]
    regions = raw["regions"]
    renames = {k: v for k, v in ov["duplicate_check_renames"].items() if not k.startswith("_")}
    rule_patches = {k: v for k, v in ov["check_rule_patches"].items() if not k.startswith("_")}
    excluded = set(ov["excluded_checks"])
    kinds = set(ov["location_types"])
    token_over = {k: v for k, v in ov["token_uid_overrides"].items() if not k.startswith("_")}
    mole_index = {k: v for k, v in ov["molehill_index"].items() if not k.startswith("_")}

    seen = set()
    for region in regions:
        fixed = []
        for check_id, rule in region["checks"]:
            if check_id in seen and check_id in renames:
                check_id = renames[check_id]
            if check_id in seen:
                raise LogicError(f"checagem repetida sem correção: {check_id}")
            seen.add(check_id)
            fixed.append([check_id, rule_patches.get(check_id, rule)])
        region["checks"] = fixed
        region["world"] = FILE_WORLDS[region["file"]]
    for region in regions:
        for check_id, _ in region["checks"]:
            if check_id not in checks and check_id in renames.values():
                original = next(k for k, v in renames.items() if v == check_id)
                checks[check_id] = dict(checks[original])

    region_ids = [r["id"] for r in regions]
    region_index = {rid: i for i, rid in enumerate(region_ids)}
    for extra in ov["extra_events"]:
        if extra["region"] not in region_index:
            raise LogicError(f"extra_events cita região inexistente: {extra['region']}")
        regions[region_index[extra["region"]]]["events"].append([extra["event"], extra["rule"]])
    for extra in ov.get("extra_exits", []):
        if extra["from"] not in region_index or extra["to"] not in region_index:
            raise LogicError(f"extra_exits cita região inexistente: {extra}")
        regions[region_index[extra["from"]]]["exits"].append([extra["to"], extra["rule"]])

    locations, jinjo_events, note_groups, ignored = [], [], [], {}
    for r_i, region in enumerate(regions):
        groups = {}
        for check_id, rule in region["checks"]:
            check = checks[check_id]
            ctype = check["type"]
            if ctype == "JINJO":
                name = "JINJO_" + check_id[3:].replace("_JINJO_", "_", 1)
                jinjo_events.append((r_i, name, rule))
            elif ctype == "NOTE":
                groups.setdefault(rule, {"count": 0, "world": check["world"]})
                groups[rule]["count"] += 1
            elif ctype in kinds and check_id not in excluded:
                locations.append(make_location(check_id, check, r_i, rule, token_over, mole_index, ov))
            else:
                ignored[ctype] = ignored.get(ctype, 0) + 1
        for rule, info in groups.items():
            note_groups.append((r_i, rule, info["count"], info["world"]))
    return {
        "regions": regions, "region_index": region_index, "locations": locations,
        "jinjo_events": jinjo_events, "note_groups": note_groups, "ignored": ignored,
    }


def make_location(check_id, check, region_i, rule, token_over, mole_index, ov):
    """Cria uma localização com id de jogo e item vanilla."""
    ctype = check["type"]
    if ctype == "TOKEN":
        game_id = token_over.get(check_id, check["collection_number"])
    elif ctype == "MOLEHILL":
        game_id = mole_index[check_id]
    else:
        game_id = check["collection_number"]
    if game_id is None:
        raise LogicError(f"sem id de jogo para {check_id} ({check['collection']})")
    if ctype == "MOLEHILL":
        vanilla_item = ITEM_MOVE_BASE + ov["moves"][collection_move(check["collection"])]
    else:
        vanilla_item = ITEMS[ctype]
    return {
        "id": check_id, "kind": ctype, "world": check["world"], "region": region_i,
        "rule": rule, "game_id": game_id, "vanilla_item": vanilla_item,
        "name": display_name(check_id, check["world"], ctype),
    }


def collection_move(collection):
    """ABILITY_10_TALON_TROT -> TALON_TROT (nomes da DSL)."""
    table = {
        "ABILITY_0_BARGE": "BEAK_BARGE", "ABILITY_1_BEAK_BOMB": "BEAK_BOMB",
        "ABILITY_2_BEAK_BUSTER": "BEAK_BUSTER", "ABILITY_3_CAMERA_CONTROL": "CAMERA_CONTROL",
        "ABILITY_4_CLAW_SWIPE": "CLAW_SWIPE", "ABILITY_5_CLIMB": "CLIMB", "ABILITY_6_EGGS": "EGGS",
        "ABILITY_7_FEATHERY_FLAP": "FEATHERY_FLAP", "ABILITY_8_FLAP_FLIP": "FLAP_FLIP",
        "ABILITY_9_FLIGHT": "FLIGHT", "ABILITY_A_HOLD_A_JUMP_HIGHER": "HIGH_JUMP",
        "ABILITY_B_RATATAT_RAP": "RATATAT_RAP", "ABILITY_C_ROLL": "ROLL",
        "ABILITY_D_SHOCK_JUMP": "SHOCK_JUMP", "ABILITY_E_WADING_BOOTS": "WADING_BOOTS",
        "ABILITY_F_DIVE": "DIVE", "ABILITY_10_TALON_TROT": "TALON_TROT",
        "ABILITY_11_TURBO_TALON": "TURBO_TALON", "ABILITY_12_WONDERWING": "WONDERWING",
    }
    if collection not in table:
        raise LogicError(f"molehill com habilidade desconhecida: {collection}")
    return table[collection]


def validate(model, ov):
    """Invariantes do conjunto de localizações (aborta em qualquer desvio)."""
    by_kind = {}
    for loc in model["locations"]:
        by_kind.setdefault(loc["kind"], []).append(loc["game_id"])
    expected = {"JIGGY": set(range(1, 101)), "HONEYCOMB": set(range(1, 25)),
                "TOKEN": set(range(1, 117)), "MOLEHILL": set(range(0, 9))}
    for kind, ids in expected.items():
        got = by_kind.get(kind, [])
        if sorted(got) != sorted(ids):
            missing = sorted(ids - set(got))
            dups = sorted({x for x in got if got.count(x) > 1})
            raise LogicError(f"{kind}: faltando {missing[:10]} duplicados {dups[:10]}")
    notes = {}
    for _, _, count, world in model["note_groups"]:
        notes[world] = notes.get(world, 0) + count
    for world in ("MM", "TTC", "CC", "BGS", "FP", "GV", "MMM", "RBB", "CCW"):
        if notes.get(world) != 100:
            raise LogicError(f"{world} deveria ter 100 notas, tem {notes.get(world)}")
    if len(model["jinjo_events"]) != 45:
        raise LogicError(f"esperados 45 Jinjos, achados {len(model['jinjo_events'])}")
    if ov["start_region"] not in model["region_index"]:
        raise LogicError("start_region inexistente")


# --------------------------------------------------------------------- emissão C

def c_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def fnv1a(values):
    h = 0x811C9DC5
    for v in values:
        for shift in (0, 8):
            h ^= (v >> shift) & 0xFF
            h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def compile_all(raw, ov):
    """Executa o pipeline completo e devolve (header, source, report)."""
    model = build_model(raw, ov)
    validate(model, ov)
    regions = model["regions"]
    region_index = model["region_index"]

    events = set()
    for region in regions:
        for name, _ in region["events"]:
            events.add(name)
    for _, name, _ in model["jinjo_events"]:
        events.add(name)
    event_names = sorted(events)
    event_index = {name: i for i, name in enumerate(event_names)}
    if ov["goal_event"] not in event_index:
        raise LogicError("goal_event não definido")

    ctx = {
        "helpers": {k: v for k, v in ov["helpers"].items() if not k.startswith("_")},
        "moves": ov["moves"], "note_doors": ov["note_doors"],
        "puzzle_slots": {k: v for k, v in ov["puzzle_slots"].items() if not k.startswith("_")},
        "puzzle_events": ov["puzzle_events"], "transformations": ov["transformations"],
        "jinjo_events": [name for _, name, _ in model["jinjo_events"]],
        "referenced_events": set(),
    }
    lowerer = Lowerer(ctx)
    code = []
    cache = {}
    max_depth = [0]

    def rule_offset(text):
        node = simplify(lowerer.lower(Parser(text).parse()))
        rpn, depth = emit_rpn(node, event_index, region_index)
        max_depth[0] = max(max_depth[0], depth)
        key = tuple(rpn)
        if key not in cache:
            cache[key] = len(code)
            code.extend(rpn)
        return cache[key]

    exits, region_rows, event_defs, groups, locs = [], [], [], [], []
    loc_by_region = {}
    for loc in model["locations"]:
        loc_by_region.setdefault(loc["region"], []).append(loc)
    jinjo_by_region = {}
    for r_i, name, rule in model["jinjo_events"]:
        jinjo_by_region.setdefault(r_i, []).append((name, rule))
    groups_by_region = {}
    for r_i, rule, count, world in model["note_groups"]:
        groups_by_region.setdefault(r_i, []).append((rule, count, world))

    world_ids = ov["worlds"]
    for r_i, region in enumerate(regions):
        row = {"exit_first": len(exits), "loc_first": len(locs),
               "event_first": len(event_defs), "group_first": len(groups)}
        for target, rule in region["exits"]:
            exits.append((region_index[target], rule_offset(rule)))
        for name, rule in region["events"] + jinjo_by_region.get(r_i, []):
            event_defs.append((event_index[name], rule_offset(rule)))
        for rule, count, world in groups_by_region.get(r_i, []):
            groups.append((rule_offset(rule), count, world_ids[world]))
        for loc in loc_by_region.get(r_i, []):
            locs.append(dict(loc, rule_off=rule_offset(loc["rule"])))
        row.update(exit_count=len(exits) - row["exit_first"], loc_count=len(locs) - row["loc_first"],
                   event_count=len(event_defs) - row["event_first"],
                   group_count=len(groups) - row["group_first"], world=world_ids[region["world"]])
        region_rows.append(row)

    undefined = sorted(ctx["referenced_events"] - set(event_names))
    if undefined:
        raise LogicError(f"eventos referenciados e não definidos: {undefined}")

    starting = 0
    for move in ov["starting_moves"]:
        starting |= 1 << ov["moves"][move]
    lookup = {"JIGGY": [0xFFFF] * 101, "HONEYCOMB": [0xFFFF] * 25, "TOKEN": [0xFFFF] * 128,
              "MOLEHILL": [0xFFFF] * 10}
    for i, loc in enumerate(locs):
        lookup[loc["kind"]][loc["game_id"]] = i

    numeric = list(code)
    for row in region_rows:
        numeric += [row[k] for k in ("exit_first", "exit_count", "loc_first", "loc_count",
                                     "event_first", "event_count", "group_first", "group_count",
                                     "world")]
    for e in exits:
        numeric += list(e)
    for loc in locs:
        numeric += [loc["region"], loc["rule_off"], loc["game_id"], LOC_KINDS[loc["kind"]],
                    world_ids[loc["world"]], loc["vanilla_item"]]
    for e in event_defs:
        numeric += list(e)
    for g in groups:
        numeric += list(g)
    data_hash = fnv1a(numeric)

    defines = {
        "BKR_REGION_COUNT": len(region_rows), "BKR_LOCATION_COUNT": len(locs),
        "BKR_EXIT_COUNT": len(exits), "BKR_EVENT_COUNT": len(event_names),
        "BKR_EVENT_DEF_COUNT": len(event_defs), "BKR_NOTE_GROUP_COUNT": len(groups),
        "BKR_RULE_CODE_SIZE": len(code), "BKR_START_REGION": region_index[ov["start_region"]],
        "BKR_GOAL_EVENT": event_index[ov["goal_event"]],
    }
    header = render_header(defines, starting, data_hash, raw["source"]["pinned_commit"])
    source = render_source(region_rows, exits, locs, event_defs, groups, code, regions,
                           event_names, lookup, world_ids)
    report = {
        "defines": defines, "max_rule_depth": max_depth[0], "ignored_checks": model["ignored"],
        "data_hash": f"0x{data_hash:08X}",
        "locations_by_kind": {k: sum(1 for l in locs if l["kind"] == k) for k in LOC_KINDS},
    }
    return header, source, report


def render_header(defines, starting, data_hash, commit):
    lines = [
        "/* Arquivo gerado por tools/compile_logic.py — NÃO EDITE À MÃO. */",
        "#ifndef BKR_LOGIC_DATA_H", "#define BKR_LOGIC_DATA_H", "",
        f'#define BKR_LOGIC_SOURCE_COMMIT "{commit}"',
    ]
    for key, value in defines.items():
        lines.append(f"#define {key} {value}")
    lines += [
        f"#define BKR_STARTING_MOVES 0x{starting:08X}u",
        f"#define BKR_LOGIC_DATA_HASH 0x{data_hash:08X}u",
        "", "#endif", "",
    ]
    return "\n".join(lines)


def render_source(region_rows, exits, locs, event_defs, groups, code, regions, event_names,
                  lookup, world_ids):
    out = ["/* Arquivo gerado por tools/compile_logic.py — NÃO EDITE À MÃO. */",
           '#include "bkr/bkr_world.h"', ""]
    out.append("const BkrRegionDef bkr_regions[BKR_REGION_COUNT] = {")
    for row, region in zip(region_rows, regions):
        out.append("    {{{exit_first}, {exit_count}, {loc_first}, {loc_count}, {event_first}, "
                   "{event_count}, {group_first}, {group_count}, {world}, 0}}, /* {name} */"
                   .format(name=region["id"], **row))
    out.append("};\n")
    out.append("const BkrExitDef bkr_exits[BKR_EXIT_COUNT] = {")
    out += [f"    {{{to}, {rule}}}," for to, rule in exits]
    out.append("};\n")
    out.append("const BkrLocDef bkr_locations[BKR_LOCATION_COUNT] = {")
    for i, loc in enumerate(locs):
        out.append(f"    {{{loc['region']}, {loc['rule_off']}, {loc['game_id']}, "
                   f"{LOC_KINDS[loc['kind']]}, {world_ids[loc['world']]}, {loc['vanilla_item']}, 0}},"
                   f" /* {i}: {loc['id']} */")
    out.append("};\n")
    out.append("const BkrEventDef bkr_event_defs[BKR_EVENT_DEF_COUNT] = {")
    out += [f"    {{{ev}, {rule}}}, /* {event_names[ev]} */" for ev, rule in event_defs]
    out.append("};\n")
    out.append("const BkrNoteGroupDef bkr_note_groups[BKR_NOTE_GROUP_COUNT] = {")
    out += [f"    {{{rule}, {count}, {world}}}," for rule, count, world in groups]
    out.append("};\n")
    out.append("const bkr_u16 bkr_rule_code[BKR_RULE_CODE_SIZE] = {")
    for i in range(0, len(code), 16):
        out.append("    " + ", ".join(str(v) for v in code[i:i + 16]) + ",")
    out.append("};\n")
    out.append("const char* const bkr_location_names[BKR_LOCATION_COUNT] = {")
    out += [f"    {c_string(loc['name'])}," for loc in locs]
    out.append("};\n")
    out.append("const char* const bkr_region_names[BKR_REGION_COUNT] = {")
    out += [f"    {c_string(r['id'])}," for r in regions]
    out.append("};\n")
    out.append("const char* const bkr_event_names[BKR_EVENT_COUNT] = {")
    out += [f"    {c_string(n)}," for n in event_names]
    out.append("};\n")
    for kind, size in (("JIGGY", 101), ("HONEYCOMB", 25), ("TOKEN", 128), ("MOLEHILL", 10)):
        name = {"JIGGY": "jiggy", "HONEYCOMB": "honeycomb", "TOKEN": "token", "MOLEHILL": "mole"}[kind]
        out.append(f"const bkr_u16 bkr_loc_by_{name}[{size}] = {{")
        values = lookup[kind]
        for i in range(0, size, 16):
            out.append("    " + ", ".join(f"0x{v:04X}" for v in values[i:i + 16]) + ",")
        out.append("};\n")
    return "\n".join(out)


def main(argv=None):
    parser = argparse.ArgumentParser(description="Compila a lógica do randomizer em C")
    parser.add_argument("--raw", required=True, type=Path)
    parser.add_argument("--overrides", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    args = parser.parse_args(argv)
    raw = json.loads(args.raw.read_text(encoding="utf-8"))
    ov = json.loads(args.overrides.read_text(encoding="utf-8"))
    try:
        header, source, report = compile_all(raw, ov)
    except LogicError as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "bkr_logic_data.h").write_text(header, encoding="utf-8", newline="\n")
    (args.out / "bkr_logic_data.c").write_text(source, encoding="utf-8", newline="\n")
    args.report.write_text(json.dumps(report, indent=1, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps(report["defines"]))
    print(f"profundidade máxima de regra: {report['max_rule_depth']}  hash: {report['data_hash']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

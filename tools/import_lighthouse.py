#!/usr/bin/env python3
"""Importador da lógica do randomizer do Lighthouse (HarbourMasters, licença CC0-1.0).

Lê ``src/port/Rando/StaticData/Checks.cpp`` e ``src/port/Rando/Logic/Regions/*.cpp``
de um checkout do Lighthouse e grava um JSON bruto e neutro em
``logic/lighthouse_raw.json``. As regras C++ são traduzidas para a DSL do projeto
(``has(X)``, ``event(X)``, ``transform(X)``, ``world(X)``, ``note_door(N)``...).

Uso:
    python3 tools/import_lighthouse.py --lighthouse ../lighthouse --out logic/lighthouse_raw.json

O commit validado é ``PINNED_COMMIT``. Outro commit funciona, mas exige rodar a
auditoria de lógica de novo (docs/logic-audit.md).
"""
import argparse
import json
import re
import sys
from pathlib import Path

PINNED_COMMIT = "c09a1f11041f50c2fd86d8aa60f8ee15a3ca13fd"

ABILITY_NAMES = {
    "ABILITY_0_BARGE": "BEAK_BARGE",
    "ABILITY_1_BEAK_BOMB": "BEAK_BOMB",
    "ABILITY_2_BEAK_BUSTER": "BEAK_BUSTER",
    "ABILITY_3_CAMERA_CONTROL": "CAMERA_CONTROL",
    "ABILITY_4_CLAW_SWIPE": "CLAW_SWIPE",
    "ABILITY_5_CLIMB": "CLIMB",
    "ABILITY_6_EGGS": "EGGS",
    "ABILITY_7_FEATHERY_FLAP": "FEATHERY_FLAP",
    "ABILITY_8_FLAP_FLIP": "FLAP_FLIP",
    "ABILITY_9_FLIGHT": "FLIGHT",
    "ABILITY_A_HOLD_A_JUMP_HIGHER": "HIGH_JUMP",
    "ABILITY_B_RATATAT_RAP": "RATATAT_RAP",
    "ABILITY_C_ROLL": "ROLL",
    "ABILITY_D_SHOCK_JUMP": "SHOCK_JUMP",
    "ABILITY_E_WADING_BOOTS": "WADING_BOOTS",
    "ABILITY_F_DIVE": "DIVE",
    "ABILITY_10_TALON_TROT": "TALON_TROT",
    "ABILITY_11_TURBO_TALON": "TURBO_TALON",
    "ABILITY_12_WONDERWING": "WONDERWING",
    "ABILITY_13_1ST_NOTEDOOR": "NOTE_DOORS",
}

TRANSFORM_NAMES = {
    "TRANSFORM_2_TERMITE": "TERMITE",
    "TRANSFORM_3_PUMPKIN": "PUMPKIN",
    "TRANSFORM_4_WALRUS": "WALRUS",
    "TRANSFORM_5_CROC": "CROC",
    "TRANSFORM_6_BEE": "BEE",
}

LEVEL_NAMES = {
    "LEVEL_1_MUMBOS_MOUNTAIN": "MM",
    "LEVEL_2_TREASURE_TROVE_COVE": "TTC",
    "LEVEL_3_CLANKERS_CAVERN": "CC",
    "LEVEL_4_BUBBLEGLOOP_SWAMP": "BGS",
    "LEVEL_5_FREEZEEZY_PEAK": "FP",
    "LEVEL_6_LAIR": "LAIR",
    "LEVEL_7_GOBIS_VALLEY": "GV",
    "LEVEL_8_CLICK_CLOCK_WOOD": "CCW",
    "LEVEL_9_RUSTY_BUCKET_BAY": "RBB",
    "LEVEL_A_MAD_MONSTER_MANSION": "MMM",
    "LEVEL_B_SPIRAL_MOUNTAIN": "SM",
    "LEVEL_D_CUTSCENE": "NONE",
}

ENEMY_NAMES = {
    "ACTOR_124_SIR_SLUSH": "SIR_SLUSH",
    "ACTOR_29B_ZUBBA": "ZUBBA",
}

TYPE_NAMES = {
    "RCTYPE_UNKNOWN": "UNKNOWN",
    "RCTYPE_EMPTY_HONEYCOMB": "HONEYCOMB",
    "RCTYPE_JIGGY": "JIGGY",
    "RCTYPE_JINJO": "JINJO",
    "RCTYPE_MOLEHILL": "MOLEHILL",
    "RCTYPE_MUMBO_TOKEN": "TOKEN",
    "RCTYPE_MUSIC_NOTE": "NOTE",
    "RCTYPE_STOP_N_SWOP": "STOP_N_SWOP",
}

TOKEN_RE = re.compile(r"\s*(&&|\|\||!|\(|\)|[A-Za-z_][A-Za-z0-9_]*|-?\d+)")
CHECK_ROW_RE = re.compile(
    r"RC\(\s*(RC_[A-Z0-9_]+)\s*,\s*(RCTYPE_[A-Z_]+)\s*,\s*(LEVEL_[A-Z0-9_]+)\s*,"
    r"\s*(ACTOR_[A-Z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\)"
)
COLLECTION_ID_RE = re.compile(r"^(JIGGY|HONEYCOMB|MUMBOTOKEN|ABILITY)_([0-9A-F]+)_")


class ImportErrorBkr(Exception):
    """Erro de importação com mensagem legível."""


def strip_comments(text):
    """Remove comentários // e /* */ preservando strings simples."""
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def find_matching(text, open_index, open_ch, close_ch):
    """Retorna o índice do fechamento que casa com ``text[open_index]``."""
    depth = 0
    for i in range(open_index, len(text)):
        ch = text[i]
        if ch == open_ch:
            depth += 1
        elif ch == close_ch:
            depth -= 1
            if depth == 0:
                return i
    raise ImportErrorBkr(f"delimitador '{open_ch}' sem fechamento na posição {open_index}")


def split_first_arg(inner):
    """Separa ``A, resto`` no primeiro vírgula de nível zero."""
    depth = 0
    for i, ch in enumerate(inner):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            return inner[:i].strip(), inner[i + 1:].strip()
    raise ImportErrorBkr(f"entrada sem vírgula: {inner!r}")


def collection_number(collection):
    """Converte JIGGY_9_MM_CHIMPY em 9, MUMBOTOKEN_5D_... em 0x5D etc."""
    match = COLLECTION_ID_RE.match(collection)
    if not match:
        return None
    return int(match.group(2), 16)


def parse_checks(checks_cpp):
    """Lê a tabela de checagens (id, tipo, mundo, ator, coleção, posição)."""
    text = strip_comments(checks_cpp.read_text(encoding="utf-8"))
    checks = {}
    for match in CHECK_ROW_RE.finditer(text):
        rc_id, rc_type, level, actor, collection, x, y, z = match.groups()
        if rc_id in checks:
            raise ImportErrorBkr(f"checagem duplicada: {rc_id}")
        checks[rc_id] = {
            "type": TYPE_NAMES[rc_type],
            "world": LEVEL_NAMES[level],
            "actor": actor,
            "collection": collection,
            "collection_number": collection_number(collection),
            "pos": [int(x), int(y), int(z)],
        }
    if not checks:
        raise ImportErrorBkr("nenhuma checagem encontrada; o formato de Checks.cpp mudou?")
    return checks


class RuleTranslator:
    """Traduz uma expressão booleana C++ do Lighthouse para a DSL do projeto."""

    def __init__(self, source):
        self.tokens = [t for t in TOKEN_RE.findall(source) if t.strip()]
        rebuilt = "".join(self.tokens)
        compact = re.sub(r"\s+", "", source)
        if rebuilt != compact:
            raise ImportErrorBkr(f"caractere não reconhecido na regra: {source!r}")
        self.pos = 0
        self.source = source

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def take(self, expected=None):
        tok = self.peek()
        if tok is None or (expected is not None and tok != expected):
            raise ImportErrorBkr(f"esperado {expected!r}, achado {tok!r} em {self.source!r}")
        self.pos += 1
        return tok

    def translate(self):
        out = self.parse_or()
        if self.peek() is not None:
            raise ImportErrorBkr(f"sobra de tokens em {self.source!r}: {self.tokens[self.pos:]}")
        return out

    def parse_or(self):
        parts = [self.parse_and()]
        while self.peek() == "||":
            self.take("||")
            parts.append(self.parse_and())
        return parts[0] if len(parts) == 1 else "(" + " or ".join(parts) + ")"

    def parse_and(self):
        parts = [self.parse_not()]
        while self.peek() == "&&":
            self.take("&&")
            parts.append(self.parse_not())
        return parts[0] if len(parts) == 1 else "(" + " and ".join(parts) + ")"

    def parse_not(self):
        if self.peek() == "!":
            self.take("!")
            return "not " + self.parse_not()
        return self.parse_atom()

    def parse_atom(self):
        tok = self.take()
        if tok == "(":
            inner = self.parse_or()
            self.take(")")
            return inner
        if tok == "true":
            return "true"
        if tok == "false":
            return "false"
        if tok == "CAN_ATTACK":
            return "can_attack"
        if tok == "CAN_EXTEND_JUMP_DISTANCE":
            return "extend_jump"
        if self.peek() != "(":
            raise ImportErrorBkr(f"identificador solto {tok!r} em {self.source!r}")
        self.take("(")
        arg = self.take()
        self.take(")")
        return self.translate_call(tok, arg)

    def translate_call(self, macro, arg):
        if macro == "CAN_USE_ABILITY":
            return f"has({ABILITY_NAMES[arg]})"
        if macro in ("CAN_USE_TRANSFORMATION", "GET_CURRENT_TRANSFORMATION"):
            return f"transform({TRANSFORM_NAMES[arg]})"
        if macro == "CAN_ACCESS":
            return f"event({arg[3:] if arg.startswith('RA_') else arg})"
        if macro == "CAN_BREAK_OBJECT":
            return f"break({arg.replace('RA_BREAK_OBJECT_', '')})"
        if macro == "CAN_COLLECT_JINJOS":
            return f"jinjos({LEVEL_NAMES[arg]})"
        if macro == "CAN_KILL_ENEMY":
            return f"kill({ENEMY_NAMES[arg]})"
        if macro == "CAN_UNLOCK_NOTE_DOOR":
            return f"note_door({int(arg)})"
        if macro == "CAN_UNLOCK_WORLD":
            return f"world({LEVEL_NAMES[arg]})"
        raise ImportErrorBkr(f"macro desconhecida {macro}({arg}) em {self.source!r}")


def translate_rule(source):
    """Atalho: traduz uma regra C++ em DSL."""
    return RuleTranslator(source).translate()


def parse_entries(block, macro):
    """Extrai pares (primeiro_argumento, regra_traduzida) de CHECK/CONNECTION/EVENT."""
    entries = []
    pattern = re.compile(r"\b" + macro + r"\s*\(")
    pos = 0
    while True:
        match = pattern.search(block, pos)
        if not match:
            break
        open_index = match.end() - 1
        close_index = find_matching(block, open_index, "(", ")")
        first, rule = split_first_arg(block[open_index + 1:close_index])
        entries.append([first, translate_rule(rule)])
        pos = close_index + 1
    return entries


def section(body, name):
    """Retorna o texto de ``.name = { ... }`` dentro do corpo de uma região."""
    match = re.search(r"\." + name + r"\s*=\s*\{", body)
    if not match:
        return ""
    open_index = match.end() - 1
    return body[open_index + 1:find_matching(body, open_index, "{", "}")]


def parse_regions(region_dir):
    """Lê todas as regiões, na ordem dos arquivos e das declarações."""
    regions = []
    seen = set()
    for path in sorted(region_dir.glob("*.cpp")):
        text = strip_comments(path.read_text(encoding="utf-8"))
        for match in re.finditer(r"Regions\[(RR_[A-Z0-9_]+)\]\s*=\s*RandoRegion\s*\{", text):
            region_id = match.group(1)[3:]
            if region_id in seen:
                raise ImportErrorBkr(f"região duplicada: {region_id}")
            seen.add(region_id)
            open_index = match.end() - 1
            body = text[open_index + 1:find_matching(text, open_index, "{", "}")]
            name = re.search(r'\.regionName\s*=\s*"([^"]*)"', body)
            map_id = re.search(r"\.mapId\s*=\s*(MAP_[A-Z0-9_]+)", body)
            checks = parse_entries(section(body, "checks"), "CHECK")
            exits = parse_entries(section(body, "connections"), "CONNECTION")
            events = parse_entries(section(body, "events"), "EVENT")
            regions.append({
                "id": region_id,
                "name": name.group(1) if name else region_id,
                "map": map_id.group(1) if map_id else None,
                "file": path.name,
                "checks": checks,
                "exits": [[target[3:], rule] for target, rule in exits],
                "events": [[event[3:], rule] for event, rule in events],
            })
    if not regions:
        raise ImportErrorBkr("nenhuma região encontrada; o formato de Regions/*.cpp mudou?")
    return regions


def build_raw(lighthouse_root):
    """Monta o dicionário bruto a partir de um checkout do Lighthouse."""
    rando = lighthouse_root / "src" / "port" / "Rando"
    checks = parse_checks(rando / "StaticData" / "Checks.cpp")
    regions = parse_regions(rando / "Logic" / "Regions")
    referenced = {check for region in regions for check, _ in region["checks"]}
    unknown = sorted(referenced - set(checks))
    if unknown:
        raise ImportErrorBkr(f"regiões citam checagens inexistentes: {unknown[:10]}")
    return {
        "source": {
            "project": "HarbourMasters/Lighthouse",
            "license": "CC0-1.0",
            "pinned_commit": PINNED_COMMIT,
        },
        "checks": checks,
        "regions": regions,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--lighthouse", required=True, type=Path, help="checkout do Lighthouse")
    parser.add_argument("--out", required=True, type=Path, help="arquivo JSON de saída")
    args = parser.parse_args(argv)
    try:
        raw = build_raw(args.lighthouse)
    except ImportErrorBkr as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(raw, indent=1, ensure_ascii=False) + "\n", encoding="utf-8", newline="\n")
    by_type = {}
    for check in raw["checks"].values():
        by_type[check["type"]] = by_type.get(check["type"], 0) + 1
    print(f"regiões: {len(raw['regions'])}")
    print("checagens por tipo: " + ", ".join(f"{k}={v}" for k, v in sorted(by_type.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())

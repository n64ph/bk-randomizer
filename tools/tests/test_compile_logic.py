"""Testes do compilador de lógica (tools/compile_logic.py).

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import compile_logic as cl  # noqa: E402


class OpcodeSyncTest(unittest.TestCase):
    def test_opcodes_match_c_header(self):
        header = (ROOT / "core" / "include" / "bkr" / "bkr_rules.h").read_text(encoding="utf-8")
        c_ops = {m.group(1): int(m.group(2)) for m in re.finditer(r"BKR_OP_([A-Z_]+)\s*=\s*(\d+)", header)}
        self.assertEqual(c_ops, cl.OPCODES)
        depth = re.search(r"#define BKR_RULE_MAX_DEPTH (\d+)", header)
        self.assertEqual(int(depth.group(1)), cl.MAX_DEPTH)


class ParserTest(unittest.TestCase):
    def test_precedence(self):
        node = cl.Parser("has(EGGS) or has(CLIMB) and not has(DIVE)").parse()
        self.assertEqual(node, ("or", [("call", "has", "EGGS"),
                                       ("and", [("call", "has", "CLIMB"), ("not", ("call", "has", "DIVE"))])]))

    def test_rejects_garbage(self):
        with self.assertRaises(cl.LogicError):
            cl.Parser("has(EGGS) &&").parse()
        with self.assertRaises(cl.LogicError):
            cl.Parser("has(EGGS) has(DIVE)").parse()

    def test_simplify_constants(self):
        self.assertEqual(cl.simplify(("and", [("true",), ("op", "HAS", 6)])), ("op", "HAS", 6))
        self.assertEqual(cl.simplify(("or", [("true",), ("op", "HAS", 6)])), ("true",))
        self.assertEqual(cl.simplify(("not", ("false",))), ("true",))
        self.assertEqual(cl.simplify(("and", [("op", "HAS", 6), ("and", [("op", "HAS", 6), ("op", "HAS", 8)])])),
                         ("and", [("op", "HAS", 6), ("op", "HAS", 8)]))

    def test_emit_rpn(self):
        node = ("and", [("op", "HAS", 6), ("or", [("op", "HAS", 8), ("op", "TOKENS", 5)])])
        rpn, depth = cl.emit_rpn(node, {}, {})
        ops = cl.OPCODES
        self.assertEqual(rpn, [ops["HAS"], 6, ops["HAS"], 8, ops["TOKENS"], 5, ops["OR"], ops["AND"], ops["END"]])
        self.assertEqual(depth, 3)

    def test_display_name(self):
        self.assertEqual(cl.display_name("RC_MM_JIGGY_TICKERS_TOWER", "MM", "JIGGY"), "MM Jiggy - Tickers Tower")
        self.assertEqual(cl.display_name("RC_GL_MUMBO_TOKEN_ABOVE_CC_ENTRANCE", "LAIR", "TOKEN"),
                         "GL Token - Above CC Entrance")
        self.assertEqual(cl.display_name("RC_TTC_EMPTY_HONEYCOMB_UNDERWATER", "TTC", "HONEYCOMB"),
                         "TTC Honeycomb - Underwater")


class RealDataTest(unittest.TestCase):
    """Compila os dados reais (logic/*.json) e confere os valores-ouro do roteiro."""

    @classmethod
    def setUpClass(cls):
        cls.raw = json.loads((ROOT / "logic" / "lighthouse_raw.json").read_text(encoding="utf-8"))
        cls.ov = json.loads((ROOT / "logic" / "overrides.json").read_text(encoding="utf-8"))

    def test_counts_and_hash(self):
        _, _, report = cl.compile_all(self.raw, self.ov)
        self.assertEqual(report["locations_by_kind"], {"JIGGY": 100, "HONEYCOMB": 24, "TOKEN": 116, "MOLEHILL": 9})
        self.assertEqual(report["defines"]["BKR_REGION_COUNT"], 125)
        self.assertEqual(report["ignored_checks"], {"STOP_N_SWOP": 7, "MOLEHILL": 6})
        self.assertLessEqual(report["max_rule_depth"], cl.MAX_DEPTH)
        self.assertEqual(report["data_hash"], "0x5C82A342")

    def test_compile_is_pure(self):
        first = cl.compile_all(self.raw, self.ov)
        second = cl.compile_all(self.raw, self.ov)
        self.assertEqual(first, second)

    def test_lighthouse_fixes_are_applied(self):
        model = cl.build_model(self.raw, self.ov)
        rules = {loc["id"]: loc["rule"] for loc in model["locations"]}
        self.assertEqual(rules["RC_BGS_JIGGY_JINJO"], "jinjos(BGS)")
        events = {name for region in model["regions"] for name, _ in region["events"]}
        for switch in ("WITCH_SWITCH_CLANKERS_CAVERN", "WITCH_SWITCH_MAD_MONSTER_MANSION",
                       "WITCH_SWITCH_CLICK_CLOCK_WOOD"):
            self.assertIn(switch, events)
        tokens = {loc["id"]: loc["game_id"] for loc in model["locations"] if loc["kind"] == "TOKEN"}
        self.assertEqual(tokens["RC_MMM_MUMBO_TOKEN_INSIDE_LOGGO"], 116)

    def test_bad_override_fails(self):
        broken = json.loads(json.dumps(self.ov))
        broken["extra_exits"].append({"from": "NAO_EXISTE", "to": "GOBIS_VALLEY_UPPER_AREA", "rule": "true"})
        with self.assertRaises(cl.LogicError):
            cl.compile_all(self.raw, broken)


if __name__ == "__main__":
    unittest.main()

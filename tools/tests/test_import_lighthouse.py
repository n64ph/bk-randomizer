"""Testes do importador da lógica do Lighthouse (tools/import_lighthouse.py).

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import import_lighthouse as il  # noqa: E402

CHECKS_CPP = """
namespace Rando { namespace StaticData {
    // comentário que deve ser ignorado: RC(RC_FAKE, RCTYPE_JIGGY, ...)
    RC(RC_MM_JIGGY_CHIMPY,   RCTYPE_JIGGY,       LEVEL_1_MUMBOS_MOUNTAIN, ACTOR_46_JIGGY,        JIGGY_9_MM_CHIMPY,        0, 0, 0),
    RC(RC_MM_MUMBO_TOKEN_X,  RCTYPE_MUMBO_TOKEN, LEVEL_1_MUMBOS_MOUNTAIN, ACTOR_2D_MUMBO_TOKEN,  MUMBOTOKEN_5D_MM_X,       10, -20, 30),
    RC(RC_MM_NOTE_A,         RCTYPE_MUSIC_NOTE,  LEVEL_1_MUMBOS_MOUNTAIN, ACTOR_51_MUSIC_NOTE,   NULL,                     1, 2, 3),
}}
"""

REGION_CPP = """
static RegisterShipInitFunc initFunc([]() {
    Regions[RR_MM_START] = RandoRegion{ .regionName = "Mumbo's Mountain", .mapId = MAP_2_MM_MUMBOS_MOUNTAIN,
        .checks = {
            CHECK(RC_MM_JIGGY_CHIMPY,  CAN_USE_ABILITY(ABILITY_5_CLIMB) || CAN_USE_ABILITY(ABILITY_8_FLAP_FLIP)),
            CHECK(RC_MM_NOTE_A,        true),
        },
        .connections = {
            CONNECTION(RR_MM_TOP,      CAN_USE_ABILITY(ABILITY_10_TALON_TROT) && CAN_ATTACK),
        },
        .events = {
            EVENT(RA_MM_EVENT,         CAN_USE_TRANSFORMATION(TRANSFORM_2_TERMITE)),
        },
    };
    Regions[RR_MM_TOP] = RandoRegion{ .regionName = "Topo", .mapId = MAP_2_MM_MUMBOS_MOUNTAIN,
        .checks = {
            CHECK(RC_MM_MUMBO_TOKEN_X, CAN_ACCESS(RA_MM_EVENT)),
        },
    };
});
"""


class TranslatorTest(unittest.TestCase):
    def test_macros(self):
        rule = il.translate_rule("CAN_USE_ABILITY(ABILITY_6_EGGS) && (CAN_UNLOCK_NOTE_DOOR(180) || "
                                 "CAN_USE_TRANSFORMATION(TRANSFORM_6_BEE))")
        self.assertEqual(rule, "(has(EGGS) and (note_door(180) or transform(BEE)))")
        self.assertEqual(il.translate_rule("CAN_ACCESS(RA_WITCH_SWITCH_GOBIS_VALLEY)"),
                         "event(WITCH_SWITCH_GOBIS_VALLEY)")
        self.assertEqual(il.translate_rule("CAN_BREAK_OBJECT(RA_BREAK_OBJECT_GRATE)"), "break(GRATE)")
        self.assertEqual(il.translate_rule("CAN_UNLOCK_WORLD(LEVEL_8_CLICK_CLOCK_WOOD)"), "world(CCW)")
        self.assertEqual(il.translate_rule("!CAN_ATTACK"), "not can_attack")
        self.assertEqual(il.translate_rule("true"), "true")

    def test_unknown_macro_fails(self):
        with self.assertRaises(il.ImportErrorBkr):
            il.translate_rule("CAN_DO_MAGIC(X)")

    def test_unknown_character_fails(self):
        with self.assertRaises(il.ImportErrorBkr):
            il.translate_rule("CAN_ATTACK + 1")

    def test_collection_number(self):
        self.assertEqual(il.collection_number("JIGGY_9_MM_CHIMPY"), 9)
        self.assertEqual(il.collection_number("MUMBOTOKEN_5D_CCW_X"), 0x5D)
        self.assertEqual(il.collection_number("ABILITY_10_TALON_TROT"), 0x10)
        self.assertIsNone(il.collection_number("NULL"))


class SyntheticTreeTest(unittest.TestCase):
    def test_build_raw_from_minimal_tree(self):
        with tempfile.TemporaryDirectory() as tmp:
            rando = Path(tmp) / "src" / "port" / "Rando"
            (rando / "StaticData").mkdir(parents=True)
            (rando / "Logic" / "Regions").mkdir(parents=True)
            (rando / "StaticData" / "Checks.cpp").write_text(CHECKS_CPP, encoding="utf-8")
            (rando / "Logic" / "Regions" / "MumbosMountain.cpp").write_text(REGION_CPP, encoding="utf-8")
            raw = il.build_raw(Path(tmp))
        self.assertEqual(sorted(raw["checks"]), ["RC_MM_JIGGY_CHIMPY", "RC_MM_MUMBO_TOKEN_X", "RC_MM_NOTE_A"])
        token = raw["checks"]["RC_MM_MUMBO_TOKEN_X"]
        self.assertEqual((token["type"], token["world"], token["collection_number"], token["pos"]),
                         ("TOKEN", "MM", 0x5D, [10, -20, 30]))
        self.assertEqual([r["id"] for r in raw["regions"]], ["MM_START", "MM_TOP"])
        start = raw["regions"][0]
        self.assertEqual(start["file"], "MumbosMountain.cpp")
        self.assertEqual(start["map"], "MAP_2_MM_MUMBOS_MOUNTAIN")
        self.assertEqual(start["checks"][0], ["RC_MM_JIGGY_CHIMPY", "(has(CLIMB) or has(FLAP_FLIP))"])
        self.assertEqual(start["exits"], [["MM_TOP", "(has(TALON_TROT) and can_attack)"]])
        self.assertEqual(start["events"], [["MM_EVENT", "transform(TERMITE)"]])
        self.assertEqual(raw["regions"][1]["checks"], [["RC_MM_MUMBO_TOKEN_X", "event(MM_EVENT)"]])

    def test_unknown_check_reference_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            rando = Path(tmp) / "src" / "port" / "Rando"
            (rando / "StaticData").mkdir(parents=True)
            (rando / "Logic" / "Regions").mkdir(parents=True)
            (rando / "StaticData" / "Checks.cpp").write_text(CHECKS_CPP, encoding="utf-8")
            (rando / "Logic" / "Regions" / "MumbosMountain.cpp").write_text(
                REGION_CPP.replace("RC_MM_NOTE_A,", "RC_MM_NAO_EXISTE,"), encoding="utf-8")
            with self.assertRaises(il.ImportErrorBkr):
                il.build_raw(Path(tmp))


if __name__ == "__main__":
    unittest.main()

"""Testes da checagem de conflitos de RECOMP_PATCH com o Banjo: Recompiled.

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import check_patch_conflicts as cpc  # noqa: E402


class ConflictCheckTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.base = root / "BanjoRecomp"
        self.mod = root / "src"
        (self.base / "patches").mkdir(parents=True)
        self.mod.mkdir()
        (self.base / "patches" / "a.c").write_text(
            "RECOMP_PATCH void gameFile_load(s32 gamenum) {}\n"
            "RECOMP_PATCH Actor *chfoo_spawn(ActorMarker *m) { return 0; }\n", encoding="utf-8")
        (self.base / "banjo.us.rev0.toml").write_text(
            "[[patches.instruction]]\nfunc = \"func_8038AC04\"\nvram = 0x8038AC0C\nvalue = 0x3C0E8000\n",
            encoding="utf-8")

    def tearDown(self):
        self.tmp.cleanup()

    def run_check(self, mod_source):
        """Roda a checagem e guarda a saída em self.output (sem poluir o log dos testes)."""
        (self.mod / "b.c").write_text(mod_source, encoding="utf-8")
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer), contextlib.redirect_stderr(buffer):
            code = cpc.main(["--banjorecomp", str(self.base), "--mod", str(self.mod)])
        self.output = buffer.getvalue()
        return code

    def test_regex_finds_pointer_returns(self):
        self.assertEqual(cpc.patched_functions(self.base / "patches"), {"gameFile_load", "chfoo_spawn"})

    def test_detects_patch_conflict(self):
        self.assertEqual(self.run_check("RECOMP_PATCH void gameFile_load(s32 gamenum) {}\n"), 1)
        self.assertIn("gameFile_load", self.output)

    def test_detects_instruction_patch_conflict(self):
        self.assertEqual(self.run_check("RECOMP_PATCH void func_8038AC04(void) {}\n"), 1)
        self.assertIn("func_8038AC04", self.output)

    def test_hooks_and_other_patches_are_fine(self):
        source = ("RECOMP_HOOK(\"gameFile_load\") void on_load(s32 gamenum) {}\n"
                  "RECOMP_PATCH s32 item_adjustByDiffWithHud(s32 item, s32 diff) { return 0; }\n")
        self.assertEqual(self.run_check(source), 0)
        self.assertIn("ok: nenhum conflito", self.output)

    def test_missing_banjorecomp_folder(self):
        buffer = io.StringIO()
        with contextlib.redirect_stdout(buffer), contextlib.redirect_stderr(buffer):
            code = cpc.main(["--banjorecomp", str(self.base / "nao_existe"), "--mod", str(self.mod)])
        self.assertEqual(code, 2)


if __name__ == "__main__":
    unittest.main()

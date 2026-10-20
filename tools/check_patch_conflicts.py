#!/usr/bin/env python3
"""Falha se o mod usar RECOMP_PATCH em uma função que o Banjo: Recompiled já
altera: funções com RECOMP_PATCH em BanjoRecomp/patches/*.c (nesse caso o mod
nem carrega) ou com patch de instrução em BanjoRecomp/banjo.us.rev0.toml (o
patch do mod descartaria a correção do base). Hooks (RECOMP_HOOK e
RECOMP_HOOK_RETURN) em funções alteradas são permitidos pelo runtime e não
entram na checagem.

Uso:
    python3 tools/check_patch_conflicts.py --banjorecomp ../BanjoRecomp --mod src
"""
import argparse
import re
import sys
from pathlib import Path

PATCH_RE = re.compile(r"RECOMP_(?:FORCE_)?PATCH\s+[^;{(]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
INSTRUCTION_BLOCK_RE = re.compile(r"\[\[patches\.instruction\]\]\s*func\s*=\s*\"([A-Za-z_][A-Za-z0-9_]*)\"")


def patched_functions(folder):
    """Nomes de funções marcadas com RECOMP_PATCH em todos os .c da pasta."""
    names = set()
    for path in sorted(Path(folder).rglob("*.c")):
        text = path.read_text(encoding="utf-8", errors="replace")
        names.update(PATCH_RE.findall(text))
    return names


def instruction_patched_functions(toml_path):
    """Funções com [[patches.instruction]] no TOML de recompilação do base."""
    if not toml_path.is_file():
        return set()
    return set(INSTRUCTION_BLOCK_RE.findall(toml_path.read_text(encoding="utf-8", errors="replace")))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--banjorecomp", required=True, type=Path, help="checkout do BanjoRecomp")
    parser.add_argument("--mod", required=True, type=Path, help="pasta src/ do mod")
    args = parser.parse_args(argv)
    base_dir = args.banjorecomp / "patches"
    if not base_dir.is_dir():
        print(f"ERRO: {base_dir} não existe", file=sys.stderr)
        return 2
    base = patched_functions(base_dir)
    instr = instruction_patched_functions(args.banjorecomp / "banjo.us.rev0.toml")
    ours = patched_functions(args.mod)
    print(f"patches do Banjo: Recompiled: {len(base)} funções + {len(instr)} patches de instrução | "
          f"patches do mod: {len(ours)} -> {sorted(ours)}")
    conflicts = sorted((base | instr) & ours)
    if conflicts:
        print("ERRO: o mod substitui funções já alteradas pelo Banjo: Recompiled:")
        for name in conflicts:
            print(f"  - {name} (use RECOMP_HOOK/RECOMP_HOOK_RETURN em vez de RECOMP_PATCH)")
        return 1
    print("ok: nenhum conflito de RECOMP_PATCH")
    return 0


if __name__ == "__main__":
    sys.exit(main())

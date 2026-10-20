#!/usr/bin/env python3
"""Executa o núcleo compilado em MIPS (big-endian, 32 bits) num emulador
Unicorn e compara o hash do placement com o do host. Prova que o seed gerado
dentro do jogo é idêntico ao da CLI, sem precisar abrir o jogo.

Pré-requisito:  pip install unicorn pyelftools
Uso:            python3 tools/mips_selftest.py core/build/mips/selftest.elf --expect 31098721
"""
import argparse
import sys
import time

from elftools.elf.elffile import ELFFile
from unicorn import Uc, UcError, UC_ARCH_MIPS, UC_MODE_MIPS32, UC_MODE_BIG_ENDIAN
from unicorn.mips_const import UC_MIPS_REG_29, UC_MIPS_REG_31, UC_MIPS_REG_2

STACK_TOP = 0x00300000
STACK_SIZE = 0x17F0          # mesmo tamanho da pilha da thread principal do jogo
RETURN_MAGIC = 0x00200000    # endereço de parada (jr $ra cai aqui)
PAGE = 0x1000


def align_down(value):
    return value & ~(PAGE - 1)


def align_up(value):
    return (value + PAGE - 1) & ~(PAGE - 1)


def load(uc, path):
    """Mapeia a união dos segmentos PT_LOAD (podem dividir páginas) e copia os dados."""
    with open(path, "rb") as f:
        elf = ELFFile(f)
        entry = elf.header["e_entry"]
        segments = [s for s in elf.iter_segments() if s["p_type"] == "PT_LOAD"]
        start = min(align_down(s["p_vaddr"]) for s in segments)
        end = max(align_up(s["p_vaddr"] + s["p_memsz"]) for s in segments)
        uc.mem_map(start, end - start)
        for seg in segments:
            uc.mem_write(seg["p_vaddr"], seg.data())
    return entry


def main(argv=None):
    parser = argparse.ArgumentParser(description="Autoteste do núcleo MIPS")
    parser.add_argument("elf")
    parser.add_argument("--expect", required=True, help="hash esperado em hexadecimal")
    args = parser.parse_args(argv)
    uc = Uc(UC_ARCH_MIPS, UC_MODE_MIPS32 + UC_MODE_BIG_ENDIAN)
    entry = load(uc, args.elf)
    uc.mem_map(align_down(STACK_TOP - 0x10000), 0x10000)
    uc.mem_map(RETURN_MAGIC, PAGE)
    uc.reg_write(UC_MIPS_REG_29, STACK_TOP - 16)
    uc.reg_write(UC_MIPS_REG_31, RETURN_MAGIC)
    # "Pinta" a pilha com um padrão para medir o uso máximo depois.
    canary = b"\xDE\xAD\xBE\xEF" * (0x10000 // 4)
    uc.mem_write(STACK_TOP - 0x10000, canary)
    started = time.time()
    try:
        uc.emu_start(entry, RETURN_MAGIC)
    except UcError as exc:
        print(f"ERRO no emulador: {exc}", file=sys.stderr)
        return 1
    elapsed = time.time() - started
    value = uc.reg_read(UC_MIPS_REG_2)
    expected = int(args.expect, 16)
    stack = bytes(uc.mem_read(STACK_TOP - 0x10000, 0x10000))
    first_dirty = next((i for i in range(0, 0x10000, 4) if stack[i:i + 4] != canary[:4]), 0x10000)
    used = 0x10000 - first_dirty
    print(f"hash MIPS={value:08X} esperado={expected:08X} tempo={elapsed:.2f}s pilha={used} bytes")
    if used >= STACK_SIZE // 2:
        print(f"ERRO: o núcleo usou {used} bytes de pilha; o limite de projeto é {STACK_SIZE // 2}", file=sys.stderr)
        return 1
    return 0 if value == expected else 1


if __name__ == "__main__":
    sys.exit(main())

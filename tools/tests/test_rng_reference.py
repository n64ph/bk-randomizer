"""Implementação de referência (Python) do RNG, do FNV-1a, do código de
compartilhamento e da impressão digital do seed. Os valores fixos daqui são os
mesmos dos testes C em core/tests/: se um lado mudar, os dois testes acusam.

Rodar:  python3 -m unittest discover -s tools/tests -v
"""
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MASK = 0xFFFFFFFF
BASE32 = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"
GENERATOR_VERSION = 1  # BKR_GENERATOR_VERSION em core/include/bkr/bkr_settings.h


def splitmix32(state):
    """Retorna (novo_estado, valor), como bkr_splitmix32."""
    state = (state + 0x9E3779B9) & MASK
    z = state
    z = ((z ^ (z >> 16)) * 0x85EBCA6B) & MASK
    z = ((z ^ (z >> 13)) * 0xC2B2AE35) & MASK
    return state, z ^ (z >> 16)


def rotl(x, k):
    return ((x << k) | (x >> (32 - k))) & MASK


class Xoshiro128:
    """xoshiro128** semeado por splitmix32, como bkr_rng_seed/bkr_rng_next."""

    def __init__(self, seed):
        x = seed
        self.s = []
        for _ in range(4):
            x, v = splitmix32(x)
            self.s.append(v)
        if not any(self.s):
            self.s[0] = 1

    def next(self):
        s = self.s
        result = (rotl((s[1] * 5) & MASK, 7) * 9) & MASK
        t = (s[1] << 9) & MASK
        s[2] ^= s[0]
        s[3] ^= s[1]
        s[1] ^= s[2]
        s[0] ^= s[3]
        s[2] ^= t
        s[3] = rotl(s[3], 11)
        return result


def fnv1a(data, h=0x811C9DC5):
    for b in data:
        h ^= b
        h = (h * 0x01000193) & MASK
    return h


def fnv1a_u32(value, h):
    """Mistura os 4 bytes em little-endian, como bkr_fnv1a_u32."""
    return fnv1a(bytes([(value >> (8 * i)) & 0xFF for i in range(4)]), h)


def settings_block(bits):
    """6 caracteres base32: 24 bits de opções + 6 bits de checagem."""
    check = fnv1a_u32(bits, 0x811C9DC5) & 0x3F
    value = (bits & 0xFFFFFF) | (check << 24)
    out = []
    for _ in range(6):
        out.append(BASE32[value & 31])
        value >>= 5
    return "".join(reversed(out))


def fingerprint(bits, seed, data_hash):
    """Como bkr_seed_fingerprint: FNV de "BKR|", versão, dados, bloco, "|", semente."""
    h = fnv1a(b"BKR|")
    h = fnv1a_u32(GENERATOR_VERSION, h)
    h = fnv1a_u32(data_hash, h)
    h = fnv1a(settings_block(bits).encode("ascii"), h)
    h = fnv1a(b"|", h)
    return fnv1a(seed.encode("ascii"), h)


# logic=glitchless(0) | shuffle=mixed(2)<<1 | scope=game(0)<<3 | moves=molehills(1)<<4 | access=all(0)<<5
DEFAULT_SETTINGS_BITS = 0 | (2 << 1) | (0 << 3) | (1 << 4) | (0 << 5)


class ReferenceTest(unittest.TestCase):
    def test_fnv_vectors(self):
        self.assertEqual(fnv1a(b""), 0x811C9DC5)
        self.assertEqual(fnv1a(b"a"), 0xE40C292C)
        self.assertEqual(fnv1a(b"foobar"), 0xBF9CF968)
        self.assertEqual(fnv1a_u32(1, 0x811C9DC5), 0xFB69B604)

    def test_splitmix_golden(self):
        x, a = splitmix32(0)
        x, b = splitmix32(x)
        self.assertEqual((a, b), (0x92CA2F0E, 0x3CD6E3F3))

    def test_xoshiro_golden(self):
        rng = Xoshiro128(12345)
        self.assertEqual([rng.next() for _ in range(3)], [0x1EEA3CC1, 0x1A40A62E, 0xFC4CD240])

    def test_share_block_golden(self):
        self.assertEqual(DEFAULT_SETTINGS_BITS, 20)
        self.assertEqual(settings_block(DEFAULT_SETTINGS_BITS), "GG000M")

    def test_fingerprint_golden(self):
        header = (ROOT / "core" / "generated" / "bkr_logic_data.h").read_text(encoding="utf-8")
        data_hash = int(re.search(r"BKR_LOGIC_DATA_HASH 0x([0-9A-F]{8})u", header).group(1), 16)
        self.assertEqual(data_hash, 0x5C82A342)
        self.assertEqual(fingerprint(DEFAULT_SETTINGS_BITS, "BANJO", data_hash), 0x623A1517)


if __name__ == "__main__":
    unittest.main()

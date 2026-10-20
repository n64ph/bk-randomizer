/* bkr_hash.c — FNV-1a de 32 bits (byte a byte, independente de endianness). */
#include "bkr/bkr_hash.h"

bkr_u32 bkr_fnv1a_bytes(bkr_u32 hash, const bkr_u8* data, bkr_u32 len) {
    bkr_u32 i;
    for (i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= BKR_FNV_PRIME;
    }
    return hash;
}

bkr_u32 bkr_fnv1a_str(bkr_u32 hash, const char* text) {
    while (*text != '\0') {
        hash ^= (bkr_u8)*text;
        hash *= BKR_FNV_PRIME;
        text++;
    }
    return hash;
}

/* Mistura os 4 bytes do valor em ordem little-endian, calculados por
 * deslocamento: o resultado é o mesmo no host (LE) e no MIPS (BE). */
bkr_u32 bkr_fnv1a_u32(bkr_u32 hash, bkr_u32 value) {
    int i;
    for (i = 0; i < 4; i++) {
        hash ^= (value >> (i * 8)) & 0xFFu;
        hash *= BKR_FNV_PRIME;
    }
    return hash;
}

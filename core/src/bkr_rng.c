/* bkr_rng.c — xoshiro128** (Blackman & Vigna) semeado por splitmix32. */
#include "bkr/bkr_rng.h"

static bkr_u32 bkr_rotl(bkr_u32 x, int k) {
    return (x << k) | (x >> (32 - k));
}

bkr_u32 bkr_splitmix32(bkr_u32* x) {
    bkr_u32 z;
    *x += 0x9E3779B9u;
    z = *x;
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

void bkr_rng_seed(BkrRng* rng, bkr_u32 seed) {
    bkr_u32 x = seed;
    int i;
    for (i = 0; i < 4; i++) {
        rng->s[i] = bkr_splitmix32(&x);
    }
    if ((rng->s[0] | rng->s[1] | rng->s[2] | rng->s[3]) == 0) {
        rng->s[0] = 1;
    }
}

bkr_u32 bkr_rng_next(BkrRng* rng) {
    bkr_u32* s = rng->s;
    bkr_u32 result = bkr_rotl(s[1] * 5u, 7) * 9u;
    bkr_u32 t = s[1] << 9;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = bkr_rotl(s[3], 11);
    return result;
}

bkr_u32 bkr_rng_below(BkrRng* rng, bkr_u32 n) {
    bkr_u32 threshold = (0u - n) % n;
    for (;;) {
        bkr_u32 r = bkr_rng_next(rng);
        if (r >= threshold) {
            return r % n;
        }
    }
}

void bkr_rng_shuffle_u16(BkrRng* rng, bkr_u16* values, bkr_u32 count) {
    bkr_u32 i;
    if (count < 2) {
        return;
    }
    for (i = count - 1; i > 0; i--) {
        bkr_u32 j = bkr_rng_below(rng, i + 1);
        bkr_u16 tmp = values[i];
        values[i] = values[j];
        values[j] = tmp;
    }
}

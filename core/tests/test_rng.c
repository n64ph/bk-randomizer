/* test_rng.c — xoshiro128** + splitmix32: valores-ouro, limites e embaralhamento. */
#include "bkr_test.h"
#include "bkr/bkr_rng.h"

void test_rng_reference(void) {
    /* Valores conferidos com a implementação Python em tools/tests/test_rng_reference.py. */
    BkrRng rng;
    bkr_u32 x = 0;
    BKR_CHECK_EQ(bkr_splitmix32(&x), 0x92CA2F0Eu);
    BKR_CHECK_EQ(bkr_splitmix32(&x), 0x3CD6E3F3u);
    bkr_rng_seed(&rng, 12345u);
    BKR_CHECK_EQ(bkr_rng_next(&rng), 0x1EEA3CC1u);
    BKR_CHECK_EQ(bkr_rng_next(&rng), 0x1A40A62Eu);
    BKR_CHECK_EQ(bkr_rng_next(&rng), 0xFC4CD240u);
}

void test_rng_below_bounds(void) {
    BkrRng rng;
    bkr_u32 i;
    bkr_u32 seen[7] = { 0, 0, 0, 0, 0, 0, 0 };
    bkr_rng_seed(&rng, 7u);
    for (i = 0; i < 7000u; i++) {
        bkr_u32 v = bkr_rng_below(&rng, 7u);
        BKR_CHECK(v < 7u);
        seen[v]++;
    }
    for (i = 0; i < 7u; i++) {
        BKR_CHECK(seen[i] > 800u && seen[i] < 1200u);
    }
    BKR_CHECK_EQ(bkr_rng_below(&rng, 1u), 0u);
}

void test_shuffle_is_permutation(void) {
    BkrRng rng;
    bkr_u16 v[50];
    bkr_u32 count[50];
    bkr_u32 i;
    for (i = 0; i < 50u; i++) {
        v[i] = (bkr_u16)i;
        count[i] = 0;
    }
    bkr_rng_seed(&rng, 99u);
    bkr_rng_shuffle_u16(&rng, v, 50u);
    for (i = 0; i < 50u; i++) {
        BKR_CHECK(v[i] < 50u);
        count[v[i]]++;
    }
    for (i = 0; i < 50u; i++) {
        BKR_CHECK_EQ(count[i], 1u);
    }
    BKR_CHECK(v[0] != 0 || v[1] != 1 || v[2] != 2);
}

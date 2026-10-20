/*
 * mips_selftest.c — Ponto de entrada para rodar o núcleo compilado em MIPS
 * dentro de um emulador (tools/mips_selftest.py). Gera o seed "BANJO" com as
 * opções padrão e devolve o hash do placement em $v0.
 */
#include "bkr/bkr_generate.h"

static BkrGenResult g_selftest_result;
static BkrSeedSpec g_selftest_spec;

bkr_u32 bkr_mips_selftest(void) {
    bkr_settings_default(&g_selftest_spec.settings);
    bkr_seed_sanitize("BANJO", g_selftest_spec.seed);
    if (!bkr_generate(&g_selftest_spec, &g_selftest_result)) {
        return 0;
    }
    return bkr_placement_hash(g_selftest_result.placement);
}

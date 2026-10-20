/* test_spoiler.c — Spoiler em texto: cabeçalho, esferas e lista por mundo. */
#include "bkr_test.h"
#include "bkr/bkr_spoiler.h"
#include "bkr/bkr_ids.h"

static BkrGenResult s_result;
static char s_spoiler[96 * 1024];

static int contains(const char* text, const char* needle) {
    bkr_u32 n = bkr_strlen(needle);
    for (; *text != '\0'; text++) {
        bkr_u32 i = 0;
        while (i < n && text[i] == needle[i]) {
            i++;
        }
        if (i == n) {
            return 1;
        }
    }
    return 0;
}

void test_spoiler_text(void) {
    BkrSeedSpec spec;
    BkrText t;
    bkr_u32 spheres;
    bkr_settings_default(&spec.settings);
    bkr_seed_sanitize("BANJO", spec.seed);
    BKR_CHECK(bkr_generate(&spec, &s_result));
    bkr_text_init(&t, s_spoiler, sizeof(s_spoiler));
    spheres = bkr_spoiler_write(&spec, &s_result, BKR_LANG_PT_BR, &t);
    BKR_CHECK(spheres > 3u);
    BKR_CHECK(!t.overflow);
    BKR_CHECK(t.len > 1000u);
    BKR_CHECK(contains(s_spoiler, "Codigo: BKR1-GG000M-BANJO"));
    BKR_CHECK(contains(s_spoiler, "Impressao digital: 623A1517"));
    BKR_CHECK(contains(s_spoiler, "Esfera 1:"));
    BKR_CHECK(contains(s_spoiler, "== Localizacoes por mundo =="));
    BKR_CHECK(contains(s_spoiler, "Covil de Gruntilda:"));
    bkr_text_init(&t, s_spoiler, sizeof(s_spoiler));
    BKR_CHECK_EQ(bkr_spoiler_write(&spec, &s_result, BKR_LANG_EN, &t), spheres);
    BKR_CHECK(contains(s_spoiler, "Code: BKR1-GG000M-BANJO"));
    BKR_CHECK(contains(s_spoiler, "== Playthrough spheres =="));
}

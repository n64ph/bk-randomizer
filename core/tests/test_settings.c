/* test_settings.c — Opções, semente, código de compartilhamento e impressão digital. */
#include "bkr_test.h"
#include "bkr/bkr_settings.h"

void test_seed_sanitize(void) {
    char out[BKR_SEED_MAX + 1];
    BKR_CHECK_EQ(bkr_seed_sanitize("banjo kazooie!", out), 13u);
    BKR_CHECK(bkr_streq(out, "BANJO_KAZOOIE"));
    BKR_CHECK_EQ(bkr_seed_sanitize("@@@", out), 0u);
    BKR_CHECK(bkr_streq(out, ""));
    BKR_CHECK_EQ(bkr_seed_sanitize("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123", out), BKR_SEED_MAX);
    BKR_CHECK(bkr_streq(out, "ABCDEFGHIJKLMNOPQRSTUVWX"));
}

void test_settings_roundtrip(void) {
    BkrSettings s;
    BkrSettings back;
    bkr_u32 bits;
    int ok_count = 0;
    for (bits = 0; bits < 64u; bits++) {
        if (bkr_settings_unpack(bits, &s)) {
            ok_count++;
            BKR_CHECK_EQ(bkr_settings_pack(&s), bits);
            BKR_CHECK(bkr_settings_unpack(bkr_settings_pack(&s), &back));
            BKR_CHECK_EQ(back.shuffle, s.shuffle);
        }
    }
    /* 2 lógicas x 3 embaralhamentos x 2 escopos x 2 modos de movimento x 2 acessos. */
    BKR_CHECK_EQ(ok_count, 48);
    BKR_CHECK(!bkr_settings_unpack(64u, &s));
    bkr_settings_default(&s);
    BKR_CHECK(bkr_settings_valid(&s));
    BKR_CHECK_EQ(bkr_settings_pack(&s), 20u);
}

void test_share_code(void) {
    BkrSeedSpec spec;
    BkrSeedSpec back;
    char code[BKR_SHARE_CODE_MAX];
    bkr_settings_default(&spec.settings);
    bkr_seed_sanitize("Banjo", spec.seed);
    BKR_CHECK(bkr_share_code_format(&spec, code, sizeof(code)));
    BKR_CHECK(bkr_streq(code, "BKR1-GG000M-BANJO"));
    BKR_CHECK(bkr_share_code_parse(code, &back));
    BKR_CHECK(bkr_streq(back.seed, "BANJO"));
    BKR_CHECK_EQ(bkr_settings_pack(&back.settings), bkr_settings_pack(&spec.settings));
    BKR_CHECK(bkr_share_code_parse("bkr1-gg000m-banjo", &back));
    BKR_CHECK(bkr_share_code_parse("BKR1-GGOOOM-BANJO", &back)); /* O lido como 0 */
    BKR_CHECK(!bkr_share_code_parse("BKR2-GG000M-BANJO", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1-000000-BANJO", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1-GG000M-", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1-GG000MBANJO", &back));
    BKR_CHECK(!bkr_share_code_parse("BKR1", &back));
}

void test_fingerprint_changes(void) {
    BkrSeedSpec a;
    BkrSeedSpec b;
    bkr_settings_default(&a.settings);
    bkr_settings_default(&b.settings);
    bkr_seed_sanitize("BANJO", a.seed);
    bkr_seed_sanitize("BANJO", b.seed);
    BKR_CHECK_EQ(bkr_seed_fingerprint(&a), bkr_seed_fingerprint(&b));
    b.settings.scope = BKR_SCOPE_WORLD;
    BKR_CHECK(bkr_seed_fingerprint(&a) != bkr_seed_fingerprint(&b));
    bkr_settings_default(&b.settings);
    bkr_seed_sanitize("BANJO2", b.seed);
    BKR_CHECK(bkr_seed_fingerprint(&a) != bkr_seed_fingerprint(&b));
    /* Valor-ouro da semente BANJO com as opções padrão (vira o nome do save). */
    BKR_CHECK_EQ(bkr_seed_fingerprint(&a), 0x623A1517u);
}

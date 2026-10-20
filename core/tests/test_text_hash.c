/* test_text_hash.c — Utilidades sem libc: strings, palavras, texto e FNV-1a. */
#include "bkr_test.h"
#include "bkr/bkr_hash.h"
#include "bkr/bkr_text.h"

void test_strings_and_words(void) {
    bkr_u32 a[3] = { 1u, 2u, 3u };
    bkr_u32 b[3] = { 9u, 9u, 9u };
    BKR_CHECK_EQ(bkr_strlen(""), 0u);
    BKR_CHECK_EQ(bkr_strlen("Banjo"), 5u);
    BKR_CHECK(bkr_streq("Kazooie", "Kazooie"));
    BKR_CHECK(!bkr_streq("Kazooie", "Kazoo"));
    BKR_CHECK(!bkr_streq("", "x"));
    bkr_words_copy(b, a, 3u);
    BKR_CHECK_EQ(b[0] + b[1] + b[2], 6u);
    bkr_words_clear(b, 2u);
    BKR_CHECK_EQ(b[0], 0u);
    BKR_CHECK_EQ(b[1], 0u);
    BKR_CHECK_EQ(b[2], 3u);
    BKR_BIT_SET(a, 40u);
    BKR_CHECK_EQ(BKR_BIT_GET(a, 40u), 1u);
    BKR_CHECK_EQ(a[1], 2u | (1u << 8));
    BKR_BIT_CLR(a, 40u);
    BKR_CHECK_EQ(a[1], 2u);
}

void test_fnv_vectors(void) {
    static const bkr_u8 bytes[3] = { 'f', 'o', 'o' };
    BKR_CHECK_EQ(bkr_fnv1a_str(BKR_FNV_OFFSET, ""), 0x811C9DC5u);
    BKR_CHECK_EQ(bkr_fnv1a_str(BKR_FNV_OFFSET, "a"), 0xE40C292Cu);
    BKR_CHECK_EQ(bkr_fnv1a_str(BKR_FNV_OFFSET, "foobar"), 0xBF9CF968u);
    BKR_CHECK_EQ(bkr_fnv1a_bytes(BKR_FNV_OFFSET, bytes, 3u), bkr_fnv1a_str(BKR_FNV_OFFSET, "foo"));
    /* fnv1a_u32 mistura em little-endian: igual ao FNV dos bytes 01 00 00 00. */
    BKR_CHECK_EQ(bkr_fnv1a_u32(BKR_FNV_OFFSET, 1u), 0xFB69B604u);
}

void test_text_builder(void) {
    char buf[16];
    BkrText t;
    bkr_text_init(&t, buf, sizeof(buf));
    bkr_text_put(&t, "N=");
    bkr_text_u32(&t, 4096u);
    bkr_text_putc(&t, ' ');
    bkr_text_hex(&t, 0xBEEFu, 4);
    BKR_CHECK(bkr_streq(buf, "N=4096 BEEF"));
    BKR_CHECK(!t.overflow);
    bkr_text_put(&t, "estoura o buffer");
    BKR_CHECK(t.overflow);
    BKR_CHECK_EQ(bkr_strlen(buf), 15u);
    bkr_text_init(&t, buf, sizeof(buf));
    bkr_text_u32(&t, 0u);
    bkr_text_putc(&t, '/');
    bkr_text_u32(&t, 4294967295u);
    BKR_CHECK(bkr_streq(buf, "0/4294967295"));
}

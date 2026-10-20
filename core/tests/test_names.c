/* test_names.c — Nomes de itens e mundos (PT-BR/EN) e palavras do hash. */
#include "bkr_test.h"
#include "bkr/bkr_ids.h"

void test_item_names(void) {
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_JIGGY, BKR_LANG_PT_BR), "Jiggy"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_HONEYCOMB, BKR_LANG_PT_BR), "Favo de Mel Vazio"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_HONEYCOMB, BKR_LANG_EN), "Empty Honeycomb"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_TOKEN, BKR_LANG_EN), "Mumbo Token"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_MOVE(BKR_ABILITY_TALON_TROT), BKR_LANG_PT_BR), "Trote de Garras"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_MOVE(BKR_ABILITY_BEAK_BUSTER), BKR_LANG_EN), "Beak Buster"));
    BKR_CHECK(bkr_streq(bkr_item_name(BKR_ITEM_NONE, BKR_LANG_PT_BR), "Nada"));
    BKR_CHECK(bkr_streq(bkr_item_name(0x7F, BKR_LANG_EN), "Nothing"));
    BKR_CHECK(BKR_ITEM_IS_MOVE(BKR_ITEM_MOVE(BKR_ABILITY_NOTE_DOORS)));
    BKR_CHECK(!BKR_ITEM_IS_MOVE(BKR_ITEM_MOVE_BASE + BKR_ABILITY_COUNT));
}

void test_world_names(void) {
    BKR_CHECK(bkr_streq(bkr_world_name(BKR_WORLD_LAIR, BKR_LANG_PT_BR), "Covil de Gruntilda"));
    BKR_CHECK(bkr_streq(bkr_world_name(BKR_WORLD_LAIR, BKR_LANG_EN), "Gruntilda's Lair"));
    BKR_CHECK(bkr_streq(bkr_world_name(BKR_WORLD_MMM, BKR_LANG_EN), "Mad Monster Mansion"));
    BKR_CHECK(bkr_streq(bkr_world_name(200, BKR_LANG_PT_BR), "Nenhum"));
}

void test_hash_words(void) {
    BKR_CHECK(bkr_streq(bkr_hash_word(0u), "Banjo"));
    BKR_CHECK(bkr_streq(bkr_hash_word(31u), "Dingpot"));
    BKR_CHECK(bkr_streq(bkr_hash_word(32u), "Banjo"));
}

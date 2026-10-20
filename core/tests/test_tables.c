/* test_tables.c — Invariantes das tabelas geradas por tools/compile_logic.py. */
#include "bkr_test.h"
#include "bkr/bkr_world.h"
#include "bkr/bkr_ids.h"

void test_tables_consistent(void) {
    bkr_u32 r;
    bkr_u32 i;
    bkr_u32 kinds[5] = { 0, 0, 0, 0, 0 };
    bkr_u32 notes = 0;
    bkr_u32 next_loc = 0;
    for (r = 0; r < BKR_REGION_COUNT; r++) {
        const BkrRegionDef* reg = &bkr_regions[r];
        BKR_CHECK(reg->exit_first + reg->exit_count <= BKR_EXIT_COUNT);
        BKR_CHECK(reg->event_first + reg->event_count <= BKR_EVENT_DEF_COUNT);
        BKR_CHECK(reg->group_first + reg->group_count <= BKR_NOTE_GROUP_COUNT);
        BKR_CHECK_EQ(reg->loc_first, next_loc);
        next_loc += reg->loc_count;
        for (i = reg->loc_first; i < (bkr_u32)reg->loc_first + reg->loc_count; i++) {
            BKR_CHECK_EQ(bkr_locations[i].region, r);
        }
    }
    BKR_CHECK_EQ(next_loc, BKR_LOCATION_COUNT);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        BKR_CHECK(bkr_locations[i].kind >= BKR_LOC_JIGGY && bkr_locations[i].kind <= BKR_LOC_MOLEHILL);
        kinds[bkr_locations[i].kind]++;
        BKR_CHECK(bkr_locations[i].rule < BKR_RULE_CODE_SIZE);
        BKR_CHECK(bkr_locations[i].world < BKR_WORLD_COUNT);
    }
    BKR_CHECK_EQ(kinds[BKR_LOC_JIGGY], 100u);
    BKR_CHECK_EQ(kinds[BKR_LOC_HONEYCOMB], 24u);
    BKR_CHECK_EQ(kinds[BKR_LOC_TOKEN], 116u);
    BKR_CHECK_EQ(kinds[BKR_LOC_MOLEHILL], 9u);
    for (i = 0; i < BKR_EXIT_COUNT; i++) {
        BKR_CHECK(bkr_exits[i].to < BKR_REGION_COUNT);
        BKR_CHECK(bkr_exits[i].rule < BKR_RULE_CODE_SIZE);
    }
    for (i = 0; i < BKR_EVENT_DEF_COUNT; i++) {
        BKR_CHECK(bkr_event_defs[i].event < BKR_EVENT_COUNT);
    }
    for (i = 0; i < BKR_NOTE_GROUP_COUNT; i++) {
        notes += bkr_note_groups[i].count;
    }
    BKR_CHECK_EQ(notes, 900u);
    BKR_CHECK(bkr_streq(bkr_region_names[BKR_START_REGION], "SPIRAL_MOUNTAIN_ENTRANCE"));
    BKR_CHECK(bkr_streq(bkr_event_names[BKR_GOAL_EVENT], "GAME_COMPLETE"));
}

void test_lookup_tables(void) {
    bkr_u32 id;
    BKR_CHECK_EQ(bkr_loc_by_jiggy[0], BKR_LOC_INVALID);
    for (id = 1; id <= 100u; id++) {
        bkr_u16 loc = bkr_loc_by_jiggy[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK_EQ(bkr_locations[loc].kind, BKR_LOC_JIGGY);
        BKR_CHECK_EQ(bkr_locations[loc].game_id, id);
    }
    for (id = 1; id <= 24u; id++) {
        bkr_u16 loc = bkr_loc_by_honeycomb[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK_EQ(bkr_locations[loc].kind, BKR_LOC_HONEYCOMB);
        BKR_CHECK_EQ(bkr_locations[loc].game_id, id);
    }
    for (id = 1; id <= 116u; id++) {
        bkr_u16 loc = bkr_loc_by_token[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK_EQ(bkr_locations[loc].kind, BKR_LOC_TOKEN);
        BKR_CHECK_EQ(bkr_locations[loc].game_id, id);
    }
    for (id = 117; id < 128u; id++) {
        BKR_CHECK_EQ(bkr_loc_by_token[id], BKR_LOC_INVALID);
    }
    for (id = 0; id < 9u; id++) {
        bkr_u16 loc = bkr_loc_by_mole[id];
        BKR_CHECK(loc < BKR_LOCATION_COUNT);
        BKR_CHECK(BKR_ITEM_IS_MOVE(bkr_locations[loc].vanilla_item));
    }
    BKR_CHECK_EQ(bkr_loc_by_mole[9], BKR_LOC_INVALID);
    /* Ids efetivos no Banjo: Recompiled para os tokens que o jogo original duplicava. */
    BKR_CHECK(bkr_streq(bkr_location_names[bkr_loc_by_token[116]], "MMM Token - Inside Loggo"));
    BKR_CHECK(bkr_streq(bkr_location_names[bkr_loc_by_token[93]], "CCW Token - Spring Thorn Field Front Of Mumbos Skull"));
    /* Molehill 1 de moleTable ensina Disparo de Ovos em Mumbo's Mountain. */
    BKR_CHECK(bkr_streq(bkr_location_names[bkr_loc_by_mole[1]], "MM Molehill - Use Eggs"));
    BKR_CHECK_EQ(bkr_locations[bkr_loc_by_mole[1]].vanilla_item, BKR_ITEM_MOVE(BKR_ABILITY_EGGS));
}

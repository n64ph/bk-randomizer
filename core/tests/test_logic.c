/* test_logic.c — Estado lógico, avaliador de regras RPN e busca por ponto fixo. */
#include "bkr_test.h"
#include "bkr/bkr_logic.h"
#include "bkr/bkr_rules.h"
#include "bkr/bkr_ids.h"

static BkrSearch t_search;
static BkrState t_state;
static BkrState t_copy;
static BkrCosts t_costs;
static bkr_u8 t_placement[BKR_LOCATION_COUNT];

void test_state_items(void) {
    bkr_state_clear(&t_state);
    bkr_state_add_item(&t_state, BKR_ITEM_JIGGY);
    bkr_state_add_item(&t_state, BKR_ITEM_JIGGY);
    bkr_state_add_item(&t_state, BKR_ITEM_TOKEN);
    bkr_state_add_item(&t_state, BKR_ITEM_HONEYCOMB);
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_FLIGHT));
    BKR_CHECK_EQ(t_state.jiggies, 2u);
    BKR_CHECK_EQ(t_state.tokens, 1u);
    BKR_CHECK_EQ(t_state.honeycombs, 1u);
    BKR_CHECK_EQ(t_state.moves, 1u << BKR_ABILITY_FLIGHT);
    bkr_state_copy(&t_copy, &t_state);
    bkr_state_remove_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_FLIGHT));
    bkr_state_remove_item(&t_state, BKR_ITEM_TOKEN);
    bkr_state_remove_item(&t_state, BKR_ITEM_TOKEN); /* não fica negativo */
    BKR_CHECK_EQ(t_state.moves, 0u);
    BKR_CHECK_EQ(t_state.tokens, 0u);
    BKR_CHECK_EQ(t_copy.tokens, 1u);
    BKR_CHECK_EQ(t_copy.moves, 1u << BKR_ABILITY_FLIGHT);
    bkr_state_base(&t_state);
    BKR_CHECK_EQ(t_state.moves, BKR_STARTING_MOVES);
    BKR_CHECK(t_state.moves & (1u << BKR_ABILITY_CLIMB));
    BKR_CHECK(!(t_state.moves & (1u << BKR_ABILITY_EGGS)));
}

void test_rule_eval_handmade(void) {
    static const bkr_u16 eggs_and_flip[] = { BKR_OP_HAS, BKR_ABILITY_EGGS, BKR_OP_HAS, BKR_ABILITY_FLAP_FLIP, BKR_OP_AND, BKR_OP_END };
    static const bkr_u16 not_talon_or_tokens[] = { BKR_OP_HAS, BKR_ABILITY_TALON_TROT, BKR_OP_NOT, BKR_OP_TOKENS, 10, BKR_OP_OR, BKR_OP_END };
    static const bkr_u16 world_ccw[] = { BKR_OP_WORLD_COST, 8, BKR_OP_END };
    static const bkr_u16 door_180[] = { BKR_OP_NOTE_DOOR, 1, BKR_OP_END };
    static const bkr_u16 bee[] = { BKR_OP_TRANSFORM_COST, BKR_TRANSFORM_BEE, BKR_OP_END };
    static const bkr_u16 empty_rule[] = { BKR_OP_END };
    static const bkr_u16 bad_opcode[] = { 99, BKR_OP_END };
    bkr_costs_vanilla(&t_costs);
    bkr_state_clear(&t_state);
    BKR_CHECK(bkr_rule_eval_code(empty_rule, &t_state, &t_costs));
    BKR_CHECK(!bkr_rule_eval_code(bad_opcode, &t_state, &t_costs));
    BKR_CHECK(!bkr_rule_eval_code(eggs_and_flip, &t_state, &t_costs));
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_EGGS));
    BKR_CHECK(!bkr_rule_eval_code(eggs_and_flip, &t_state, &t_costs));
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_FLAP_FLIP));
    BKR_CHECK(bkr_rule_eval_code(eggs_and_flip, &t_state, &t_costs));
    BKR_CHECK(bkr_rule_eval_code(not_talon_or_tokens, &t_state, &t_costs));
    bkr_state_add_item(&t_state, BKR_ITEM_MOVE(BKR_ABILITY_TALON_TROT));
    BKR_CHECK(!bkr_rule_eval_code(not_talon_or_tokens, &t_state, &t_costs));
    t_state.tokens = 10;
    BKR_CHECK(bkr_rule_eval_code(not_talon_or_tokens, &t_state, &t_costs));
    /* Slot 8 = CCW: 1+2+5+7+8+9+10+12+15 = 69 jiggies acumulados. */
    t_state.jiggies = 68;
    BKR_CHECK(!bkr_rule_eval_code(world_ccw, &t_state, &t_costs));
    t_state.jiggies = 69;
    BKR_CHECK(bkr_rule_eval_code(world_ccw, &t_state, &t_costs));
    t_state.notes = 179;
    BKR_CHECK(!bkr_rule_eval_code(door_180, &t_state, &t_costs));
    t_state.notes = 180;
    BKR_CHECK(bkr_rule_eval_code(door_180, &t_state, &t_costs));
    /* Abelha = 5+10+15+20+25 = 75 tokens acumulados. */
    t_state.tokens = 74;
    BKR_CHECK(!bkr_rule_eval_code(bee, &t_state, &t_costs));
    t_state.tokens = 75;
    BKR_CHECK(bkr_rule_eval_code(bee, &t_state, &t_costs));
}

void test_vanilla_placement_beats_game(void) {
    bkr_u32 i;
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        t_placement[i] = bkr_locations[i].vanilla_item;
    }
    bkr_costs_vanilla(&t_costs);
    bkr_state_base(&t_state);
    bkr_search_init(&t_search, &t_state);
    bkr_search_sweep(&t_search, &t_costs, t_placement, BKR_SWEEP_COLLECT);
    BKR_CHECK(BKR_BIT_GET(t_search.st.events, BKR_GOAL_EVENT));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        BKR_CHECK(BKR_BIT_GET(t_search.loc_done, i));
    }
    BKR_CHECK_EQ(t_search.st.jiggies, 100u);
    BKR_CHECK_EQ(t_search.st.tokens, 116u);
    BKR_CHECK_EQ(t_search.st.honeycombs, 24u);
}

void test_all_items_reach_everything(void) {
    bkr_u32 i;
    bkr_costs_vanilla(&t_costs);
    bkr_state_base(&t_state);
    t_state.moves = 0xFFFFFu;
    t_state.jiggies = 100;
    t_state.tokens = 116;
    bkr_search_init(&t_search, &t_state);
    bkr_search_sweep(&t_search, &t_costs, BKR_NULL, BKR_SWEEP_COLLECT);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (!BKR_BIT_GET(t_search.loc_done, i)) {
            printf("  inalcançável com tudo: %s\n", bkr_location_names[i]);
        }
        BKR_CHECK(BKR_BIT_GET(t_search.loc_done, i));
    }
    BKR_CHECK(BKR_BIT_GET(t_search.st.events, BKR_GOAL_EVENT));
    BKR_CHECK_EQ(t_search.st.notes, 900u);
}

void test_logic_actually_gates(void) {
    bkr_u32 i;
    bkr_u32 reached = 0;
    bkr_costs_vanilla(&t_costs);
    bkr_state_base(&t_state);
    bkr_search_init(&t_search, &t_state);
    bkr_search_sweep(&t_search, &t_costs, BKR_NULL, BKR_SWEEP_COLLECT);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        reached += BKR_BIT_GET(t_search.loc_done, i);
    }
    /* Sem itens: só Spiral Mountain e o começo do covil. */
    BKR_CHECK(reached > 0u && reached < 40u);
    BKR_CHECK(!BKR_BIT_GET(t_search.st.events, BKR_GOAL_EVENT));
}

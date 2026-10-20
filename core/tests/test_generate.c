/* test_generate.c — Geração: sucesso, determinismo, restrições e propriedades. */
#include "bkr_test.h"
#include "bkr/bkr_generate.h"
#include "bkr/bkr_text.h"
#include "bkr/bkr_ids.h"

/* O valor-ouro vem do core/Makefile (PLACEMENT_GOLDEN), fonte única
 * compartilhada com o autoteste MIPS. */
#ifndef BKR_PLACEMENT_GOLDEN
#error "BKR_PLACEMENT_GOLDEN não definido: compile os testes com make -C core test"
#endif
#ifndef BKR_PROPERTY_SEEDS
#define BKR_PROPERTY_SEEDS 100
#endif

static BkrGenResult g_a;
static BkrGenResult g_b;
static bkr_u8 g_vanilla[BKR_LOCATION_COUNT];

static void make_spec(BkrSeedSpec* spec, const char* seed) {
    bkr_settings_default(&spec->settings);
    bkr_seed_sanitize(seed, spec->seed);
}

static int same_placement(const BkrGenResult* a, const BkrGenResult* b) {
    bkr_u32 i;
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (a->placement[i] != b->placement[i]) {
            return 0;
        }
    }
    return 1;
}

/* Mesmo multiconjunto de itens do vanilla; localizações travadas intactas. */
static void check_balance(const BkrSettings* s, const BkrGenResult* r) {
    bkr_u32 want[0x40];
    bkr_u32 got[0x40];
    bkr_u32 i;
    for (i = 0; i < 0x40u; i++) {
        want[i] = 0;
        got[i] = 0;
    }
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        want[bkr_locations[i].vanilla_item]++;
        got[r->placement[i]]++;
        BKR_CHECK(r->placement[i] != BKR_ITEM_NONE);
        if (!bkr_location_is_shuffled(s, i)) {
            BKR_CHECK_EQ(r->placement[i], bkr_locations[i].vanilla_item);
        }
    }
    for (i = 0; i < 0x40u; i++) {
        BKR_CHECK_EQ(got[i], want[i]);
    }
}

void test_vanilla_world_verifies(void) {
    BkrSettings s;
    bkr_u32 i;
    bkr_settings_default(&s);
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        g_vanilla[i] = bkr_locations[i].vanilla_item;
    }
    BKR_CHECK(bkr_verify(&s, g_vanilla));
}

void test_generate_default(void) {
    BkrSeedSpec spec;
    make_spec(&spec, "BANJO");
    BKR_CHECK(bkr_generate(&spec, &g_a));
    BKR_CHECK(g_a.ok);
    BKR_CHECK(bkr_verify(&spec.settings, g_a.placement));
    check_balance(&spec.settings, &g_a);
    BKR_CHECK(bkr_generate(&spec, &g_b));
    BKR_CHECK(same_placement(&g_a, &g_b));
    BKR_CHECK_EQ(g_a.fingerprint, g_b.fingerprint);
    BKR_CHECK_EQ(g_a.fingerprint, 0x623A1517u);
    /* Valor-ouro: muda só quando o algoritmo ou os dados de lógica mudam
     * (e então BKR_GENERATOR_VERSION deve subir junto). */
    BKR_CHECK_EQ(bkr_placement_hash(g_a.placement), BKR_PLACEMENT_GOLDEN);
}

void test_generate_rejects_invalid(void) {
    BkrSeedSpec spec;
    make_spec(&spec, "@@@");
    BKR_CHECK(!bkr_generate(&spec, &g_a));
    BKR_CHECK(!g_a.ok);
    make_spec(&spec, "BANJO");
    spec.settings.shuffle = 7;
    BKR_CHECK(!bkr_generate(&spec, &g_a));
}

void test_generate_seed_changes_result(void) {
    BkrSeedSpec spec;
    make_spec(&spec, "BANJO");
    BKR_CHECK(bkr_generate(&spec, &g_a));
    make_spec(&spec, "KAZOOIE");
    BKR_CHECK(bkr_generate(&spec, &g_b));
    BKR_CHECK(!same_placement(&g_a, &g_b));
}

void test_molehills_hold_moves(void) {
    BkrSeedSpec spec;
    bkr_u32 i;
    make_spec(&spec, "MOLES");
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        int is_mole = bkr_locations[i].kind == BKR_LOC_MOLEHILL;
        BKR_CHECK_EQ(is_mole, BKR_ITEM_IS_MOVE(g_a.placement[i]) ? 1 : 0);
    }
}

void test_by_type_keeps_kinds(void) {
    BkrSeedSpec spec;
    bkr_u32 i;
    make_spec(&spec, "TYPES");
    spec.settings.shuffle = BKR_SHUFFLE_BY_TYPE;
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (bkr_locations[i].kind != BKR_LOC_MOLEHILL) {
            BKR_CHECK_EQ(g_a.placement[i], bkr_locations[i].vanilla_item);
        }
    }
}

void test_world_scope_keeps_worlds(void) {
    BkrSeedSpec spec;
    bkr_u32 per_world[BKR_WORLD_COUNT][4];
    bkr_u32 w;
    bkr_u32 k;
    bkr_u32 i;
    make_spec(&spec, "WORLDS");
    spec.settings.scope = BKR_SCOPE_WORLD;
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (w = 0; w < BKR_WORLD_COUNT; w++) {
        for (k = 0; k < 4; k++) {
            per_world[w][k] = 0;
        }
    }
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        bkr_u8 item = g_a.placement[i];
        bkr_u8 van = bkr_locations[i].vanilla_item;
        if (item < 4) {
            per_world[bkr_locations[i].world][item]++;
        }
        if (van < 4) {
            per_world[bkr_locations[i].world][van]--;
        }
    }
    for (w = 0; w < BKR_WORLD_COUNT; w++) {
        for (k = 0; k < 4; k++) {
            BKR_CHECK_EQ(per_world[w][k], 0u);
        }
    }
}

void test_vanilla_settings_reproduce_vanilla(void) {
    BkrSeedSpec spec;
    bkr_u32 i;
    make_spec(&spec, "VANILLA");
    spec.settings.shuffle = BKR_SHUFFLE_VANILLA;
    spec.settings.moves = BKR_MOVES_VANILLA;
    BKR_CHECK(bkr_generate(&spec, &g_a));
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        BKR_CHECK_EQ(g_a.placement[i], bkr_locations[i].vanilla_item);
    }
}

void test_property_many_seeds(void) {
    static const bkr_u8 presets[4][5] = {
        /* logic, shuffle, scope, moves, access */
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_MIXED, BKR_SCOPE_GAME, BKR_MOVES_MOLEHILLS, BKR_ACCESS_ALL },
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_BY_TYPE, BKR_SCOPE_GAME, BKR_MOVES_MOLEHILLS, BKR_ACCESS_ALL },
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_MIXED, BKR_SCOPE_WORLD, BKR_MOVES_MOLEHILLS, BKR_ACCESS_ALL },
        { BKR_LOGIC_GLITCHLESS, BKR_SHUFFLE_MIXED, BKR_SCOPE_GAME, BKR_MOVES_MOLEHILLS, BKR_ACCESS_BEATABLE },
    };
    bkr_u32 p;
    for (p = 0; p < 4u; p++) {
        bkr_u32 n;
        bkr_u32 max_attempts = 0;
        bkr_u32 failures = 0;
        for (n = 0; n < BKR_PROPERTY_SEEDS; n++) {
            BkrSeedSpec spec;
            char seed[16];
            BkrText t;
            bkr_text_init(&t, seed, sizeof(seed));
            bkr_text_put(&t, "P");
            bkr_text_u32(&t, p);
            bkr_text_putc(&t, '_');
            bkr_text_u32(&t, n);
            make_spec(&spec, seed);
            spec.settings.logic = presets[p][0];
            spec.settings.shuffle = presets[p][1];
            spec.settings.scope = presets[p][2];
            spec.settings.moves = presets[p][3];
            spec.settings.access = presets[p][4];
            if (!bkr_generate(&spec, &g_a) || !bkr_verify(&spec.settings, g_a.placement)) {
                failures++;
                continue;
            }
            check_balance(&spec.settings, &g_a);
            if (g_a.attempts > max_attempts) {
                max_attempts = g_a.attempts;
            }
        }
        printf("  preset %u: %u seeds, falhas=%u, tentativas máx=%u\n", p, (bkr_u32)BKR_PROPERTY_SEEDS, failures,
               max_attempts);
        BKR_CHECK_EQ(failures, 0u);
    }
}

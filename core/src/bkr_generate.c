/*
 * bkr_generate.c — Geração determinística do seed.
 *
 * Algoritmo (adaptado de OoT-Randomizer, Fill.py::distribute_items_restrictive
 * e fill_restrictive):
 *  1. localizações embaralhadas formam o pool com seus itens vanilla;
 *  2. itens de progressão (movimentos, jiggies, tokens) são colocados com
 *     preenchimento assumido: para cada item, assume-se que todos os itens
 *     ainda não colocados já foram coletados, varre-se o mundo e o item vai
 *     para uma localização vazia alcançável sem ele;
 *  3. o resto (favos) é colocado ao acaso;
 *  4. o resultado é verificado (vencível e, se pedido, 100% alcançável).
 * Toda memória de trabalho é estática (pilha do jogo é pequena).
 */
#include "bkr/bkr_generate.h"
#include "bkr/bkr_rng.h"
#include "bkr/bkr_hash.h"
#include "bkr/bkr_ids.h"

typedef struct BkrItemInst {
    bkr_u8 item;
    bkr_u8 world;        /* mundo de origem (escopo "por mundo") */
    bkr_u8 origin_kind;  /* tipo da localização de origem (modo "por tipo") */
    bkr_u8 pad;
} BkrItemInst;

static BkrItemInst g_pool[BKR_LOCATION_COUNT];
static bkr_u16 g_prog[BKR_LOCATION_COUNT];
static bkr_u16 g_rest[BKR_LOCATION_COUNT];
static bkr_u16 g_cand[BKR_LOCATION_COUNT];
static BkrSearch g_search;
static BkrState g_assumed;
static BkrState g_base;
static BkrCosts g_costs;

int bkr_location_is_shuffled(const BkrSettings* s, bkr_u32 loc) {
    if (bkr_locations[loc].kind == BKR_LOC_MOLEHILL) {
        return s->moves != BKR_MOVES_VANILLA;
    }
    return s->shuffle != BKR_SHUFFLE_VANILLA;
}

static int bkr_is_progression(bkr_u8 item) {
    return BKR_ITEM_IS_MOVE(item) || item == BKR_ITEM_JIGGY || item == BKR_ITEM_TOKEN;
}

static int bkr_allowed(const BkrSettings* s, const BkrItemInst* it, const BkrLocDef* loc) {
    int is_move = BKR_ITEM_IS_MOVE(it->item);
    if (loc->kind == BKR_LOC_MOLEHILL) {
        if (!is_move) {
            return 0;
        }
    } else {
        if (is_move) {
            return 0;
        }
        if (s->shuffle == BKR_SHUFFLE_BY_TYPE && it->origin_kind != loc->kind) {
            return 0;
        }
    }
    if (s->scope == BKR_SCOPE_WORLD && it->world != loc->world) {
        return 0;
    }
    return 1;
}

int bkr_verify(const BkrSettings* s, const bkr_u8* placement) {
    bkr_u32 i;
    bkr_costs_vanilla(&g_costs);
    bkr_state_base(&g_base);
    bkr_search_init(&g_search, &g_base);
    bkr_search_sweep(&g_search, &g_costs, placement, BKR_SWEEP_COLLECT);
    if (!BKR_BIT_GET(g_search.st.events, BKR_GOAL_EVENT)) {
        return 0;
    }
    if (s->access == BKR_ACCESS_ALL) {
        for (i = 0; i < BKR_LOCATION_COUNT; i++) {
            if (!BKR_BIT_GET(g_search.loc_done, i)) {
                return 0;
            }
        }
    }
    return 1;
}

/* Uma tentativa de geração. Retorna 1 se produziu um placement válido. */
static int bkr_attempt(const BkrSettings* s, BkrRng* rng, bkr_u8* placement) {
    bkr_u32 pool_n = 0;
    bkr_u32 prog_n = 0;
    bkr_u32 rest_n = 0;
    bkr_u32 moves_n = 0;
    bkr_u32 i;
    bkr_u32 k;

    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        const BkrLocDef* loc = &bkr_locations[i];
        if (bkr_location_is_shuffled(s, i)) {
            g_pool[pool_n].item = loc->vanilla_item;
            g_pool[pool_n].world = loc->world;
            g_pool[pool_n].origin_kind = loc->kind;
            g_pool[pool_n].pad = 0;
            pool_n++;
            placement[i] = BKR_ITEM_NONE;
        } else {
            placement[i] = loc->vanilla_item;
        }
    }
    for (i = 0; i < pool_n; i++) {
        if (bkr_is_progression(g_pool[i].item)) {
            g_prog[prog_n++] = (bkr_u16)i;
        } else {
            g_rest[rest_n++] = (bkr_u16)i;
        }
    }
    bkr_rng_shuffle_u16(rng, g_prog, prog_n);
    bkr_rng_shuffle_u16(rng, g_rest, rest_n);

    /* Movimentos primeiro (mais restritos), preservando a ordem sorteada. */
    for (i = 0; i < prog_n; i++) {
        if (BKR_ITEM_IS_MOVE(g_pool[g_prog[i]].item)) {
            bkr_u16 v = g_prog[i];
            for (k = i; k > moves_n; k--) {
                g_prog[k] = g_prog[k - 1];
            }
            g_prog[moves_n++] = v;
        }
    }

    bkr_state_base(&g_assumed);
    for (i = 0; i < prog_n; i++) {
        bkr_state_add_item(&g_assumed, g_pool[g_prog[i]].item);
    }

    for (i = 0; i < prog_n; i++) {
        const BkrItemInst* it = &g_pool[g_prog[i]];
        bkr_u32 cand_n = 0;
        bkr_state_remove_item(&g_assumed, it->item);
        if (s->logic != BKR_LOGIC_NONE) {
            bkr_search_init(&g_search, &g_assumed);
            bkr_search_sweep(&g_search, &g_costs, placement, BKR_SWEEP_COLLECT);
        }
        for (k = 0; k < BKR_LOCATION_COUNT; k++) {
            if (placement[k] != BKR_ITEM_NONE || !bkr_location_is_shuffled(s, k)) {
                continue;
            }
            if (!bkr_allowed(s, it, &bkr_locations[k])) {
                continue;
            }
            if (s->logic != BKR_LOGIC_NONE && !BKR_BIT_GET(g_search.loc_done, k)) {
                continue;
            }
            g_cand[cand_n++] = (bkr_u16)k;
        }
        if (cand_n == 0) {
            return 0;
        }
        placement[g_cand[bkr_rng_below(rng, cand_n)]] = it->item;
    }

    for (i = 0; i < rest_n; i++) {
        const BkrItemInst* it = &g_pool[g_rest[i]];
        bkr_u32 cand_n = 0;
        for (k = 0; k < BKR_LOCATION_COUNT; k++) {
            if (placement[k] == BKR_ITEM_NONE && bkr_location_is_shuffled(s, k) &&
                bkr_allowed(s, it, &bkr_locations[k])) {
                g_cand[cand_n++] = (bkr_u16)k;
            }
        }
        if (cand_n == 0) {
            return 0;
        }
        placement[g_cand[bkr_rng_below(rng, cand_n)]] = it->item;
    }

    if (s->logic == BKR_LOGIC_NONE) {
        return 1;
    }
    return bkr_verify(s, placement);
}

int bkr_generate(const BkrSeedSpec* spec, BkrGenResult* out) {
    BkrRng rng;
    bkr_u32 attempt;
    out->ok = 0;
    out->attempts = 0;
    out->fingerprint = bkr_seed_fingerprint(spec);
    if (!bkr_settings_valid(&spec->settings) || spec->seed[0] == '\0') {
        return 0;
    }
    bkr_costs_vanilla(&g_costs);
    bkr_rng_seed(&rng, out->fingerprint);
    for (attempt = 1; attempt <= BKR_MAX_ATTEMPTS; attempt++) {
        out->attempts = attempt;
        if (bkr_attempt(&spec->settings, &rng, out->placement)) {
            out->ok = 1;
            return 1;
        }
    }
    return 0;
}

bkr_u32 bkr_placement_hash(const bkr_u8* placement) {
    return bkr_fnv1a_bytes(BKR_FNV_OFFSET, placement, BKR_LOCATION_COUNT);
}

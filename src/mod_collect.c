/*
 * mod_collect.c — Detecta a coleta de jiggies, favos vazios e tokens e troca
 * o item recebido pelo item sorteado para a localização.
 *
 * Fluxo vanilla, tudo dentro da mesma chamada (src/core2/ba/marker.c e src/core2/code_59A80.c):
 *   jiggy:  jiggyscore_setCollected(id,1) -> item_adjustByDiffWithoutHud(ITEM_26_JIGGY_TOTAL, 1)
 *   favo:   honeycombscore_set(id,1)      -> ... -> item_inc(ITEM_13_EMPTY_HONEYCOMB)
 *   token:  mumboscore_set(id,1)          -> ... -> item_inc(ITEM_1C_MUMBO_TOKEN)
 * O hook roda antes da função marcada. Ele entrega o item sorteado na hora (o bit
 * vanilla de "localização checada" e o item ficam consistentes no mesmo quadro) e
 * arma a supressão do próximo incremento do contador vanilla, que vem logo depois.
 */
#include "bkr_mod.h"

typedef struct BkrSuppress {
    int armed;
    s32 item;
} BkrSuppress;

static BkrSuppress g_suppress;

void bkr_suppress_clear(void) {
    g_suppress.armed = 0;
}

static int bkr_suppress_take(s32 item, s32 diff) {
    if (g_suppress.armed && g_suppress.item == item && diff == 1) {
        g_suppress.armed = 0;
        return 1;
    }
    return 0;
}

/* Corpo original: item_adjustByDiff(item, diff, 0) (src/core2/code_BEF20.c). */
RECOMP_PATCH s32 item_adjustByDiffWithHud(enum item_e item, s32 diff) {
    if (bkr_suppress_take(item, diff)) {
        return item_getCount(item);
    }
    return item_adjustByDiff(item, diff, 0);
}

/* Corpo original: item_adjustByDiff(item, diff, 1) (src/core2/code_BEF20.c). */
RECOMP_PATCH void item_adjustByDiffWithoutHud(enum item_e item, s32 diff) {
    if (bkr_suppress_take(item, diff)) {
        return;
    }
    item_adjustByDiff(item, diff, 1);
}

static void bkr_location_checked(bkr_u16 loc, s32 vanilla_counter_item) {
    bkr_u8 item;
    if (!bkr_rando_active() || loc == BKR_LOC_INVALID || getGameMode() != GAME_MODE_3_NORMAL) {
        return;
    }
    item = g_bkr.result.placement[loc];
    if (item == bkr_locations[loc].vanilla_item) {
        return; /* o fluxo vanilla já entrega o item certo */
    }
    /* Entrega antes de armar a supressão: a entrega nunca é suprimida. */
    bkr_grant_item(item);
    g_suppress.armed = 1;
    g_suppress.item = vanilla_counter_item;
}

RECOMP_HOOK("jiggyscore_setCollected") void bkr_on_jiggy_set(s32 index, s32 value) {
    if (value && index > 0 && index <= 100 && !jiggyscore_isCollected(index)) {
        bkr_location_checked(bkr_loc_by_jiggy[index], ITEM_26_JIGGY_TOTAL);
    }
}

RECOMP_HOOK("honeycombscore_set") void bkr_on_honeycomb_set(s32 index, s32 value) {
    if (value && index > 0 && index <= 24 && !honeycombscore_get(index)) {
        bkr_location_checked(bkr_loc_by_honeycomb[index], ITEM_13_EMPTY_HONEYCOMB);
    }
}

RECOMP_HOOK("mumboscore_set") void bkr_on_token_set(s32 index, s32 value) {
    if (value && index > 0 && index < 128 && !mumboscore_get(index)) {
        bkr_location_checked(bkr_loc_by_token[index], ITEM_1C_MUMBO_TOKEN);
    }
}

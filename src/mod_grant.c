/*
 * mod_grant.c — Entrega de itens e estado derivado.
 *
 * Princípio do estado derivado: nada extra é salvo. Jiggies para gastar (ITEM_26),
 * tokens (ITEM_1C) e movimentos já são persistidos pelo jogo; favos recebidos são
 * recontados a partir dos bits vanilla de "localização checada" + placement.
 * Por isso a entrega acontece no mesmo quadro em que o jogo marca a localização:
 * não existe janela em que o bit esteja salvo e o item não.
 */
#include "bkr_mod.h"

void bkr_grant_item(bkr_u8 item) {
    if (BKR_ITEM_IS_MOVE(item)) {
        ability_unlock((enum ability_e)BKR_ITEM_ABILITY(item));
    } else if (item == BKR_ITEM_JIGGY) {
        item_adjustByDiffWithoutHud(ITEM_26_JIGGY_TOTAL, 1);
    } else if (item == BKR_ITEM_TOKEN) {
        item_inc(ITEM_1C_MUMBO_TOKEN);
    } else if (item == BKR_ITEM_HONEYCOMB) {
        item_inc(ITEM_13_EMPTY_HONEYCOMB);
        /* Igual ao vanilla (marker.c): trava a pausa até a animação de vida
         * extra, que depois chama gcpausemenu_80314AC8(1). */
        if (!(item_getCount(ITEM_13_EMPTY_HONEYCOMB) < 6)) {
            gcpausemenu_80314AC8(0);
        }
    } else {
        return;
    }
    bkr_ui_toast_item(item);
}

static int bkr_location_checked_in_save(bkr_u32 loc) {
    const BkrLocDef* d = &bkr_locations[loc];
    bkr_u8 item = g_bkr.result.placement[loc];
    switch (d->kind) {
        case BKR_LOC_JIGGY:
            return jiggyscore_isCollected(d->game_id) != 0;
        case BKR_LOC_HONEYCOMB:
            return honeycombscore_get(d->game_id) != 0;
        case BKR_LOC_TOKEN:
            return mumboscore_get(d->game_id) != 0;
        case BKR_LOC_MOLEHILL:
            /* Modo "entre molehills": molehill checado <=> movimento sorteado aprendido. */
            return BKR_ITEM_IS_MOVE(item) && ability_hasLearned((enum ability_e)BKR_ITEM_ABILITY(item));
        default:
            return 0;
    }
}

bkr_u32 bkr_count_received(bkr_u8 item) {
    bkr_u32 i;
    bkr_u32 count = 0;
    for (i = 0; i < BKR_LOCATION_COUNT; i++) {
        if (g_bkr.result.placement[i] == item && bkr_location_checked_in_save(i)) {
            count++;
        }
    }
    return count;
}

/* Corpo original (src/core2/honeycombscore.c) mantido para o modo desligado.
 * Usado por func_8034789C (vida máxima, a cada carga de save e troca de mapa)
 * e pelo total de favos do menu de pausa. */
RECOMP_PATCH s32 honeycombscore_get_total(void) {
    int i;
    s32 total;
    if (bkr_rando_active()) {
        return (s32)bkr_count_received(BKR_ITEM_HONEYCOMB);
    }
    for (i = 0, total = 0; i < 0xC; i++) {
        total += honeycombscore_get_level_total(i);
    }
    return total;
}

/* saveData_load carrega os favos ANTES dos tokens, então a vida calculada
 * no meio do carregamento ficaria errada; recalcula no fim. */
RECOMP_HOOK_RETURN("saveData_load") void bkr_after_save_load(void) {
    if (bkr_rando_active()) {
        func_80347958();
    }
}

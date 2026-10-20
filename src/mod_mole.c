/*
 * mod_mole.c — Movimentos embaralhados entre os 9 molehills dos mundos.
 *
 * Em vez de reescrever o ator do Bottles, reescrevemos a tabela moleTable
 * (overlay core2, residente): cada molehill passa a ensinar o movimento
 * sorteado com o texto de ensino/lembrete DESSE movimento, mantendo o nó de
 * câmera do próprio molehill. Toda a lógica vanilla (aprender vs. lembrete,
 * ovos/penas grátis embutidos no diálogo, plataformas de salto/voo) continua.
 */
#include "bkr_mod.h"

BKR_STATIC_ASSERT(mole_desc_size, sizeof(BkrMoleDescription) == 6);

static BkrMoleDescription g_mole_vanilla[9];
static BkrMoleDescription g_mole_wanted[9];
static int g_mole_ready;

static void bkr_mole_copy(BkrMoleDescription* dst, const BkrMoleDescription* src) {
    dst->teach_text_id = src->teach_text_id;
    dst->refresher_text_id = src->refresher_text_id;
    dst->camera_node = src->camera_node;
    dst->ability = src->ability;
}

static int bkr_mole_same(const BkrMoleDescription* a, const BkrMoleDescription* b) {
    return a->teach_text_id == b->teach_text_id && a->refresher_text_id == b->refresher_text_id &&
           a->camera_node == b->camera_node && a->ability == b->ability;
}

void bkr_mole_apply(const bkr_u8* placement) {
    int i;
    if (!g_mole_ready) {
        for (i = 0; i < 9; i++) {
            bkr_mole_copy(&g_mole_vanilla[i], &moleTable[i]);
        }
    }
    for (i = 0; i < 9; i++) {
        bkr_u8 item = placement[bkr_loc_by_mole[i]];
        int src = i;
        int j;
        if (BKR_ITEM_IS_MOVE(item)) {
            for (j = 0; j < 9; j++) {
                if (g_mole_vanilla[j].ability == BKR_ITEM_ABILITY(item)) {
                    src = j;
                }
            }
        }
        bkr_mole_copy(&g_mole_wanted[i], &g_mole_vanilla[src]);
        g_mole_wanted[i].camera_node = g_mole_vanilla[i].camera_node;
        bkr_mole_copy(&moleTable[i], &g_mole_wanted[i]);
    }
    g_mole_ready = 1;
}

/* Chamada a cada quadro: se algo restaurar a tabela (por exemplo, uma recarga do
 * overlay core2), reaplica. Custa 9 comparações. */
void bkr_mole_tick(void) {
    int i;
    if (!g_mole_ready) {
        return;
    }
    for (i = 0; i < 9; i++) {
        if (!bkr_mole_same(&moleTable[i], &g_mole_wanted[i])) {
            bkr_mole_copy(&moleTable[i], &g_mole_wanted[i]);
        }
    }
}

/*
 * Corpo original: src/core2/ch/mole.c. Os casos 1–4 apontam a câmera para as
 * botas (BGS, nó 0x11) e os tênis (GV, nó 0x29). Quando esses textos são
 * ensinados em outro molehill, o nó não existe no mapa: pulamos a câmera e a
 * flag de nível, mantendo o restante do comportamento.
 */
RECOMP_PATCH void chmole_additionalAbilityLearnActions(ActorMarker *marker, enum asset_e arg1, s32 arg2) {
    Actor *actor = marker_getActor(marker);
    int mole_index = actor->actorTypeSpecificField - 9;
    int boots_here = !bkr_rando_active() || mole_index == 7;
    int trainers_here = !bkr_rando_active() || mole_index == 8;
    switch (arg2) {
        case 1: /* Botas pernaltas */
            if (boots_here) {
                timed_setStaticCameraToNode(0.0f, 0x11);
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, TRUE);
            }
            break;
        case 2:
            if (boots_here) {
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, FALSE);
            }
            chmole_setStaticCamera(actor);
            break;
        case 3: /* Tênis turbo */
            if (trainers_here) {
                timed_setStaticCameraToNode(0.0f, 0x29);
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, TRUE);
            }
            break;
        case 4:
            if (trainers_here) {
                levelSpecificFlags_set(LEVEL_FLAG_1A_UNKNOWN, FALSE);
            }
            chmole_setStaticCamera(actor);
            break;
        case 5: /* Disparo de ovos */
            item_adjustByDiffWithHud(ITEM_D_EGGS, 50);
            break;
        case 6: /* Voo */
            item_adjustByDiffWithHud(ITEM_F_RED_FEATHER, 25);
            break;
        case 7: /* Asa maravilha */
            item_adjustByDiffWithHud(ITEM_10_GOLD_FEATHER, 5);
            break;
        case 8: /* Recarga de vida */
            item_set(ITEM_14_HEALTH, item_getCount(ITEM_15_HEALTH_TOTAL));
            break;
        case 0xff:
            chmole_setStaticCamera(actor);
            break;
    }
}

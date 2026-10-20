/*
 * mod_tick.c — Pulso por quadro: mainLoop (src/core1/code_0.c) roda uma vez
 * por quadro e não é alterado pelo Banjo: Recompiled.
 */
#include "bkr_mod.h"

RECOMP_HOOK("mainLoop") void bkr_on_main_loop(void) {
    /* Segurança: a supressão é de uso imediato; nunca sobrevive a um quadro. */
    bkr_suppress_clear();
    if (bkr_rando_active()) {
        bkr_mole_tick();
    }
    bkr_ui_tick();
}

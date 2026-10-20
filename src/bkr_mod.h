/*
 * bkr_mod.h — Declarações compartilhadas do mod (lado MIPS).
 * As assinaturas do jogo foram copiadas das definições do decomp
 * (gitlab.com/banjo.decomp/banjo-kazooie @ 351ca158) porque os headers
 * públicos do decomp não as declaram.
 */
#ifndef BKR_MOD_H
#define BKR_MOD_H

#include "modding.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "functions.h"

#include "bkr/bkr_generate.h"
#include "bkr/bkr_ids.h"

/* ---- Funções do jogo (definidas no decomp, ausentes nos headers) ---- */
u32 jiggyscore_isCollected(enum jiggy_e jiggy_id);
bool honeycombscore_get(enum honeycomb_e indx);
s32 honeycombscore_get_level_total(enum level_e level_id);
bool mumboscore_get(enum mumbotoken_e indx);
s32 item_adjustByDiff(enum item_e item, s32 diff, s32 no_hud);
s32 item_adjustByDiffWithHud(enum item_e item, s32 diff);      /* patch em mod_collect.c */
void item_adjustByDiffWithoutHud(enum item_e item, s32 diff);  /* patch em mod_collect.c */
s32 item_getCount(enum item_e item);
void item_inc(enum item_e item);
void item_set(s32 item, s32 val);
int ability_hasLearned(enum ability_e ability);
enum map_e map_get(void);
s32 getGameMode(void);
void gcpausemenu_80314AC8(int arg0);
void func_80347958(void);
void chmole_setStaticCamera(Actor *this);

/* moleTable (src/core2/ch/mole.c): 10 entradas de 6 bytes no overlay core2. */
typedef struct BkrMoleDescription {
    s16 teach_text_id;
    s16 refresher_text_id;
    s8 camera_node;
    s8 ability;
} BkrMoleDescription;
extern BkrMoleDescription moleTable[10];

/* Exportado pelo Banjo: Recompiled (patches/mumbo_token_patches.c). */
RECOMP_IMPORT("*", void bkrecomp_set_mumbo_token_fixes_enabled(int val));

/* ---- Estado do mod ---- */
typedef struct BkrModState {
    int enabled;          /* opção "Randomizer" ligada */
    int active;           /* seed gerado e save trocado */
    int error;            /* opção ligada mas configuração inválida */
    int lang;             /* BKR_LANG_* */
    int toasts;           /* mostrar avisos de item */
    int debug;            /* registrar no console (--show-console) */
    BkrSeedSpec spec;
    BkrGenResult result;
    char share_code[BKR_SHARE_CODE_MAX];
} BkrModState;

extern BkrModState g_bkr;

/* mod_init.c */
int bkr_rando_active(void);

/* mod_collect.c */
void bkr_suppress_clear(void);

/* mod_grant.c */
void bkr_grant_item(bkr_u8 item);
bkr_u32 bkr_count_received(bkr_u8 item);

/* mod_mole.c */
void bkr_mole_apply(const bkr_u8* placement);
void bkr_mole_tick(void);

/* mod_ui.c */
void bkr_ui_tick(void);
void bkr_ui_toast_item(bkr_u8 item);
void bkr_ui_toast_text(const char* text);

#endif

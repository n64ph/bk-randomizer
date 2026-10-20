/*
 * bkr_logic.h — Estado lógico, avaliação de regras e busca por ponto fixo.
 */
#ifndef BKR_LOGIC_H
#define BKR_LOGIC_H

#include "bkr_types.h"
#include "bkr_world.h"

/* Custos dependentes de configuração (v1 usa os valores vanilla). */
typedef struct BkrCosts {
    bkr_u16 note_door[12];                 /* notas por porta */
    bkr_u16 world_cum[10];                 /* jiggies acumulados por slot de quebra-cabeça */
    bkr_u16 transform_cum[8];              /* tokens acumulados por transformation_e */
} BkrCosts;

typedef struct BkrState {
    bkr_u32 moves;                         /* bit = ability_e */
    bkr_u32 jiggies;
    bkr_u32 tokens;
    bkr_u32 honeycombs;
    bkr_u32 notes;
    bkr_u32 events[BKR_EVENT_WORDS];
    bkr_u32 regions[BKR_REGION_WORDS];
} BkrState;

typedef struct BkrSearch {
    BkrState st;
    bkr_u32 loc_done[BKR_LOC_WORDS];       /* localização alcançada nesta busca */
    bkr_u32 group_done[BKR_GROUP_WORDS];   /* grupo de notas já somado */
    bkr_u32 evdef_done[BKR_EVDEF_WORDS];   /* definição de evento já satisfeita */
} BkrSearch;

enum { BKR_SWEEP_COLLECT = 0, BKR_SWEEP_MARK = 1 };

void bkr_costs_vanilla(BkrCosts* c);

void bkr_state_clear(BkrState* st);
void bkr_state_copy(BkrState* dst, const BkrState* src);
void bkr_state_add_item(BkrState* st, bkr_u8 item);
void bkr_state_remove_item(BkrState* st, bkr_u8 item);
/* Estado inicial: movimentos de Spiral Mountain (BKR_STARTING_MOVES). */
void bkr_state_base(BkrState* st);

int bkr_rule_eval_code(const bkr_u16* code, const BkrState* st, const BkrCosts* costs);
int bkr_rule_eval(bkr_u16 rule, const BkrState* st, const BkrCosts* costs);

/* Começa uma busca a partir de base, com a região inicial alcançada. */
void bkr_search_init(BkrSearch* s, const BkrState* base);
/* Varre até o ponto fixo. Em BKR_SWEEP_COLLECT, o item de cada localização
 * alcançada (placement[i] != BKR_ITEM_NONE) entra no estado; em
 * BKR_SWEEP_MARK, só marca loc_done. placement pode ser BKR_NULL. */
void bkr_search_sweep(BkrSearch* s, const BkrCosts* costs, const bkr_u8* placement, int mode);

#endif

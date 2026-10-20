/* bkr_logic.c — Estado, avaliação de regras (RPN) e busca por ponto fixo. */
#include "bkr/bkr_logic.h"
#include "bkr/bkr_rules.h"
#include "bkr/bkr_ids.h"

void bkr_costs_vanilla(BkrCosts* c) {
    static const bkr_u16 doors[12] = { 50, 180, 260, 350, 450, 640, 765, 810, 828, 846, 864, 882 };
    static const bkr_u16 puzzle[10] = { 1, 2, 5, 7, 8, 9, 10, 12, 15, 25 };
    bkr_u32 i;
    bkr_u32 acc = 0;
    for (i = 0; i < 12; i++) {
        c->note_door[i] = doors[i];
    }
    for (i = 0; i < 10; i++) {
        acc += puzzle[i];
        c->world_cum[i] = (bkr_u16)acc;
    }
    for (i = 0; i < 8; i++) {
        c->transform_cum[i] = 0xFFFF;
    }
    /* Ordem vanilla dos mundos: MM(5) BGS(10) FP(15) MMM(20) CCW(25). */
    c->transform_cum[BKR_TRANSFORM_TERMITE] = 5;
    c->transform_cum[BKR_TRANSFORM_CROC] = 15;
    c->transform_cum[BKR_TRANSFORM_WALRUS] = 30;
    c->transform_cum[BKR_TRANSFORM_PUMPKIN] = 50;
    c->transform_cum[BKR_TRANSFORM_BEE] = 75;
}

void bkr_state_clear(BkrState* st) {
    st->moves = 0;
    st->jiggies = 0;
    st->tokens = 0;
    st->honeycombs = 0;
    st->notes = 0;
    bkr_words_clear(st->events, BKR_EVENT_WORDS);
    bkr_words_clear(st->regions, BKR_REGION_WORDS);
}

void bkr_state_copy(BkrState* dst, const BkrState* src) {
    dst->moves = src->moves;
    dst->jiggies = src->jiggies;
    dst->tokens = src->tokens;
    dst->honeycombs = src->honeycombs;
    dst->notes = src->notes;
    bkr_words_copy(dst->events, src->events, BKR_EVENT_WORDS);
    bkr_words_copy(dst->regions, src->regions, BKR_REGION_WORDS);
}

void bkr_state_add_item(BkrState* st, bkr_u8 item) {
    if (BKR_ITEM_IS_MOVE(item)) {
        st->moves |= 1u << BKR_ITEM_ABILITY(item);
        return;
    }
    switch (item) {
        case BKR_ITEM_JIGGY:
            st->jiggies++;
            break;
        case BKR_ITEM_TOKEN:
            st->tokens++;
            break;
        case BKR_ITEM_HONEYCOMB:
            st->honeycombs++;
            break;
        default:
            break;
    }
}

void bkr_state_remove_item(BkrState* st, bkr_u8 item) {
    if (BKR_ITEM_IS_MOVE(item)) {
        st->moves &= ~(1u << BKR_ITEM_ABILITY(item));
        return;
    }
    switch (item) {
        case BKR_ITEM_JIGGY:
            if (st->jiggies > 0) {
                st->jiggies--;
            }
            break;
        case BKR_ITEM_TOKEN:
            if (st->tokens > 0) {
                st->tokens--;
            }
            break;
        case BKR_ITEM_HONEYCOMB:
            if (st->honeycombs > 0) {
                st->honeycombs--;
            }
            break;
        default:
            break;
    }
}

void bkr_state_base(BkrState* st) {
    bkr_state_clear(st);
    st->moves = BKR_STARTING_MOVES;
}

/* Pilha de booleanos em um único inteiro: sem arrays na pilha do MIPS. */
#define BKR_PUSH(v) do { stack = (stack << 1) | ((v) ? 1u : 0u); depth++; } while (0)
#define BKR_POP(dst) do { (dst) = stack & 1u; stack >>= 1; depth--; } while (0)

int bkr_rule_eval_code(const bkr_u16* code, const BkrState* st, const BkrCosts* costs) {
    bkr_u32 stack = 0;
    int depth = 0;
    for (;;) {
        bkr_u16 op = *code++;
        bkr_u32 a;
        bkr_u32 b;
        switch (op) {
            case BKR_OP_END:
                return depth > 0 ? (int)(stack & 1u) : 1;
            case BKR_OP_TRUE:
                BKR_PUSH(1);
                break;
            case BKR_OP_FALSE:
                BKR_PUSH(0);
                break;
            case BKR_OP_HAS:
                a = *code++;
                BKR_PUSH((st->moves >> a) & 1u);
                break;
            case BKR_OP_EVENT:
                a = *code++;
                BKR_PUSH(BKR_BIT_GET(st->events, a));
                break;
            case BKR_OP_REGION:
                a = *code++;
                BKR_PUSH(BKR_BIT_GET(st->regions, a));
                break;
            case BKR_OP_NOTE_DOOR:
                a = *code++;
                BKR_PUSH(st->notes >= costs->note_door[a]);
                break;
            case BKR_OP_WORLD_COST:
                a = *code++;
                BKR_PUSH(st->jiggies >= costs->world_cum[a]);
                break;
            case BKR_OP_TRANSFORM_COST:
                a = *code++;
                BKR_PUSH(st->tokens >= costs->transform_cum[a]);
                break;
            case BKR_OP_TOKENS:
                a = *code++;
                BKR_PUSH(st->tokens >= a);
                break;
            case BKR_OP_JIGGIES:
                a = *code++;
                BKR_PUSH(st->jiggies >= a);
                break;
            case BKR_OP_NOTES:
                a = *code++;
                BKR_PUSH(st->notes >= a);
                break;
            case BKR_OP_AND:
                BKR_POP(a);
                BKR_POP(b);
                BKR_PUSH(a & b);
                break;
            case BKR_OP_OR:
                BKR_POP(a);
                BKR_POP(b);
                BKR_PUSH(a | b);
                break;
            case BKR_OP_NOT:
                BKR_POP(a);
                BKR_PUSH(!a);
                break;
            default:
                return 0; /* opcode inválido: regra falsa (nunca acontece com dados gerados) */
        }
    }
}

int bkr_rule_eval(bkr_u16 rule, const BkrState* st, const BkrCosts* costs) {
    return bkr_rule_eval_code(&bkr_rule_code[rule], st, costs);
}

void bkr_search_init(BkrSearch* s, const BkrState* base) {
    bkr_state_copy(&s->st, base);
    bkr_words_clear(s->loc_done, BKR_LOC_WORDS);
    bkr_words_clear(s->group_done, BKR_GROUP_WORDS);
    bkr_words_clear(s->evdef_done, BKR_EVDEF_WORDS);
    BKR_BIT_SET(s->st.regions, BKR_START_REGION);
}

void bkr_search_sweep(BkrSearch* s, const BkrCosts* costs, const bkr_u8* placement, int mode) {
    int changed;
    do {
        bkr_u32 r;
        changed = 0;
        for (r = 0; r < BKR_REGION_COUNT; r++) {
            const BkrRegionDef* reg = &bkr_regions[r];
            bkr_u32 i;
            bkr_u32 end;
            if (!BKR_BIT_GET(s->st.regions, r)) {
                continue;
            }
            end = (bkr_u32)reg->exit_first + reg->exit_count;
            for (i = reg->exit_first; i < end; i++) {
                const BkrExitDef* e = &bkr_exits[i];
                if (!BKR_BIT_GET(s->st.regions, e->to) && bkr_rule_eval(e->rule, &s->st, costs)) {
                    BKR_BIT_SET(s->st.regions, e->to);
                    changed = 1;
                }
            }
            end = (bkr_u32)reg->event_first + reg->event_count;
            for (i = reg->event_first; i < end; i++) {
                const BkrEventDef* ev = &bkr_event_defs[i];
                if (!BKR_BIT_GET(s->evdef_done, i) && bkr_rule_eval(ev->rule, &s->st, costs)) {
                    BKR_BIT_SET(s->evdef_done, i);
                    if (!BKR_BIT_GET(s->st.events, ev->event)) {
                        BKR_BIT_SET(s->st.events, ev->event);
                        changed = 1;
                    }
                }
            }
            end = (bkr_u32)reg->group_first + reg->group_count;
            for (i = reg->group_first; i < end; i++) {
                const BkrNoteGroupDef* g = &bkr_note_groups[i];
                if (!BKR_BIT_GET(s->group_done, i) && bkr_rule_eval(g->rule, &s->st, costs)) {
                    BKR_BIT_SET(s->group_done, i);
                    s->st.notes += g->count;
                    changed = 1;
                }
            }
            end = (bkr_u32)reg->loc_first + reg->loc_count;
            for (i = reg->loc_first; i < end; i++) {
                if (!BKR_BIT_GET(s->loc_done, i) && bkr_rule_eval(bkr_locations[i].rule, &s->st, costs)) {
                    BKR_BIT_SET(s->loc_done, i);
                    if (mode == BKR_SWEEP_COLLECT && placement != BKR_NULL &&
                        placement[i] != BKR_ITEM_NONE) {
                        bkr_state_add_item(&s->st, placement[i]);
                        changed = 1;
                    }
                }
            }
        }
    } while (changed);
}

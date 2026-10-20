/*
 * bkr_rules.h — Bytecode de regras (RPN) produzido por tools/compile_logic.py.
 * Os valores DEVEM coincidir com OPCODES em tools/compile_logic.py
 * (tools/tests/test_compile_logic.py confere automaticamente).
 */
#ifndef BKR_RULES_H
#define BKR_RULES_H

enum {
    BKR_OP_END = 0,            /* fim da regra */
    BKR_OP_TRUE = 1,
    BKR_OP_FALSE = 2,
    BKR_OP_HAS = 3,            /* arg: ability_e */
    BKR_OP_EVENT = 4,          /* arg: índice de evento */
    BKR_OP_REGION = 5,         /* arg: índice de região */
    BKR_OP_NOTE_DOOR = 6,      /* arg: índice da porta (0..11) -> notas >= custo */
    BKR_OP_WORLD_COST = 7,     /* arg: slot do quebra-cabeça (0..9) -> jiggies >= custo acumulado */
    BKR_OP_TRANSFORM_COST = 8, /* arg: transformation_e -> tokens >= custo acumulado */
    BKR_OP_TOKENS = 9,         /* arg: quantidade */
    BKR_OP_JIGGIES = 10,       /* arg: quantidade */
    BKR_OP_NOTES = 11,         /* arg: quantidade */
    BKR_OP_AND = 12,
    BKR_OP_OR = 13,
    BKR_OP_NOT = 14
};

#define BKR_RULE_MAX_DEPTH 32

#endif

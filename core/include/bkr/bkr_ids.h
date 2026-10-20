/*
 * bkr_ids.h — Identificadores estáveis do randomizer.
 * Os valores de habilidade, mundo e transformação são IDÊNTICOS aos enums do
 * decomp de Banjo-Kazooie (include/enums.h, commit 351ca158): ability_e,
 * level_e e transformation_e. Não renumere.
 */
#ifndef BKR_IDS_H
#define BKR_IDS_H

#include "bkr_types.h"

/* enum ability_e */
enum {
    BKR_ABILITY_BEAK_BARGE = 0x00,
    BKR_ABILITY_BEAK_BOMB = 0x01,
    BKR_ABILITY_BEAK_BUSTER = 0x02,
    BKR_ABILITY_CAMERA_CONTROL = 0x03,
    BKR_ABILITY_CLAW_SWIPE = 0x04,
    BKR_ABILITY_CLIMB = 0x05,
    BKR_ABILITY_EGGS = 0x06,
    BKR_ABILITY_FEATHERY_FLAP = 0x07,
    BKR_ABILITY_FLAP_FLIP = 0x08,
    BKR_ABILITY_FLIGHT = 0x09,
    BKR_ABILITY_HIGH_JUMP = 0x0A,
    BKR_ABILITY_RATATAT_RAP = 0x0B,
    BKR_ABILITY_ROLL = 0x0C,
    BKR_ABILITY_SHOCK_JUMP = 0x0D,
    BKR_ABILITY_WADING_BOOTS = 0x0E,
    BKR_ABILITY_DIVE = 0x0F,
    BKR_ABILITY_TALON_TROT = 0x10,
    BKR_ABILITY_TURBO_TALON = 0x11,
    BKR_ABILITY_WONDERWING = 0x12,
    BKR_ABILITY_NOTE_DOORS = 0x13,
    BKR_ABILITY_COUNT = 0x14
};

/* enum level_e */
enum {
    BKR_WORLD_NONE = 0,
    BKR_WORLD_MM = 1,
    BKR_WORLD_TTC = 2,
    BKR_WORLD_CC = 3,
    BKR_WORLD_BGS = 4,
    BKR_WORLD_FP = 5,
    BKR_WORLD_LAIR = 6,
    BKR_WORLD_GV = 7,
    BKR_WORLD_CCW = 8,
    BKR_WORLD_RBB = 9,
    BKR_WORLD_MMM = 10,
    BKR_WORLD_SM = 11,
    BKR_WORLD_COUNT = 12
};

/* enum transformation_e */
enum {
    BKR_TRANSFORM_TERMITE = 2,
    BKR_TRANSFORM_PUMPKIN = 3,
    BKR_TRANSFORM_WALRUS = 4,
    BKR_TRANSFORM_CROC = 5,
    BKR_TRANSFORM_BEE = 6,
    BKR_TRANSFORM_SLOTS = 8
};

/* Tipos de localização (iguais aos de tools/compile_logic.py). */
enum {
    BKR_LOC_NONE = 0,
    BKR_LOC_JIGGY = 1,
    BKR_LOC_HONEYCOMB = 2,
    BKR_LOC_TOKEN = 3,
    BKR_LOC_MOLEHILL = 4
};

/* Itens. Movimento = BKR_ITEM_MOVE_BASE + ability_e. */
enum {
    BKR_ITEM_NONE = 0,
    BKR_ITEM_JIGGY = 1,
    BKR_ITEM_HONEYCOMB = 2,
    BKR_ITEM_TOKEN = 3,
    BKR_ITEM_MOVE_BASE = 0x20
};
#define BKR_ITEM_MOVE(ability) ((bkr_u8)(BKR_ITEM_MOVE_BASE + (ability)))
#define BKR_ITEM_IS_MOVE(item) ((item) >= BKR_ITEM_MOVE_BASE && (item) < BKR_ITEM_MOVE_BASE + BKR_ABILITY_COUNT)
#define BKR_ITEM_ABILITY(item) ((item) - BKR_ITEM_MOVE_BASE)

/* Idiomas de texto. */
enum { BKR_LANG_PT_BR = 0, BKR_LANG_EN = 1 };

/* Nomes legíveis (bkr_names.c). Nunca retornam NULL. */
const char* bkr_item_name(bkr_u8 item, int lang);
const char* bkr_world_name(bkr_u8 world, int lang);
const char* bkr_hash_word(bkr_u32 index);

#endif

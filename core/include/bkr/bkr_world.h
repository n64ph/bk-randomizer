/*
 * bkr_world.h — Estruturas das tabelas de lógica geradas em
 * core/generated/bkr_logic_data.c por tools/compile_logic.py.
 * Cada região possui faixas contíguas de saídas, localizações, eventos e
 * grupos de notas.
 */
#ifndef BKR_WORLD_H
#define BKR_WORLD_H

#include "bkr_types.h"
#include "bkr_logic_data.h"

typedef struct BkrRegionDef {
    bkr_u16 exit_first, exit_count;
    bkr_u16 loc_first, loc_count;
    bkr_u16 event_first, event_count;
    bkr_u16 group_first, group_count;
    bkr_u8 world;
    bkr_u8 pad;
} BkrRegionDef;

typedef struct BkrExitDef {
    bkr_u16 to;
    bkr_u16 rule;
} BkrExitDef;

typedef struct BkrLocDef {
    bkr_u16 region;
    bkr_u16 rule;
    bkr_u16 game_id;      /* jiggy_e / honeycomb_e / uid do token / índice em moleTable */
    bkr_u8 kind;          /* BKR_LOC_* */
    bkr_u8 world;         /* level_e */
    bkr_u8 vanilla_item;  /* BKR_ITEM_* */
    bkr_u8 pad;
} BkrLocDef;

typedef struct BkrEventDef {
    bkr_u16 event;
    bkr_u16 rule;
} BkrEventDef;

typedef struct BkrNoteGroupDef {
    bkr_u16 rule;
    bkr_u8 count;
    bkr_u8 world;
} BkrNoteGroupDef;

#define BKR_LOC_INVALID 0xFFFFu
#define BKR_LOC_WORDS BKR_WORDS(BKR_LOCATION_COUNT)
#define BKR_REGION_WORDS BKR_WORDS(BKR_REGION_COUNT)
#define BKR_EVENT_WORDS BKR_WORDS(BKR_EVENT_COUNT)
#define BKR_EVDEF_WORDS BKR_WORDS(BKR_EVENT_DEF_COUNT)
#define BKR_GROUP_WORDS BKR_WORDS(BKR_NOTE_GROUP_COUNT)

extern const BkrRegionDef bkr_regions[BKR_REGION_COUNT];
extern const BkrExitDef bkr_exits[BKR_EXIT_COUNT];
extern const BkrLocDef bkr_locations[BKR_LOCATION_COUNT];
extern const BkrEventDef bkr_event_defs[BKR_EVENT_DEF_COUNT];
extern const BkrNoteGroupDef bkr_note_groups[BKR_NOTE_GROUP_COUNT];
extern const bkr_u16 bkr_rule_code[BKR_RULE_CODE_SIZE];
extern const char* const bkr_location_names[BKR_LOCATION_COUNT];
extern const char* const bkr_region_names[BKR_REGION_COUNT];
extern const char* const bkr_event_names[BKR_EVENT_COUNT];

/* Busca reversa: id do jogo -> índice de localização (BKR_LOC_INVALID se não houver). */
extern const bkr_u16 bkr_loc_by_jiggy[101];
extern const bkr_u16 bkr_loc_by_honeycomb[25];
extern const bkr_u16 bkr_loc_by_token[128];
extern const bkr_u16 bkr_loc_by_mole[10];

#endif

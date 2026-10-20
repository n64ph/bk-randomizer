/* bkr_names.c — Nomes de itens, mundos e palavras do hash (PT-BR / EN). */
#include "bkr/bkr_ids.h"

static const char* const bkr_move_names_pt[BKR_ABILITY_COUNT] = {
    "Investida de Bico", "Bico-Bomba", "Bico-Martelo", "Controle de Camera",
    "Patada", "Escalar", "Disparo de Ovos", "Bater de Penas", "Salto Mortal",
    "Voo", "Salto Alto", "Bicadas Rapidas", "Rolamento", "Salto de Mola",
    "Botas Pernaltas", "Mergulho", "Trote de Garras", "Tenis Turbo",
    "Asa Maravilha", "Portas de Notas"
};

static const char* const bkr_move_names_en[BKR_ABILITY_COUNT] = {
    "Beak Barge", "Beak Bomb", "Beak Buster", "Camera Control", "Claw Swipe",
    "Climb", "Egg Firing", "Feathery Flap", "Flap Flip", "Flight",
    "High Jump", "Rat-a-tat Rap", "Roll", "Shock Spring Jump", "Wading Boots",
    "Dive", "Talon Trot", "Turbo Talon Trot", "Wonderwing", "Note Doors"
};

static const char* const bkr_world_names[2][BKR_WORLD_COUNT] = {
    { "Nenhum", "Mumbo's Mountain", "Treasure Trove Cove", "Clanker's Cavern",
      "Bubblegloop Swamp", "Freezeezy Peak", "Covil de Gruntilda", "Gobi's Valley",
      "Click Clock Wood", "Rusty Bucket Bay", "Mad Monster Mansion", "Spiral Mountain" },
    { "None", "Mumbo's Mountain", "Treasure Trove Cove", "Clanker's Cavern",
      "Bubblegloop Swamp", "Freezeezy Peak", "Gruntilda's Lair", "Gobi's Valley",
      "Click Clock Wood", "Rusty Bucket Bay", "Mad Monster Mansion", "Spiral Mountain" }
};

static const char* const bkr_hash_words[32] = {
    "Banjo", "Kazooie", "Mumbo", "Bottles", "Tooty", "Grunty", "Klungo", "Brentilda",
    "Cheato", "Jinjo", "Jiggy", "Nota", "Favo", "Token", "Conga", "Chimpy",
    "Clanker", "Gobi", "Boggy", "Wozza", "Tiptup", "Nipper", "Leaky", "Blubber",
    "Snacker", "Tumblar", "Loggo", "Napper", "Nabnut", "Eyrie", "Gnawty", "Dingpot"
};

const char* bkr_item_name(bkr_u8 item, int lang) {
    int en = (lang == BKR_LANG_EN);
    if (BKR_ITEM_IS_MOVE(item)) {
        return en ? bkr_move_names_en[BKR_ITEM_ABILITY(item)] : bkr_move_names_pt[BKR_ITEM_ABILITY(item)];
    }
    switch (item) {
        case BKR_ITEM_JIGGY:
            return "Jiggy";
        case BKR_ITEM_HONEYCOMB:
            return en ? "Empty Honeycomb" : "Favo de Mel Vazio";
        case BKR_ITEM_TOKEN:
            return en ? "Mumbo Token" : "Token do Mumbo";
        default:
            return en ? "Nothing" : "Nada";
    }
}

const char* bkr_world_name(bkr_u8 world, int lang) {
    if (world >= BKR_WORLD_COUNT) {
        world = BKR_WORLD_NONE;
    }
    return bkr_world_names[lang == BKR_LANG_EN ? 1 : 0][world];
}

const char* bkr_hash_word(bkr_u32 index) {
    return bkr_hash_words[index & 31u];
}

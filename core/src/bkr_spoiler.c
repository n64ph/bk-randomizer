/* bkr_spoiler.c — Spoiler: cabeçalho, esferas de coleta e lista por mundo. */
#include "bkr/bkr_spoiler.h"
#include "bkr/bkr_ids.h"

static BkrSearch g_sp_search;
static BkrState g_sp_base;
static BkrCosts g_sp_costs;
static bkr_u32 g_sp_collected[BKR_LOC_WORDS];
static bkr_u16 g_sp_new[BKR_LOCATION_COUNT];

static const char* bkr_opt(int lang, const char* pt, const char* en) {
    return lang == BKR_LANG_EN ? en : pt;
}

static void bkr_spoiler_header(const BkrSeedSpec* spec, const BkrGenResult* res, int lang, BkrText* t) {
    char code[BKR_SHARE_CODE_MAX];
    const BkrSettings* s = &spec->settings;
    bkr_u32 i;
    bkr_share_code_format(spec, code, sizeof(code));
    bkr_text_put(t, "Banjo-Kazooie Randomizer - Spoiler\n");
    bkr_text_put(t, bkr_opt(lang, "Codigo: ", "Code: "));
    bkr_text_put(t, code);
    bkr_text_put(t, "\nHash: ");
    for (i = 0; i < 5; i++) {
        bkr_text_put(t, bkr_hash_word(res->fingerprint >> (i * 5)));
        bkr_text_putc(t, i < 4 ? ' ' : '\n');
    }
    bkr_text_put(t, bkr_opt(lang, "Impressao digital: ", "Fingerprint: "));
    bkr_text_hex(t, res->fingerprint, 8);
    bkr_text_put(t, bkr_opt(lang, "\nTentativas: ", "\nAttempts: "));
    bkr_text_u32(t, res->attempts);
    bkr_text_put(t, bkr_opt(lang, "\nOpcoes: logica=", "\nSettings: logic="));
    bkr_text_put(t, s->logic == BKR_LOGIC_NONE ? "none" : "glitchless");
    bkr_text_put(t, " shuffle=");
    bkr_text_put(t, s->shuffle == BKR_SHUFFLE_VANILLA ? "vanilla" : (s->shuffle == BKR_SHUFFLE_BY_TYPE ? "type" : "mixed"));
    bkr_text_put(t, " scope=");
    bkr_text_put(t, s->scope == BKR_SCOPE_WORLD ? "world" : "game");
    bkr_text_put(t, " moves=");
    bkr_text_put(t, s->moves == BKR_MOVES_MOLEHILLS ? "molehills" : "vanilla");
    bkr_text_put(t, " access=");
    bkr_text_put(t, s->access == BKR_ACCESS_BEATABLE ? "beatable" : "all");
    bkr_text_putc(t, '\n');
}

static void bkr_spoiler_line(bkr_u32 loc, const bkr_u8* placement, int lang, BkrText* t) {
    bkr_text_put(t, "  ");
    bkr_text_put(t, bkr_location_names[loc]);
    bkr_text_put(t, ": ");
    bkr_text_put(t, bkr_item_name(placement[loc], lang));
    bkr_text_putc(t, '\n');
}

bkr_u32 bkr_spoiler_write(const BkrSeedSpec* spec, const BkrGenResult* res, int lang, BkrText* out) {
    bkr_u32 sphere = 0;
    bkr_u32 i;
    bkr_u32 w;
    bkr_spoiler_header(spec, res, lang, out);
    bkr_text_put(out, bkr_opt(lang, "\n== Esferas de coleta ==\n", "\n== Playthrough spheres ==\n"));
    bkr_costs_vanilla(&g_sp_costs);
    bkr_state_base(&g_sp_base);
    bkr_search_init(&g_sp_search, &g_sp_base);
    bkr_words_clear(g_sp_collected, BKR_LOC_WORDS);
    for (;;) {
        bkr_u32 new_n = 0;
        bkr_search_sweep(&g_sp_search, &g_sp_costs, res->placement, BKR_SWEEP_MARK);
        for (i = 0; i < BKR_LOCATION_COUNT; i++) {
            if (BKR_BIT_GET(g_sp_search.loc_done, i) && !BKR_BIT_GET(g_sp_collected, i)) {
                g_sp_new[new_n++] = (bkr_u16)i;
            }
        }
        if (new_n == 0) {
            break;
        }
        sphere++;
        {
            bkr_u32 jiggies = 0;
            bkr_u32 tokens = 0;
            bkr_u32 combs = 0;
            for (i = 0; i < new_n; i++) {
                bkr_u8 item = res->placement[g_sp_new[i]];
                jiggies += (item == BKR_ITEM_JIGGY);
                tokens += (item == BKR_ITEM_TOKEN);
                combs += (item == BKR_ITEM_HONEYCOMB);
            }
            bkr_text_put(out, bkr_opt(lang, "Esfera ", "Sphere "));
            bkr_text_u32(out, sphere);
            bkr_text_put(out, ": +");
            bkr_text_u32(out, jiggies);
            bkr_text_put(out, " jiggies, +");
            bkr_text_u32(out, tokens);
            bkr_text_put(out, " tokens, +");
            bkr_text_u32(out, combs);
            bkr_text_put(out, bkr_opt(lang, " favos\n", " honeycombs\n"));
        }
        for (i = 0; i < new_n; i++) {
            bkr_u32 loc = g_sp_new[i];
            BKR_BIT_SET(g_sp_collected, loc);
            bkr_state_add_item(&g_sp_search.st, res->placement[loc]);
            if (BKR_ITEM_IS_MOVE(res->placement[loc])) {
                bkr_spoiler_line(loc, res->placement, lang, out);
            }
        }
    }
    bkr_text_put(out, bkr_opt(lang, "\n== Localizacoes por mundo ==\n", "\n== Locations by world ==\n"));
    for (w = 1; w < BKR_WORLD_COUNT; w++) {
        int header = 0;
        for (i = 0; i < BKR_LOCATION_COUNT; i++) {
            if (bkr_locations[i].world != w) {
                continue;
            }
            if (!header) {
                bkr_text_put(out, bkr_world_name((bkr_u8)w, lang));
                bkr_text_put(out, ":\n");
                header = 1;
            }
            bkr_spoiler_line(i, res->placement, lang, out);
        }
    }
    return sphere;
}

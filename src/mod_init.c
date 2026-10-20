/*
 * mod_init.c — Inicialização: lê as opções do mod, gera o seed com o núcleo
 * portátil, troca o arquivo de save e aplica os patches de dados.
 * Roda no evento recomp_on_init (dummy_func_8025AFB0, logo após carregar o
 * core2 e antes de qualquer leitura de save).
 */
#include "bkr_mod.h"
#include "bkr/bkr_text.h"

BkrModState g_bkr;

int bkr_rando_active(void) {
    return g_bkr.active;
}

static void bkr_read_seed_option(const char* key, char* out, bkr_u32 cap) {
    char* text = recomp_get_config_string(key);
    bkr_u32 i = 0;
    out[0] = '\0';
    if (text == NULL) {
        return;
    }
    while (text[i] != '\0' && i + 1 < cap) {
        out[i] = text[i];
        i++;
    }
    out[i] = '\0';
    recomp_free_config_string(text);
}

static void bkr_read_config(void) {
    static char raw_seed[64];
    static char raw_code[64];
    BkrSettings* s = &g_bkr.spec.settings;
    g_bkr.enabled = recomp_get_config_u32("enabled") == 1;
    g_bkr.lang = recomp_get_config_u32("language") == 1 ? BKR_LANG_EN : BKR_LANG_PT_BR;
    g_bkr.toasts = recomp_get_config_u32("toasts") == 1;
    g_bkr.debug = recomp_get_config_u32("debug_log") == 1;
    s->logic = (bkr_u8)recomp_get_config_u32("logic");
    s->shuffle = (bkr_u8)recomp_get_config_u32("shuffle");
    s->scope = (bkr_u8)recomp_get_config_u32("scope");
    s->moves = (bkr_u8)recomp_get_config_u32("moves");
    s->access = (bkr_u8)recomp_get_config_u32("access");
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
    bkr_read_seed_option("seed", raw_seed, sizeof(raw_seed));
    bkr_seed_sanitize(raw_seed, g_bkr.spec.seed);
    bkr_read_seed_option("share_code", raw_code, sizeof(raw_code));
    if (raw_code[0] != '\0' && !bkr_share_code_parse(raw_code, &g_bkr.spec)) {
        g_bkr.error = 1;
    }
}

static void bkr_save_name(bkr_u32 fingerprint, char* out, bkr_u32 cap) {
    BkrText t;
    bkr_text_init(&t, out, cap);
    bkr_text_put(&t, "bkr_");
    bkr_text_hex(&t, fingerprint, 8);
}

RECOMP_CALLBACK("*", recomp_on_init) void bkr_on_init(void) {
    char save_name[16];
    g_bkr.active = 0;
    g_bkr.error = 0;
    bkr_read_config();
    if (!g_bkr.enabled) {
        return;
    }
    if (g_bkr.error) {
        recomp_printf("[bkr] codigo de compartilhamento invalido; randomizer desligado\n");
        return;
    }
    if (g_bkr.spec.seed[0] == '\0' || !bkr_generate(&g_bkr.spec, &g_bkr.result)) {
        g_bkr.error = 1;
        recomp_printf("[bkr] seed invalido ou geracao falhou; randomizer desligado\n");
        return;
    }
    bkr_share_code_format(&g_bkr.spec, g_bkr.share_code, sizeof(g_bkr.share_code));
    /* IDs únicos para os tokens que o jogo original duplicava (0x3D e 0x5E). */
    bkrecomp_set_mumbo_token_fixes_enabled(1);
    bkr_save_name(g_bkr.result.fingerprint, save_name, sizeof(save_name));
    recomp_change_save_file(save_name);
    bkr_mole_apply(g_bkr.result.placement);
    g_bkr.active = 1;
    if (g_bkr.debug) {
        recomp_printf("[bkr] codigo=%s fingerprint=%08X placement_hash=%08X tentativas=%u save=%s\n",
                      g_bkr.share_code, g_bkr.result.fingerprint,
                      bkr_placement_hash(g_bkr.result.placement), g_bkr.result.attempts, save_name);
    }
}

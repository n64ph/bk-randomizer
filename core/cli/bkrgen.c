/*
 * bkrgen.c — CLI do gerador (host). Usa o MESMO núcleo do mod, então o seed
 * gerado aqui é idêntico ao gerado no jogo.
 *
 *   bkrgen --seed BANJO [--logic glitchless|none] [--shuffle vanilla|type|mixed]
 *          [--scope game|world] [--moves vanilla|molehills] [--access all|beatable]
 *          [--code BKR1-XXXXXX-SEED] [--lang pt|en] [--spoiler arquivo.txt]
 */
#include <stdio.h>
#include <string.h>
#include "bkr/bkr_generate.h"
#include "bkr/bkr_spoiler.h"
#include "bkr/bkr_ids.h"

static BkrGenResult g_result;
static char g_spoiler[256 * 1024];

static int pick(const char* value, const char* const* names, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(value, names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static int usage(void) {
    fprintf(stderr,
            "uso: bkrgen --seed TEXTO [--logic glitchless|none] [--shuffle vanilla|type|mixed]\n"
            "            [--scope game|world] [--moves vanilla|molehills] [--access all|beatable]\n"
            "            [--code BKR1-XXXXXX-SEED] [--lang pt|en] [--spoiler arquivo.txt]\n");
    return 2;
}

int main(int argc, char** argv) {
    static const char* const logic[] = { "glitchless", "none" };
    static const char* const shuffle[] = { "vanilla", "type", "mixed" };
    static const char* const scope[] = { "game", "world" };
    static const char* const moves[] = { "vanilla", "molehills" };
    static const char* const access[] = { "all", "beatable" };
    BkrSeedSpec spec;
    const char* spoiler_path = NULL;
    const char* code = NULL;
    char share[BKR_SHARE_CODE_MAX];
    int lang = BKR_LANG_PT_BR;
    int i;
    int v;

    bkr_settings_default(&spec.settings);
    spec.seed[0] = '\0';
    for (i = 1; i < argc; i++) {
        const char* opt = argv[i];
        const char* val = (i + 1 < argc) ? argv[i + 1] : NULL;
        if (val == NULL) {
            return usage();
        }
        i++;
        if (strcmp(opt, "--seed") == 0) {
            bkr_seed_sanitize(val, spec.seed);
        } else if (strcmp(opt, "--code") == 0) {
            code = val;
        } else if (strcmp(opt, "--spoiler") == 0) {
            spoiler_path = val;
        } else if (strcmp(opt, "--lang") == 0) {
            lang = strcmp(val, "en") == 0 ? BKR_LANG_EN : BKR_LANG_PT_BR;
        } else if (strcmp(opt, "--logic") == 0 && (v = pick(val, logic, 2)) >= 0) {
            spec.settings.logic = (bkr_u8)v;
        } else if (strcmp(opt, "--shuffle") == 0 && (v = pick(val, shuffle, 3)) >= 0) {
            spec.settings.shuffle = (bkr_u8)v;
        } else if (strcmp(opt, "--scope") == 0 && (v = pick(val, scope, 2)) >= 0) {
            spec.settings.scope = (bkr_u8)v;
        } else if (strcmp(opt, "--moves") == 0 && (v = pick(val, moves, 2)) >= 0) {
            spec.settings.moves = (bkr_u8)v;
        } else if (strcmp(opt, "--access") == 0 && (v = pick(val, access, 2)) >= 0) {
            spec.settings.access = (bkr_u8)v;
        } else {
            return usage();
        }
    }
    if (code != NULL && !bkr_share_code_parse(code, &spec)) {
        fprintf(stderr, "código inválido: %s\n", code);
        return 2;
    }
    if (spec.seed[0] == '\0') {
        return usage();
    }
    if (!bkr_generate(&spec, &g_result)) {
        fprintf(stderr, "falha ao gerar após %u tentativas\n", g_result.attempts);
        return 1;
    }
    bkr_share_code_format(&spec, share, sizeof(share));
    printf("codigo=%s\n", share);
    printf("fingerprint=%08X\n", g_result.fingerprint);
    printf("save=bkr_%08X\n", g_result.fingerprint);
    printf("placement_hash=%08X\n", bkr_placement_hash(g_result.placement));
    printf("tentativas=%u\n", g_result.attempts);
    printf("hash=%s %s %s %s %s\n", bkr_hash_word(g_result.fingerprint), bkr_hash_word(g_result.fingerprint >> 5),
           bkr_hash_word(g_result.fingerprint >> 10), bkr_hash_word(g_result.fingerprint >> 15),
           bkr_hash_word(g_result.fingerprint >> 20));
    if (spoiler_path != NULL) {
        BkrText t;
        FILE* f = fopen(spoiler_path, "wb");
        if (f == NULL) {
            fprintf(stderr, "não consegui abrir %s\n", spoiler_path);
            return 1;
        }
        bkr_text_init(&t, g_spoiler, sizeof(g_spoiler));
        bkr_spoiler_write(&spec, &g_result, lang, &t);
        fwrite(g_spoiler, 1, t.len, f);
        fclose(f);
        printf("spoiler=%s\n", spoiler_path);
    }
    return 0;
}

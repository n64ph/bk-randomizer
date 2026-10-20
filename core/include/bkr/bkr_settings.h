/*
 * bkr_settings.h — Opções do randomizer, código de compartilhamento e
 * impressão digital do seed.
 *
 * Código de compartilhamento: "BKR1-XXXXXX-SEMENTE"
 *   - XXXXXX: 30 bits em base32 Crockford (24 bits de opções + 6 de checagem);
 *   - SEMENTE: [A-Z0-9_], 1..24 caracteres.
 */
#ifndef BKR_SETTINGS_H
#define BKR_SETTINGS_H

#include "bkr_types.h"

/* Versão do gerador. Incremente SEMPRE que o algoritmo de geração mudar. */
#define BKR_GENERATOR_VERSION 1u

enum { BKR_LOGIC_GLITCHLESS = 0, BKR_LOGIC_NONE = 1 };
enum { BKR_SHUFFLE_VANILLA = 0, BKR_SHUFFLE_BY_TYPE = 1, BKR_SHUFFLE_MIXED = 2 };
enum { BKR_SCOPE_GAME = 0, BKR_SCOPE_WORLD = 1 };
enum { BKR_MOVES_VANILLA = 0, BKR_MOVES_MOLEHILLS = 1 };
enum { BKR_ACCESS_ALL = 0, BKR_ACCESS_BEATABLE = 1 };

typedef struct BkrSettings {
    bkr_u8 logic;    /* BKR_LOGIC_* */
    bkr_u8 shuffle;  /* BKR_SHUFFLE_* (jiggies, favos e tokens) */
    bkr_u8 scope;    /* BKR_SCOPE_* */
    bkr_u8 moves;    /* BKR_MOVES_* */
    bkr_u8 access;   /* BKR_ACCESS_* */
    bkr_u8 pad[3];
} BkrSettings;

#define BKR_SEED_MAX 24u
#define BKR_SHARE_CODE_MAX 40u

typedef struct BkrSeedSpec {
    BkrSettings settings;
    char seed[BKR_SEED_MAX + 1];
} BkrSeedSpec;

void bkr_settings_default(BkrSettings* s);
int bkr_settings_valid(const BkrSettings* s);
bkr_u32 bkr_settings_pack(const BkrSettings* s);
int bkr_settings_unpack(bkr_u32 bits, BkrSettings* out);

/* Normaliza texto livre para semente: maiúsculas, espaço vira '_', descarta
 * o resto, corta em BKR_SEED_MAX. Retorna o tamanho (0 = inválida). */
bkr_u32 bkr_seed_sanitize(const char* in, char* out);

/* Formata/lê o código de compartilhamento. Retornam 1 em sucesso. */
int bkr_share_code_format(const BkrSeedSpec* spec, char* out, bkr_u32 cap);
int bkr_share_code_parse(const char* code, BkrSeedSpec* out);

/* Impressão digital: muda com versão do gerador, dados de lógica, opções e semente. */
bkr_u32 bkr_seed_fingerprint(const BkrSeedSpec* spec);

#endif

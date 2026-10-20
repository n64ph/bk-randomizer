/* bkr_settings.c — Opções, código de compartilhamento e impressão digital. */
#include "bkr/bkr_settings.h"
#include "bkr/bkr_hash.h"
#include "bkr/bkr_text.h"
#include "bkr/bkr_world.h"

static const char bkr_base32[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

void bkr_settings_default(BkrSettings* s) {
    s->logic = BKR_LOGIC_GLITCHLESS;
    s->shuffle = BKR_SHUFFLE_MIXED;
    s->scope = BKR_SCOPE_GAME;
    s->moves = BKR_MOVES_MOLEHILLS;
    s->access = BKR_ACCESS_ALL;
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
}

int bkr_settings_valid(const BkrSettings* s) {
    return s->logic <= BKR_LOGIC_NONE && s->shuffle <= BKR_SHUFFLE_MIXED &&
           s->scope <= BKR_SCOPE_WORLD && s->moves <= BKR_MOVES_MOLEHILLS &&
           s->access <= BKR_ACCESS_BEATABLE;
}

bkr_u32 bkr_settings_pack(const BkrSettings* s) {
    return ((bkr_u32)s->logic) | ((bkr_u32)s->shuffle << 1) | ((bkr_u32)s->scope << 3) |
           ((bkr_u32)s->moves << 4) | ((bkr_u32)s->access << 5);
}

int bkr_settings_unpack(bkr_u32 bits, BkrSettings* out) {
    if ((bits >> 6) != 0) {
        return 0;
    }
    out->logic = (bkr_u8)(bits & 1u);
    out->shuffle = (bkr_u8)((bits >> 1) & 3u);
    out->scope = (bkr_u8)((bits >> 3) & 1u);
    out->moves = (bkr_u8)((bits >> 4) & 1u);
    out->access = (bkr_u8)((bits >> 5) & 1u);
    out->pad[0] = out->pad[1] = out->pad[2] = 0;
    return bkr_settings_valid(out);
}

bkr_u32 bkr_seed_sanitize(const char* in, char* out) {
    bkr_u32 n = 0;
    while (*in != '\0' && n < BKR_SEED_MAX) {
        char c = *in;
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (c == ' ') {
            c = '_';
        }
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            out[n] = c;
            n++;
        }
        in++;
    }
    out[n] = '\0';
    return n;
}

static bkr_u32 bkr_settings_check(bkr_u32 bits) {
    return bkr_fnv1a_u32(BKR_FNV_OFFSET, bits) & 0x3Fu;
}

static int bkr_base32_value(char c) {
    int i;
    if (c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    }
    if (c == 'O') {
        c = '0';
    }
    if (c == 'I' || c == 'L') {
        c = '1';
    }
    for (i = 0; i < 32; i++) {
        if (bkr_base32[i] == c) {
            return i;
        }
    }
    return -1;
}

static void bkr_settings_block(const BkrSettings* s, char* out6) {
    bkr_u32 bits = bkr_settings_pack(s);
    bkr_u32 value = (bits & 0xFFFFFFu) | (bkr_settings_check(bits) << 24);
    int i;
    for (i = 5; i >= 0; i--) {
        out6[i] = bkr_base32[value & 31u];
        value >>= 5;
    }
    out6[6] = '\0';
}

int bkr_share_code_format(const BkrSeedSpec* spec, char* out, bkr_u32 cap) {
    BkrText t;
    char block[7];
    if (!bkr_settings_valid(&spec->settings) || spec->seed[0] == '\0') {
        return 0;
    }
    bkr_settings_block(&spec->settings, block);
    bkr_text_init(&t, out, cap);
    bkr_text_put(&t, "BKR1-");
    bkr_text_put(&t, block);
    bkr_text_putc(&t, '-');
    bkr_text_put(&t, spec->seed);
    return !t.overflow;
}

int bkr_share_code_parse(const char* code, BkrSeedSpec* out) {
    static const char prefix[] = "BKR1-";
    bkr_u32 value = 0;
    bkr_u32 bits;
    int i;
    for (i = 0; prefix[i] != '\0'; i++) {
        char c = code[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (c != prefix[i]) {
            return 0;
        }
    }
    code += 5;
    for (i = 0; i < 6; i++) {
        int v = bkr_base32_value(code[i]);
        if (v < 0) {
            return 0;
        }
        value = (value << 5) | (bkr_u32)v;
    }
    if (code[6] != '-') {
        return 0;
    }
    bits = value & 0xFFFFFFu;
    if ((value >> 24) != bkr_settings_check(bits)) {
        return 0;
    }
    if (!bkr_settings_unpack(bits, &out->settings)) {
        return 0;
    }
    return bkr_seed_sanitize(code + 7, out->seed) > 0;
}

bkr_u32 bkr_seed_fingerprint(const BkrSeedSpec* spec) {
    char block[7];
    bkr_u32 h = BKR_FNV_OFFSET;
    bkr_settings_block(&spec->settings, block);
    h = bkr_fnv1a_str(h, "BKR|");
    h = bkr_fnv1a_u32(h, BKR_GENERATOR_VERSION);
    h = bkr_fnv1a_u32(h, BKR_LOGIC_DATA_HASH);
    h = bkr_fnv1a_str(h, block);
    h = bkr_fnv1a_str(h, "|");
    h = bkr_fnv1a_str(h, spec->seed);
    return h;
}

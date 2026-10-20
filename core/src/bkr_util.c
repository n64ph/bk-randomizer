/* bkr_util.c — Utilidades sem libc: conjuntos de palavras e strings. */
#include "bkr/bkr_types.h"

void bkr_words_clear(bkr_u32* dst, bkr_u32 count) {
    bkr_u32 i;
    for (i = 0; i < count; i++) {
        dst[i] = 0;
    }
}

void bkr_words_copy(bkr_u32* dst, const bkr_u32* src, bkr_u32 count) {
    bkr_u32 i;
    for (i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

bkr_u32 bkr_strlen(const char* s) {
    bkr_u32 n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int bkr_streq(const char* a, const char* b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

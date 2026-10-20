/*
 * bkr_text.h — Construtor de texto em buffer fixo, sem libc.
 * Sempre mantém o buffer terminado em zero; marca overflow em vez de estourar.
 */
#ifndef BKR_TEXT_H
#define BKR_TEXT_H

#include "bkr_types.h"

typedef struct BkrText {
    char* buf;
    bkr_u32 cap;
    bkr_u32 len;
    int overflow;
} BkrText;

void bkr_text_init(BkrText* t, char* buf, bkr_u32 cap);
void bkr_text_putc(BkrText* t, char c);
void bkr_text_put(BkrText* t, const char* s);
void bkr_text_u32(BkrText* t, bkr_u32 value);
void bkr_text_hex(BkrText* t, bkr_u32 value, int digits);

#endif

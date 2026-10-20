/* bkr_text.c — Construtor de texto em buffer fixo (sem printf/libc). */
#include "bkr/bkr_text.h"

void bkr_text_init(BkrText* t, char* buf, bkr_u32 cap) {
    t->buf = buf;
    t->cap = cap;
    t->len = 0;
    t->overflow = 0;
    if (cap > 0) {
        buf[0] = '\0';
    }
}

void bkr_text_putc(BkrText* t, char c) {
    if (t->len + 1 < t->cap) {
        t->buf[t->len] = c;
        t->len++;
        t->buf[t->len] = '\0';
    } else {
        t->overflow = 1;
    }
}

void bkr_text_put(BkrText* t, const char* s) {
    while (*s != '\0') {
        bkr_text_putc(t, *s);
        s++;
    }
}

void bkr_text_u32(BkrText* t, bkr_u32 value) {
    char digits[10];
    int n = 0;
    do {
        digits[n] = (char)('0' + (value % 10u));
        n++;
        value /= 10u;
    } while (value != 0);
    while (n > 0) {
        n--;
        bkr_text_putc(t, digits[n]);
    }
}

void bkr_text_hex(BkrText* t, bkr_u32 value, int digits) {
    static const char hex[] = "0123456789ABCDEF";
    int shift;
    for (shift = (digits - 1) * 4; shift >= 0; shift -= 4) {
        bkr_text_putc(t, hex[(value >> shift) & 0xFu]);
    }
}

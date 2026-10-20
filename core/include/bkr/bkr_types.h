/*
 * bkr_types.h — Tipos básicos do núcleo portátil do randomizer.
 *
 * Regras do núcleo (valem para TODOS os arquivos em core/):
 *  - C99 puro, sem libc, sem <stdint.h> (o mod compila com -nostdinc);
 *  - sem ponto flutuante e sem divisão/módulo de 64 bits;
 *  - sem recursão e sem arrays grandes na pilha (a pilha da thread principal
 *    do jogo tem 0x17F0 bytes);
 *  - sem atribuição de structs grandes (o compilador poderia emitir memcpy).
 * O mesmo código compila no host (testes e CLI) e em MIPS (mod).
 */
#ifndef BKR_TYPES_H
#define BKR_TYPES_H

typedef unsigned char  bkr_u8;
typedef signed char    bkr_s8;
typedef unsigned short bkr_u16;
typedef signed short   bkr_s16;
typedef unsigned int   bkr_u32;
typedef signed int     bkr_s32;

#define BKR_NULL ((void*)0)

#define BKR_STATIC_ASSERT(name, cond) typedef char bkr_static_assert_##name[(cond) ? 1 : -1]
BKR_STATIC_ASSERT(u8_size, sizeof(bkr_u8) == 1);
BKR_STATIC_ASSERT(u16_size, sizeof(bkr_u16) == 2);
BKR_STATIC_ASSERT(u32_size, sizeof(bkr_u32) == 4);

/* Conjuntos de bits em arrays de bkr_u32. */
#define BKR_WORDS(n) (((n) + 31u) / 32u)
#define BKR_BIT_GET(arr, i) (((arr)[(i) >> 5] >> ((i) & 31u)) & 1u)
#define BKR_BIT_SET(arr, i) ((arr)[(i) >> 5] |= (1u << ((i) & 31u)))
#define BKR_BIT_CLR(arr, i) ((arr)[(i) >> 5] &= ~(1u << ((i) & 31u)))

/* Zera/copia palavras sem depender de memset/memcpy. */
void bkr_words_clear(bkr_u32* dst, bkr_u32 count);
void bkr_words_copy(bkr_u32* dst, const bkr_u32* src, bkr_u32 count);

/* Strings mínimas (sem libc). */
bkr_u32 bkr_strlen(const char* s);
int bkr_streq(const char* a, const char* b);

#endif

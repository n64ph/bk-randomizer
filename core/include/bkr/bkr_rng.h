/*
 * bkr_rng.h — Gerador pseudoaleatório determinístico (xoshiro128**),
 * semeado por splitmix32. Só usa operações de 32 bits, logo produz a mesma
 * sequência no host e no MIPS.
 */
#ifndef BKR_RNG_H
#define BKR_RNG_H

#include "bkr_types.h"

typedef struct BkrRng {
    bkr_u32 s[4];
} BkrRng;

/* Avança *x e retorna o próximo valor de splitmix32. */
bkr_u32 bkr_splitmix32(bkr_u32* x);
/* Inicializa o estado a partir de uma semente de 32 bits. */
void bkr_rng_seed(BkrRng* rng, bkr_u32 seed);
/* Próximo valor de 32 bits. */
bkr_u32 bkr_rng_next(BkrRng* rng);
/* Inteiro uniforme em [0, n) sem viés (n > 0). */
bkr_u32 bkr_rng_below(BkrRng* rng, bkr_u32 n);
/* Embaralha um array de bkr_u16 (Fisher–Yates). */
void bkr_rng_shuffle_u16(BkrRng* rng, bkr_u16* values, bkr_u32 count);

#endif

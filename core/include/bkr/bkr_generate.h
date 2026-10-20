/*
 * bkr_generate.h — Geração do seed: preenchimento assumido (assumed fill,
 * como Fill.py::fill_restrictive do OoT Randomizer) + verificação.
 */
#ifndef BKR_GENERATE_H
#define BKR_GENERATE_H

#include "bkr_types.h"
#include "bkr_world.h"
#include "bkr_settings.h"
#include "bkr_logic.h"

#define BKR_MAX_ATTEMPTS 40u

typedef struct BkrGenResult {
    bkr_u8 placement[BKR_LOCATION_COUNT];  /* item em cada localização */
    bkr_u32 fingerprint;
    bkr_u32 attempts;
    int ok;
} BkrGenResult;

/* 1 se a localização entra no embaralhamento com estas opções. */
int bkr_location_is_shuffled(const BkrSettings* s, bkr_u32 loc);
/* Gera o seed. Retorna 1 em sucesso (out->ok). Determinístico. */
int bkr_generate(const BkrSeedSpec* spec, BkrGenResult* out);
/* Verifica um placement completo: vencível e, se pedido, 100% alcançável. */
int bkr_verify(const BkrSettings* s, const bkr_u8* placement);
/* Hash do placement (comparação host x MIPS). */
bkr_u32 bkr_placement_hash(const bkr_u8* placement);

#endif

/*
 * bkr_spoiler.h — Spoiler em texto: esferas de coleta (playthrough) e
 * lista de localizações por mundo. Usado pela CLI no host.
 */
#ifndef BKR_SPOILER_H
#define BKR_SPOILER_H

#include "bkr_text.h"
#include "bkr_generate.h"

/* Escreve o spoiler completo. Retorna o número de esferas. */
bkr_u32 bkr_spoiler_write(const BkrSeedSpec* spec, const BkrGenResult* res, int lang, BkrText* out);

#endif

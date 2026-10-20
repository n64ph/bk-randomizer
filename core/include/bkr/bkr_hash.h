/*
 * bkr_hash.h — FNV-1a de 32 bits, usado em impressões digitais de seed,
 * nomes de arquivo de save e checagens de equivalência host/MIPS.
 */
#ifndef BKR_HASH_H
#define BKR_HASH_H

#include "bkr_types.h"

#define BKR_FNV_OFFSET 0x811C9DC5u
#define BKR_FNV_PRIME 0x01000193u

bkr_u32 bkr_fnv1a_bytes(bkr_u32 hash, const bkr_u8* data, bkr_u32 len);
bkr_u32 bkr_fnv1a_str(bkr_u32 hash, const char* text);
bkr_u32 bkr_fnv1a_u32(bkr_u32 hash, bkr_u32 value);

#endif

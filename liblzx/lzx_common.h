/*
 * lzx_common.h
 *
 * Declarations shared between LZX compression and decompression.
 */

#ifndef _LZX_COMMON_H
#define _LZX_COMMON_H

#include "lzx_constants.h"
#include "types.h"

extern const s32 lzx_offset_slot_base[LZX_MAX_OFFSET_SLOTS + 1];

extern const u8 lzx_extra_offset_bits[LZX_MAX_OFFSET_SLOTS];

unsigned
lzx_get_window_order(size_t max_bufsize);

unsigned
lzx_get_num_main_syms(unsigned window_order);

void
lzx_preprocess(u8 *data, u32 size, u32 chunk_offset, u32 e8_file_size);

void
lzx_postprocess(u8 *data, u32 size, u32 chunk_offset, u32 e8_file_size);

#endif /* _LZX_COMMON_H */

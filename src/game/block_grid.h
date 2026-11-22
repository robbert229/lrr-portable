// SPDX-License-Identifier: LGPL-3.0-or-later
/* lrr_block_index: a block grid index as the original's 32-bit code
 * resolved it, safe on 64-bit.
 *
 * The level's blocks (Lego_Level.blocks, width x height) and Map3D's
 * (TerrainMap.blocks, blockWidth x blockHeight) are indexed as
 * `width * y + x`. A NaN world position converts to INT_MIN block
 * coordinates (cvttss2si and the original's fistp both give 0x80000000).
 * On Win32 the index and the byte offset are 32-bit: width * INT_MIN +
 * INT_MIN times an even block size wraps to an offset of 0, so the
 * original reads block 0 and goes on. On 64-bit the same index is
 * zero-extended and reads gigabytes past the grid.
 *
 * The index arrives already wrapped to 32 bits, as the original computed
 * it. Every grid's Win32 element size is even, so adding 2^31 to an index
 * does not move the original's wrapped byte offset: the original ignores
 * bit 31, and so does this. An index inside the grid after that is used
 * as it is, including one that runs off the end of a row into the next, as
 * the original's arithmetic does; so are INT_MIN coordinates plus a small
 * offset (a neighbour of a NaN position), which the original reads as the
 * same small block. Any other index resolves to block 0: what the original
 * gives for the INT_MIN position itself, and a defined result where it
 * read outside the grid. */
#ifndef LRR_GAME_BLOCK_GRID_H
#define LRR_GAME_BLOCK_GRID_H

#include <stdint.h>

static inline uint32_t lrr_block_index(uint32_t index, uint32_t width, uint32_t height)
{
    index &= 0x7fffffffu;
    return index < width * height ? index : 0;
}

#ifdef __cplusplus
#include <type_traits>

/* Compute the index in unsigned 32-bit arithmetic, as the original's
 * wrapped. A signed index overflows into undefined behaviour where the
 * original wrapped (INT_MIN * width already does), so a signed index does
 * not compile, nor does one wider than 32 bits. Any unsigned 32-bit type
 * converts: unsigned int is not uint32_t on every target (newlib's ARM
 * uint32_t is unsigned long). */
template <typename T,
          typename std::enable_if<std::is_integral<T>::value &&
                                      (std::is_signed<T>::value || sizeof(T) > sizeof(uint32_t)),
                                  int>::type = 0>
uint32_t lrr_block_index(T index, uint32_t width, uint32_t height) = delete;
#endif

#endif /* LRR_GAME_BLOCK_GRID_H */

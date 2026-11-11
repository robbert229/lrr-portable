// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-026: BMP palette and pixel-data preparation. */
#ifndef LEGORRR_GODS98_G98_026_H
#define LEGORRR_GODS98_G98_026_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct IndexedBitmapPaletteData;
void indexed_bitmap_palette_data_prepare(const uint8_t* bitmap, uint32_t size,
                                         struct IndexedBitmapPaletteData* output);
void object_field_38_release_target(void* record);
#ifdef __cplusplus
}
#endif
#endif

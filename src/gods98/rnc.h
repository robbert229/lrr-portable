// SPDX-License-Identifier: LGPL-3.0-or-later
/* The RNC expander (original translation unit LATE-001, finding 610).
 *
 * C-compatible header (ADR 0005). rnc.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file: every include and declaration a header adds shifts MSVC's
 * symbol numbering (finding 506). Wad.c declares wad_expand_payload through
 * wad_payload.h, and resource.hpp declares the three functions the port
 * calls; src/game/test_c_boundaries.cpp checks both against this header.
 *
 * The decoder keeps its state in the unit's commons, so it is not
 * reentrant. No function checks its pointers. */
#ifndef LEGORRR_GODS98_RNC_H
#define LEGORRR_GODS98_RNC_H

/* One Huffman table entry, 16 bytes: the canonical code (bit-reversed) and
 * its length. The words at +0 and +4 are cleared and never read. */
typedef struct RNC_HuffEntry
{
    unsigned int unused0;      /* +0x0 */
    unsigned short unused4;     /* +0x4, cleared to 0xffff */
    unsigned int code;         /* +0x8 */
    unsigned short bitDepth;    /* +0xc */
} RNC_HuffEntry;

#ifdef __cplusplus
extern "C" {
#endif

/* 0049c7c0: the unpacked length (out_payload NULL), or unpack into a new
 * malloc'd buffer stored in *out_payload and return the length; 0 when the
 * allocation or the unpack fails. */
long wad_expand_payload(const void* header, void** out_payload);

/* 0049c840: check the "RNC" magic and unpack by method. 0 on success, -1
 * for a missing magic and -2 for an unknown method. */
short wad_payload_decode_dispatch(const void* header, void* dest);

/* 0049c8e0 and 0049ca60: unpack methods 1 and 2; both return 0. */
short wad_payload_decode_type1(const void* header, void* dest);
short wad_payload_decode_type2(const void* header, void* dest);

/* 0049cc70: point the decoder at a packed stream and its output. */
void wad_bitstream_output_initialize(const void* header, void* dest);

/* 0049ccc0 and 0049cd30: read bits, LSB-first (method 1) or MSB-first
 * (method 2). */
unsigned short wad_read_bits_lsb_first(unsigned char bits);
unsigned short wad_read_bits_msb_first(unsigned char bits);

/* 0049cd90: read one Huffman table of up to count entries. */
void wad_huffman_table_read(RNC_HuffEntry* table, unsigned char count);

/* 0049ce00: decode one Huffman symbol and its extra bits. */
unsigned short wad_huffman_symbol_decode(RNC_HuffEntry* table);

/* 0049ce70 and 0049ceb0: method 2's match length and match offset. */
unsigned short wad_run_length_decode(void);
unsigned short wad_backreference_offset_decode(void);

/* 0049cf50: clear count 16-byte table entries. */
void wad_huffman_table_clear(RNC_HuffEntry* table, unsigned char count);

/* 0049cf90: assign canonical codes to a table's entries by length. */
void wad_huffman_canonical_codes_build(RNC_HuffEntry* table, unsigned char count);

/* 0049d000: the low bits bits of value, reversed. */
unsigned int wad_reverse_low_bits(unsigned int value, unsigned char bits);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_RNC_H */

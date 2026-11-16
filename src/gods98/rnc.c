// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit LATE-001 (text 0049c7c0-0049d030, literal
 * 004b2d08; finding 610): the RNC (Rob Northen) ProPack expander that
 * Wad.c's _Wad_FileOpen (0048be40) calls for packed WAD entries. The file
 * name is unknown; the stem comes from the "RNC" magic.
 *
 * The original links this object after the import libraries (its code
 * follows the import thunks). It is C: the unit calls the C library's
 * malloc, free and strncmp directly, and every function is called only
 * from this unit, except wad_expand_payload, which Wad.c calls.
 *
 * The packed stream: "RNC", a method byte, the unpacked length (big-endian)
 * at +4, and the packed data from +0x12. Method 0 is stored, method 1 is
 * Huffman-coded and read LSB-first, method 2 is read MSB-first. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "rnc.h"

/* 52 bytes of initialised data that nothing in the image reads: the
 * original's .data has them here, after LIBC's data and before this
 * object's "RNC" literal. Their meaning is not known. */
// GLOBAL: LEGORR 0x004b2cd4
static unsigned char s_rncUnreferenced[52] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x08, 0x0a, 0x12, 0x13, 0x16, 0x00,
    0x00, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x00, 0x00, 0x06, 0x08, 0x09,
    0x15, 0x17, 0x1d, 0x1f, 0x28, 0x29, 0x2c, 0x2d, 0x38, 0x39, 0x3c, 0x3d,
    0x01, 0x03, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06
};

/* Commons: the decoder's shared, non-reentrant state. MSVC 5.0 orders an
 * object's symbol table by a hash of the names, and LINK lays out an
 * object's commons in reverse symbol-table order (finding 690). These names
 * give the original's order (00557d68-00558090); the original's own names
 * are not known. */
// GLOBAL: LEGORR 0x00557d68
unsigned char rnc_bitsLeft;
// GLOBAL: LEGORR 0x00557d80
RNC_HuffEntry rnc_rawTable[16];
// GLOBAL: LEGORR 0x00557e80
RNC_HuffEntry rnc_offsetTable[16];
// GLOBAL: LEGORR 0x00557f80
RNC_HuffEntry rnc_huffLen[16];
// GLOBAL: LEGORR 0x00558080
unsigned char* rnc_inPtr;
// GLOBAL: LEGORR 0x00558084
unsigned char* rnc_output;
// GLOBAL: LEGORR 0x00558088
unsigned char rnc_byteReg;
// GLOBAL: LEGORR 0x0055808c
unsigned int rnc_bitCache;
// GLOBAL: LEGORR 0x00558090
unsigned char* rnc_outputEnd;

/* The unpacked length: the big-endian dword at +4. The original reads it
 * as a signed 32-bit word (MSVC 5.0's long); it is read unsigned, so that
 * the shifts wrap without overflow, and 32 bits wide on every target. */
#define RNC_LONG uint32_t
#define RNC_LENGTH(l) (((((l) * 256 + (((l) >> 8) & 0xff)) * 256) + (((l) >> 16) & 0xff)) * 256 \
                       + ((unsigned int)(l) >> 24))

// FUNCTION: LEGORR 0x0049c7c0
long wad_expand_payload(const void* header, void** out_payload)
{
    unsigned int len = *(unsigned int*)((unsigned char*)header + 4);
    long size = ((len & 0xff) << 24) + ((len & 0xff00) << 8) + ((len >> 8) & 0xff00) + (len >> 24);
    void* buffer;

    if (out_payload == NULL)
        return size;
    if ((buffer = malloc(size)) != NULL) {
        if (wad_payload_decode_dispatch(header, buffer)) {
            free(buffer);
            return 0;
        }
        *out_payload = buffer;
        return size;
    }
    return 0;
}

// FUNCTION: LEGORR 0x0049c840
short wad_payload_decode_dispatch(const void* header, void* dest)
{
    const unsigned char* src = (const unsigned char*)header;
    RNC_LONG len;
    unsigned short result;

    // STRING: LEGORR 0x004b2d08
    if (strncmp((const char*)src, "RNC", 3))
        return -1;
    switch (src[3]) {
    case 0:
        len = *(RNC_LONG*)(src + 4);
        memcpy(dest, src + 0x12, RNC_LENGTH(len));
        result = 0;
        break;
    case 1:
        result = wad_payload_decode_type1(header, dest);
        break;
    case 2:
        result = wad_payload_decode_type2(header, dest);
        break;
    default:
        result = 0xfffe;
        break;
    }
    return result;
}

// FUNCTION: LEGORR 0x0049c8e0
short wad_payload_decode_type1(const void* header, void* dest)
{
    unsigned short blocks;
    unsigned short count;
    unsigned char* from;

    wad_bitstream_output_initialize(header, dest);
    wad_read_bits_lsb_first(2);
    while (rnc_output < rnc_outputEnd) {
        wad_huffman_table_read(rnc_rawTable, 16);
        wad_huffman_table_read(rnc_offsetTable, 16);
        wad_huffman_table_read(rnc_huffLen, 16);
        blocks = (unsigned short)wad_read_bits_lsb_first(16);
        for (;;) {
            count = (unsigned short)wad_huffman_symbol_decode(rnc_rawTable);
            while (count--)
                *rnc_output++ = *rnc_inPtr++;
            rnc_bitCache = ((unsigned int)((((rnc_inPtr[2] << 8) + rnc_inPtr[1]) << 8) + rnc_inPtr[0]) << rnc_bitsLeft)
                          + (((1 << rnc_bitsLeft) - 1) & rnc_bitCache);
            if (--blocks == 0)
                break;
            from = rnc_output - (unsigned short)wad_huffman_symbol_decode(rnc_offsetTable) - 1;
            count = (unsigned short)(wad_huffman_symbol_decode(rnc_huffLen) + 2);
            while (count--)
                *rnc_output++ = *from++;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0049ca60
short wad_payload_decode_type2(const void* header, void* dest)
{
    unsigned short count;
    unsigned short offset;
    unsigned char* from;

    wad_bitstream_output_initialize(header, dest);
    wad_read_bits_msb_first(2);
    while (rnc_output < rnc_outputEnd) {
        for (;;) {
            while (!wad_read_bits_msb_first(1))
                *rnc_output++ = *rnc_inPtr++;
            if (wad_read_bits_msb_first(1)) {
                if (!wad_read_bits_msb_first(1)) {
                    count = 2;
                    offset = *rnc_inPtr++ + 1;
                } else {
                    if (!wad_read_bits_msb_first(1)) {
                        count = 3;
                    } else {
                        count = *rnc_inPtr++ + 8;
                        if (count == 8)
                            break;
                    }
                    offset = wad_backreference_offset_decode();
                }
                from = rnc_output - offset;
                while (count--)
                    *rnc_output++ = *from++;
            } else {
                count = wad_run_length_decode();
                if (count == 9) {
                    count = wad_read_bits_msb_first(4) * 4 + 12;
                    while (count--)
                        *rnc_output++ = *rnc_inPtr++;
                } else {
                    from = rnc_output - wad_backreference_offset_decode();
                    while (count--)
                        *rnc_output++ = *from++;
                }
            }
        }
        wad_read_bits_msb_first(1);
    }
    return 0;
}

/* 44%: this source matches exactly when compiled alone or without any one
 * of the four functions above it, but in the full unit MSVC 5.0 keeps dest
 * in ecx (the original: esi) and borrows ebx for the length's bytes. The
 * choice does not follow the symbol counter (finding 538): extra
 * declarations, functions, labels or lines before it leave it unchanged.
 * Finding 610 lists the shapes tried. */
// FUNCTION: LEGORR 0x0049cc70
void wad_bitstream_output_initialize(const void* header, void* dest)
{
    RNC_LONG len;

    rnc_inPtr = (unsigned char*)header + 0x12;
    rnc_output = (unsigned char*)dest;
    len = *(RNC_LONG*)((unsigned char*)header + 4);
    rnc_bitsLeft = 0;
    rnc_outputEnd = (unsigned char*)dest + RNC_LENGTH(len);
}

// FUNCTION: LEGORR 0x0049ccc0
unsigned short wad_read_bits_lsb_first(unsigned char bits)
{
    unsigned int value = 0;
    unsigned int bit = 1;

    while (bits--) {
        if (!rnc_bitsLeft) {
            rnc_bitCache = *(unsigned int*)rnc_inPtr;
            rnc_inPtr += 2;
            rnc_bitsLeft = 16;
        }
        if (rnc_bitCache & 1)
            value |= bit;
        bit <<= 1;
        rnc_bitCache >>= 1;
        rnc_bitsLeft--;
    }
    return value;
}

// FUNCTION: LEGORR 0x0049cd30
unsigned short wad_read_bits_msb_first(unsigned char bits)
{
    unsigned int value = 0;

    while (bits--) {
        if (!rnc_bitsLeft) {
            rnc_byteReg = *rnc_inPtr++;
            rnc_bitsLeft = 8;
        }
        value <<= 1;
        if (rnc_byteReg & 0x80)
            value++;
        rnc_byteReg <<= 1;
        rnc_bitsLeft--;
    }
    return value;
}

// FUNCTION: LEGORR 0x0049cd90
void wad_huffman_table_read(RNC_HuffEntry* table, unsigned char count)
{
    unsigned char n;
    unsigned char i;

    wad_huffman_table_clear(table, count);
    if ((n = (unsigned char)wad_read_bits_lsb_first(5))) {
        if (n > 16)
            n = 16;
        for (i = 0; i < n; i++)
            table[i].bitDepth = wad_read_bits_lsb_first(4);
        wad_huffman_canonical_codes_build(table, n);
    }
}

// FUNCTION: LEGORR 0x0049ce00
unsigned short wad_huffman_symbol_decode(RNC_HuffEntry* table)
{
    RNC_HuffEntry* entry = table;
    unsigned char i = 0;

    while (!entry->bitDepth || (((1 << entry->bitDepth) - 1) & rnc_bitCache) != entry->code) {
        entry++;
        i++;
    }
    wad_read_bits_lsb_first((unsigned char)entry->bitDepth);
    if (i < 2)
        return i;
    return (unsigned short)(wad_read_bits_lsb_first((unsigned char)(i - 1)) | (1 << (i - 1)));
}

// FUNCTION: LEGORR 0x0049ce70
unsigned short wad_run_length_decode(void)
{
    unsigned short len = wad_read_bits_msb_first(1) + 4;

    if (!wad_read_bits_msb_first(1))
        return len;
    return wad_read_bits_msb_first(1) - 2 + len * 2;
}

// FUNCTION: LEGORR 0x0049ceb0
unsigned short wad_backreference_offset_decode(void)
{
    unsigned short offset = 0;

    if (wad_read_bits_msb_first(1)) {
        offset = (unsigned short)wad_read_bits_msb_first(1);
        if (wad_read_bits_msb_first(1)) {
            offset = (unsigned short)(((offset * 2) + wad_read_bits_msb_first(1)) | 4);
            if (!wad_read_bits_msb_first(1))
                offset = (unsigned short)((offset * 2) + wad_read_bits_msb_first(1));
        } else if (!offset) {
            offset = (unsigned short)(wad_read_bits_msb_first(1) + 2);
        }
    }
    return (unsigned short)((offset << 8) + *rnc_inPtr++ + 1);
}

// FUNCTION: LEGORR 0x0049cf50
void wad_huffman_table_clear(RNC_HuffEntry* table, unsigned char count)
{
    while (count--) {
        table->unused0 = 0;
        table->unused4 = 0xffff;
        table->code = 0;
        table->bitDepth = 0;
        table++;
    }
}

// FUNCTION: LEGORR 0x0049cf90
void wad_huffman_canonical_codes_build(RNC_HuffEntry* table, unsigned char count)
{
    RNC_HuffEntry* entry;
    unsigned short bitDepth = 1;
    unsigned int value = 0;
    unsigned int divisor = 0x80000000;
    unsigned short i;

    do {
        entry = table;
        for (i = 0; i < count; i++, entry++) {
            if (entry->bitDepth == bitDepth) {
                entry->code = wad_reverse_low_bits(value / divisor, (unsigned char)bitDepth);
                value += divisor;
            }
        }
        bitDepth++;
        divisor >>= 1;
    } while (bitDepth <= 16);
}

// FUNCTION: LEGORR 0x0049d000
unsigned int wad_reverse_low_bits(unsigned int value, unsigned char bits)
{
    unsigned int result = 0;

    while (bits--) {
        result <<= 1;
        if (value & 1)
            result |= 1;
        value >>= 1;
    }
    return result;
}

/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * The DirectInput data-format layout used by dilib1.c and dilib2.c, from the
 * public DirectInput 5 interface (DIOBJECTDATAFORMAT, DIDATAFORMAT and the
 * DIDFT_ and DIDF_ flags). Self-contained, so the import-library build needs
 * no SDK header.
 */
#ifndef LRR_DINPUT_FORMATS_H
#define LRR_DINPUT_FORMATS_H

typedef struct DiGuid {
    unsigned long data1;
    unsigned short data2;
    unsigned short data3;
    unsigned char data4[8];
} DiGuid;

typedef struct DiObjectDataFormat {
    const DiGuid *pguid;
    unsigned long dwOfs;
    unsigned long dwType;
    unsigned long dwFlags;
} DiObjectDataFormat;

typedef struct DiDataFormat {
    unsigned long dwSize;
    unsigned long dwObjSize;
    unsigned long dwFlags;
    unsigned long dwDataSize;
    unsigned long dwNumObjs;
    DiObjectDataFormat *rgodf;
} DiDataFormat;

#define DIDFT_AXIS 0x00000003UL
#define DIDFT_BUTTON 0x0000000CUL
#define DIDFT_ANYINSTANCE 0x00FFFF00UL
#define DIDFT_OPTIONAL 0x80000000UL
#define DIDFT_MAKEINSTANCE(n) ((unsigned long)(n) << 8)
#define DIDF_RELAXIS 0x00000002UL

#endif

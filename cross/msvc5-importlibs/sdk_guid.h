/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * The SDK's GUID libraries define their GUIDs with DEFINE_GUID under
 * INITGUID: each becomes an external const GUID in .rdata, in definition
 * order (finding 688). MSVC 5.0's own SDK headers predate most of these
 * GUIDs, so the two library sources spell them out with this macro.
 */
#ifndef LRR_SDK_GUID_H
#define LRR_SDK_GUID_H

typedef struct SdkGuid {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} SdkGuid;

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const SdkGuid name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

#endif

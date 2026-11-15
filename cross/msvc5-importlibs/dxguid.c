/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * dxguid.lib members (finding 574): the DirectInput GUIDs that dilib1.c and
 * dilib2.c reference, and GUID_SysKeyboard, which Input.c (G98-013) passes
 * to CreateDevice (finding 683). The SDK library holds one GUID per member, in .rdata;
 * the build compiles this file once per GUID, naming it with DXGUID_<name>.
 * The values are the public DirectInput 5 GUIDs and equal the original's
 * .rdata bytes (GUID_XAxis at 004a0088, GUID_Key at 004a0108,
 * GUID_SysKeyboard at 004a0148).
 */
#include "dinput_formats.h"

#define DI_GUID(name, l) \
    const DiGuid name = { l, 0xC9F3, 0x11CF, \
        { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } }

#if defined(DXGUID_GUID_XAxis)
DI_GUID(GUID_XAxis, 0xA36D02E0);
#elif defined(DXGUID_GUID_YAxis)
DI_GUID(GUID_YAxis, 0xA36D02E1);
#elif defined(DXGUID_GUID_ZAxis)
DI_GUID(GUID_ZAxis, 0xA36D02E2);
#elif defined(DXGUID_GUID_Key)
const DiGuid GUID_Key = { 0x55728220, 0xD33C, 0x11CF,
    { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
#elif defined(DXGUID_GUID_SysKeyboard)
const DiGuid GUID_SysKeyboard = { 0x6F1D2B61, 0xD5A0, 0x11CF,
    { 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };
#else
#error "define DXGUID_<name> to choose the GUID"
#endif

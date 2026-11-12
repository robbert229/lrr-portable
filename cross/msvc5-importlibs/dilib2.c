/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * dinput.lib member dilib2.obj: c_dfDIKeyboard (finding 574). The original
 * keeps it in .text at 0049b790-0049c7a7, after c_dfDIMouse and before the
 * DirectInputCreateA thunk: one optional button per scan code 0-255, then the
 * DIDATAFORMAT. The values follow the public DirectInput 5 layout and equal
 * the original's bytes there.
 */
#include "dinput_formats.h"

extern const DiGuid GUID_Key;

#define KEY(n) { &GUID_Key, (n), DIDFT_BUTTON | DIDFT_OPTIONAL | DIDFT_MAKEINSTANCE(n), 0 }
#define KEY4(n) KEY(n), KEY((n) + 1), KEY((n) + 2), KEY((n) + 3)
#define KEY16(n) KEY4(n), KEY4((n) + 4), KEY4((n) + 8), KEY4((n) + 12)
#define KEY64(n) KEY16(n), KEY16((n) + 16), KEY16((n) + 32), KEY16((n) + 48)

#pragma data_seg(".text")
static DiObjectDataFormat c_rgodfDIKeyboard[256] = {
    KEY64(0), KEY64(64), KEY64(128), KEY64(192),
};

DiDataFormat c_dfDIKeyboard = {
    sizeof(DiDataFormat), sizeof(DiObjectDataFormat), DIDF_RELAXIS, 256, 256,
    c_rgodfDIKeyboard,
};
#pragma data_seg()

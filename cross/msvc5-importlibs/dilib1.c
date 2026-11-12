/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * dinput.lib member dilib1.obj: c_dfDIMouse (finding 574). The original keeps
 * it in .text at 0049b700-0049b787, after the avifil32 import thunks: the
 * seven object formats, then the DIDATAFORMAT. The values follow the public
 * DirectInput 5 layout and equal the original's bytes there. The SDK object
 * puts the data in .text, as this data_seg does.
 */
#include "dinput_formats.h"

extern const DiGuid GUID_XAxis;
extern const DiGuid GUID_YAxis;
extern const DiGuid GUID_ZAxis;

#pragma data_seg(".text")
static DiObjectDataFormat c_rgodfDIMouse[7] = {
    { &GUID_XAxis, 0, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
    { &GUID_YAxis, 4, DIDFT_AXIS | DIDFT_ANYINSTANCE, 0 },
    { &GUID_ZAxis, 8, DIDFT_AXIS | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0 },
    { 0, 12, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, 13, DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0 },
    { 0, 14, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0 },
    { 0, 15, DIDFT_BUTTON | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0 },
};

DiDataFormat c_dfDIMouse = {
    sizeof(DiDataFormat), sizeof(DiObjectDataFormat), DIDF_RELAXIS, 16, 7,
    c_rgodfDIMouse,
};
#pragma data_seg()

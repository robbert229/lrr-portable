// SPDX-License-Identifier: LGPL-3.0-or-later
/* DirectX error decoder (original translation unit G98-027, finding 505).
 *
 * C-compatible header (ADR 0005). dxerror.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). The C++ declarations in resource.hpp mirror these;
 * src/game/test_c_boundaries.cpp checks that they agree. */
#ifndef LEGORRR_GODS98_DXERROR_H
#define LEGORRR_GODS98_DXERROR_H

/* The module argument of directx_result_diagnostic_dispatch. */
#define DXERROR_MODULE_UNKNOWN 0
#define DXERROR_MODULE_D3DRM 1
#define DXERROR_MODULE_DINPUT 2
#define DXERROR_MODULE_DDRAW 3
#define DXERROR_MODULE_DSOUND 4
#define DXERROR_MODULE_DPLAY 5
#define DXERROR_MODULE_D3DIM 6
#define DXERROR_MODULE_IUNKNOWN 7

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00489d90: formats the last recorded result, its name and description
 * into the decoder's 2,048-byte report buffer. */
void directx_error_message_format(const char* name, const char* description);

/* FUN_00489dd0: records err, module (0 when outside 1..7), file and line,
 * counts the call, formats the report for err and returns err. */
int directx_result_diagnostic_dispatch(int err, int module, const char* file,
                                       int line);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_DXERROR_H */

// SPDX-License-Identifier: LGPL-3.0-or-later
/* platform.h — the portability seam.
 *
 * Game-code below this line is platform-neutral. The only thing that varies
 * by backend is which header set satisfies the DX6-era API surface recorded
 * in docs/findings/15-compat-audit.md §1:
 *
 *   native : miniwin (https://github.com/isledecomp/isle-portable miniwin/)
 *            — source-compatible shim over SDL3, primary end-goal build
 *   winapi : the real DirectX headers from MinGW-w64 — the reference build,
 *            A/B-compared against the shipped binary under the A.2 harness
 *
 * Keep this file shallow: re-export the platform's windows/directdraw headers
 * under one roof. Do not put game logic here (docs/adr/0003).
 */
#ifndef LEGORRR_PLATFORM_H
#define LEGORRR_PLATFORM_H

#if defined(LEGORRR_BACKEND_MINIWIN)
#  include <miniwin/windows.h>
#elif defined(LEGORRR_BACKEND_WINAPI)
#  include <windows.h>
#else
#  error "no backend defined — set meson option backend=native|winapi"
#endif

#endif /* LEGORRR_PLATFORM_H */
// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native entry point — primary end-goal build (miniwin over SDL3).
 *
 * This is a *build smoke test*, not a game shell: it proves the cross-… no,
 * *host* toolchain, miniwin adaptation, static-lib linkage and the platform
 * seam all compile and link into a runnable native binary, which writes an
 * observable log and exits.
 *
 * Replace `LegorrrMain`'s body with the real startup once the other agent
 * lands the decompiled engine init (findings 02/03).
 */
#include "platform/platform.h"

/* Shared game-shell stub — lives in src/main.cpp, backend-neutral. */
int LegorrrMain(void);

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    return LegorrrMain();
}
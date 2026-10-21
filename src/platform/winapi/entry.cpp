// SPDX-License-Identifier: LGPL-3.0-or-later
/* Win32 entry point — the reference build.
 *
 * Mirrors the shipped binary's GUI-subsystem entry (WinMain, -mwindows). Used
 * to A/B our rebuild against the shipped LegoRR.exe under the A.2 harness
 * (tools/compat/a2.py, docs/adr/0002). The stub writes a log and exits;
 * replace `LegorrrMain`'s body when the decompiled startup lands.
 */
#include "platform/platform.h"

/* Shared game-shell stub — lives in src/main.cpp, backend-neutral. */
int LegorrrMain(void);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return LegorrrMain();
}
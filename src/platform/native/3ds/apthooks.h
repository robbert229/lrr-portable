// SPDX-License-Identifier: LGPL-3.0-or-later
/* Nintendo 3DS: the system's sleep, suspend, wake, restore and exit
 * notifications (APT hooks) as the application lifecycle
 * (platform/native/lifecycle.h). Adapted from isle-portable's
 * ISLE/3ds/apthooks.h (LGPL-3.0). */
#ifndef LEGORRR_PLATFORM_NATIVE_N3DS_APTHOOKS_H
#define LEGORRR_PLATFORM_NATIVE_N3DS_APTHOOKS_H

#include <3ds.h>

void N3DS_AptHookCallback(APT_HookType hookType, void* param);
/* Installs the hook with libctru. The hook runs on the thread that calls
 * aptMainLoop, which SDL's event pump does. */
void N3DS_SetupAptHooks();

#endif /* LEGORRR_PLATFORM_NATIVE_N3DS_APTHOOKS_H */

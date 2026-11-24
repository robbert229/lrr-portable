// SPDX-License-Identifier: LGPL-3.0-or-later
/* Nintendo 3DS: the system's sleep, suspend, wake, restore and exit
 * notifications as the application lifecycle. Adapted from isle-portable's
 * ISLE/3ds/apthooks.cpp (LGPL-3.0), which pauses and resumes the game and
 * closes its main window. Here the platform reports the event to
 * platform/native/lifecycle.h, which drives the game's own paths for losing
 * and regaining focus and for a close request. */
#include "platform/native/3ds/apthooks.h"

#include "platform/native/lifecycle.h"

namespace {

aptHookCookie g_aptCookie;

} // namespace

void N3DS_AptHookCallback(APT_HookType hookType, void* param)
{
    (void) param;
    switch (hookType) {
    case APTHOOK_ONSLEEP:
    case APTHOOK_ONSUSPEND:
        LRR_ReportLifecycle(LRR_LIFECYCLE_SUSPEND);
        break;
    case APTHOOK_ONWAKEUP:
    case APTHOOK_ONRESTORE:
        LRR_ReportLifecycle(LRR_LIFECYCLE_RESUME);
        break;
    case APTHOOK_ONEXIT:
        LRR_ReportLifecycle(LRR_LIFECYCLE_EXIT);
        break;
    default:
        break;
    }
}

void N3DS_SetupAptHooks()
{
    aptHook(&g_aptCookie, N3DS_AptHookCallback, NULL);
}

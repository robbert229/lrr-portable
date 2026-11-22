// SPDX-License-Identifier: LGPL-3.0-or-later
/* Header inline copy that closes the registry unit's object (finding 694).
 *
 * A ghost ADVAPI32 import: a member whose only reference is code /OPT:REF
 * removed. Its slot takes part in LINK's qsort of the import table and is
 * then dropped. The original's IAT order needs one after RegCloseKey in
 * this unit's ADVAPI32 run. The unit queries values and never sets one, so
 * a removed set-value function calling RegSetValueExA is the likely
 * carrier; the name is not evidenced.
 *
 * As in sound3d_tail.h (finding 693), an unused `extern __inline` copy
 * carries the reference and /OPT:REF removes its code. Nothing calls it.
 * Only the matching build needs it. */
#ifndef LEGORRR_GODS98_G98_029_TAIL_H
#define LEGORRR_GODS98_G98_029_TAIL_H
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <windows.h>
extern __inline LONG registry_tail_set_value(HKEY key, const char* name,
                                             const void* data, DWORD size)
{
    return RegSetValueExA(key, name, 0, REG_BINARY, (const BYTE*)data, size);
}
#endif
#endif

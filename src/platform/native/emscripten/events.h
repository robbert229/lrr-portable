// SPDX-License-Identifier: LGPL-3.0-or-later
/* The web page's game events, as isle-portable's ISLE/emscripten/events.h:
 * DOM CustomEvents dispatched on the canvas, which the page that hosts the
 * game listens for (to back up or export saves, for example). Declared on
 * every host; defined only in the web build (events.cpp). */
#ifndef LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_EVENTS_H
#define LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_EVENTS_H

/* "dataProgress", {"name": file, "progress": percent}: the percentage of a
 * game data file (a WAD) fetched so far while the web build preloads it
 * (filesystem.cpp), as isle-portable's "extensionProgress". */
void Emscripten_SendDataProgress(const char* p_name, int p_progress);
/* "dataReady": the game data is set up (and preloaded, if the build does). */
void Emscripten_SendDataReady();
/* "dataError", {"message": text}: the game data could not be set up (no
 * manifest on the host, say). The game then runs without it. */
void Emscripten_SendDataError(const char* p_message);
/* "saveSlotWritten", {"slot": n}: a slot's progress was written. */
void Emscripten_SendSaveSlotWritten(int p_slot);
/* "saveStateChanged": the set of slots changed (one was created or reset). */
void Emscripten_SendSaveStateChanged();

#endif /* LEGORRR_PLATFORM_NATIVE_EMSCRIPTEN_EVENTS_H */

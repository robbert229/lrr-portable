// SPDX-License-Identifier: LGPL-3.0-or-later
// The app's activity (issue #927), as isle-portable's IsleActivity
// (android-project, LGPL-3.0-or-later): SDL's activity, which loads the
// libraries below and runs the last one's SDL_main (src/platform/native/
// entry.cpp) on SDL's main thread, where the game's WinMain loop blocks.
package io.github.robbert229.LegoRR;

import org.libsdl.app.SDLActivity;

public class LegoRRActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        // libc++_shared first: before Android 6.0 the loader does not find
        // an app library's dependencies in the app's own directory.
        return new String[] { "c++_shared", "SDL3", "LegoRR" };
    }
}

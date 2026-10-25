# SPDX-License-Identifier: LGPL-3.0-or-later
# CMake equivalent of i686.ini for the 32-bit Win32 reference build.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
set(CMAKE_AR i686-w64-mingw32-ar)
set(CMAKE_STRIP i686-w64-mingw32-strip)

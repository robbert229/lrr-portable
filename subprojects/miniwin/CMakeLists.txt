# SPDX-License-Identifier: LGPL-3.0-or-later
# Thin bridge so meson's `cmake.subproject('miniwin')` finds the miniwin
# sources inside the sparse isle-portable submodule. miniwin's own top-level
# CMakeLists lives at 3rdparty/isle-portable/miniwin/CMakeLists.txt.
cmake_minimum_required(VERSION 3.16)
project(miniwin-bridge CXX)

find_package(SDL3 REQUIRED)

# Stub-simplicity: enable ONLY the software renderer so the seam is proven
# without pulling miniwin's SDL GPU backend / shader pipeline into the first
# build. Flip ON when the 3D path needs it (docs/adr/0004).
set(ISLE_RENDERER_SDLGPU OFF CACHE BOOL "" FORCE)
set(ISLE_RENDERER_SOFTWARE ON CACHE BOOL "" FORCE)

add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/isle-portable/miniwin
  miniwin_build
)

# miniwin's CMakeLists defines the `miniwin` static library (and miniwin-headers);
# nothing else needed here.
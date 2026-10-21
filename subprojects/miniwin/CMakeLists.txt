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

# miniwin's OpenGL 1/ES backends reference raw gl*/egl* entry points, which
# the GLVND "libOpenGL.so" translation lib does NOT export — they live in
# mesa's libGL.so. Ask CMake for the legacy library so the link line carries
# -lGL (find_package(OpenGL) must run after this preference).
set(OpenGL_GL_PREFERENCE LEGACY)

# miniwin's CMakeLists auto-enables its OpenGL/OpenGL-ES d3drm backends when
# the libs are found on the host (find_package(OpenGL) / find_library GLESv2)
# and links them PRIVATE — a link dependency meson's cmake bridge cannot
# propagate, so the game exe would link against their gl* symbols without the
# library. Nail those lookups closed to keep the software-only build
# deterministic (the author's reference machine has no GL/GLES2 dev libs).
set(CMAKE_DISABLE_FIND_PACKAGE_OpenGL TRUE CACHE BOOL "" FORCE)
set(OPENGL_ES2_LIBRARY "" CACHE FILEPATH "GLESv2 (disabled — software renderer only)")
set(OPENGL_ES3_LIBRARY "" CACHE FILEPATH "GLESv3 (disabled — software renderer only)")

add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/isle-portable/miniwin
  miniwin_build
)

# miniwin's CMakeLists defines the `miniwin` static library (and miniwin-headers).
# Upstream marks miniwin EXCLUDE_FROM_ALL (platform backends are opt-in), which
# makes meson's cmake.subproject — it builds the default `all` target — produce
# an *empty* archive (nothing references miniwin until the frame loop does).
# Pull the library back into `all` so meson builds the real one.
set_target_properties(miniwin PROPERTIES EXCLUDE_FROM_ALL OFF)
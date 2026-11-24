# SPDX-License-Identifier: LGPL-3.0-or-later
# The tests of the units' C runtime headers (src/crt).
# CMakeLists.txt includes this file inside if(BUILD_TESTING), after
# cmake/LrrTesting.cmake; docs/building.md, "Writing a test".

# The units' vsprintf prints "%b" as VC5 LIBC does, on every host.
# vc5names.h compares without case through SDL.
lrr_unit_test(safe_format SOURCES src/crt/test_safe_format.cpp
  INCLUDES ${CMAKE_SOURCE_DIR}/src
  LINK SDL3::SDL3
  LABELS crt)

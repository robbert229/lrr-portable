# SPDX-License-Identifier: LGPL-3.0-or-later
# MSVC 5.0 CMake toolchain used by the optional Reccmp build.

if(NOT LRR_MSVC5_ROOT)
  message(FATAL_ERROR "Set LRR_MSVC5_ROOT to the unpacked MSVC 5.0 toolchain")
endif()
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES LRR_MSVC5_ROOT)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER "${LRR_MSVC5_ROOT}/wine/x86/cl")
set(CMAKE_CXX_COMPILER "${LRR_MSVC5_ROOT}/wine/x86/cl")
set(CMAKE_RC_COMPILER "${LRR_MSVC5_ROOT}/wine/x86/rc")
set(CMAKE_AR "${LRR_MSVC5_ROOT}/wine/x86/lib")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

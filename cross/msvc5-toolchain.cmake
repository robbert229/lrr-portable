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

# LINK.EXE runs on the toolchain's own MSVCRT, not Wine's builtin: its qsort
# orders the import address table (finding 694). The launcher is written into
# the build tree because CMake's link rule passes no extra arguments. The
# top-level CMakeLists.txt makes it CMAKE_LINKER after project(), which
# otherwise resets CMAKE_LINKER to the compiler's sibling.
set(LRR_MSVC5_LINK_STAGE "${CMAKE_BINARY_DIR}/msvc5-native-crt")
file(WRITE "${LRR_MSVC5_LINK_STAGE}/link"
  "#!/bin/sh\nexec \"${CMAKE_CURRENT_LIST_DIR}/msvc5-native-crt-link.sh\" \"${LRR_MSVC5_ROOT}\" \"${LRR_MSVC5_LINK_STAGE}\" \"$@\"\n")
file(CHMOD "${LRR_MSVC5_LINK_STAGE}/link"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(LRR_MSVC5_LINK "${LRR_MSVC5_LINK_STAGE}/link")
set(CMAKE_LINKER "${LRR_MSVC5_LINK}")

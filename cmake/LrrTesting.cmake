# SPDX-License-Identifier: LGPL-3.0-or-later
# lrr_unit_test: one test executable and its ctest registration.
#
#   lrr_unit_test(<name>
#     SOURCES <file>...            # required
#     [LINK <library>...]          # replaces the default, units; a bare LINK
#                                  # links nothing
#     [WRAP <symbol>...]           # LINKER:--wrap=<symbol> for each
#     [INCLUDES <dir>...]          # extra include directories
#     [DEFINES <definition>...]    # extra compile definitions
#     [ARGS <arg>...]              # the test's command-line arguments
#     [LABELS <label>...]
#     [ENVIRONMENT <VAR=value>...]
#     [SKIP_RETURN_CODE <code>])
#
# It builds <name>_test from SOURCES and adds the ctest test <name>, which
# runs it with ARGS. src/test is on the include path, for lrr_test.h.
#
# WRAP needs GNU ld's --wrap, which neither Apple's linker nor the Windows
# linkers have. Where the toolchain cannot wrap (not GCC or Clang, Apple, or
# Windows), a test with WRAP is left out: neither built nor registered.
#
# More tests on the same executable (other arguments or environments) are
# plain add_test calls on <name>_test after this one.

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE AND NOT WIN32)
  set(LRR_CAN_WRAP TRUE)
else()
  set(LRR_CAN_WRAP FALSE)
endif()

function(lrr_unit_test name)
  cmake_parse_arguments(PARSE_ARGV 1 arg ""
    "SKIP_RETURN_CODE"
    "SOURCES;LINK;WRAP;INCLUDES;DEFINES;ARGS;LABELS;ENVIRONMENT")
  if(arg_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "lrr_unit_test(${name}): unknown arguments ${arg_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT arg_SOURCES)
    message(FATAL_ERROR "lrr_unit_test(${name}): SOURCES is required")
  endif()
  if(arg_WRAP AND NOT LRR_CAN_WRAP)
    return()
  endif()

  set(target ${name}_test)
  add_executable(${target} ${arg_SOURCES})
  target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src/test" ${arg_INCLUDES})
  if(arg_DEFINES)
    target_compile_definitions(${target} PRIVATE ${arg_DEFINES})
  endif()
  if(DEFINED arg_LINK OR "LINK" IN_LIST arg_KEYWORDS_MISSING_VALUES)
    if(arg_LINK)
      target_link_libraries(${target} PRIVATE ${arg_LINK})
    endif()
  else()
    target_link_libraries(${target} PRIVATE units)
  endif()
  foreach(symbol IN LISTS arg_WRAP)
    target_link_options(${target} PRIVATE "LINKER:--wrap=${symbol}")
  endforeach()

  add_test(NAME ${name} COMMAND ${target} ${arg_ARGS})
  if(arg_LABELS)
    set_tests_properties(${name} PROPERTIES LABELS "${arg_LABELS}")
  endif()
  if(arg_ENVIRONMENT)
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "${arg_ENVIRONMENT}")
  endif()
  if(DEFINED arg_SKIP_RETURN_CODE)
    set_tests_properties(${name} PROPERTIES SKIP_RETURN_CODE ${arg_SKIP_RETURN_CODE})
  endif()
endfunction()

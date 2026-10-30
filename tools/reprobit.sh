#!/usr/bin/env bash

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
toolchain_root="${REPROBIT_MSVC_5_0_RTM_ROOT:-${HOME}/.local/share/reprobit/toolchains/msvc_5_0_rtm}"

rbit source preview "${project_root}"
rbit source lock "${project_root}"

# rbit import cmake "${project_root}" \
#   --cmake-define PLATFORM=winapi \
#   --cmake-define LRR_CLASSIC_MSVC5=ON

GIT_CEILING_DIRECTORIES="${project_root}/.reprobit-state/runs" \
REPROBIT_MSVC_5_0_RTM_ROOT="${toolchain_root}" \
rbit import cmake "${project_root}" \
  --refresh --cmake cmake --configuration RelWithDebInfo --timeout 600 \
  --cmake-define PLATFORM=winapi \
  --cmake-define LRR_CLASSIC_MSVC5=ON \
  --directive-input program=libc.lib \
  --directive-input program=oldnames.lib

REPROBIT_MSVC_5_0_RTM_ROOT="${toolchain_root}" \
  rbit build "${project_root}"

REPROBIT_MSVC_5_0_RTM_ROOT="${toolchain_root}" \
  rbit repair "${project_root}"
#

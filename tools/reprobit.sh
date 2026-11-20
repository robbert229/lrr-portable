#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Bring the matching build under ReproBit (rbit) and build it: the source
# lock, the CMake graph import and an incremental build. Run it where Wine,
# CMake and an MSVC 5.0 capable rbit are on PATH (RBIT selects another rbit).
# reference/ must be a real directory holding a copy of LegoRR.exe, not a
# symlink.
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
rbit="${RBIT:-rbit}"
toolchain_root="${REPROBIT_MSVC_5_0_RTM_ROOT:-${HOME}/.local/share/reprobit/toolchains/msvc_5_0_rtm}"
# ReproBit refuses a directory reached under two names, such as /home and
# /var/home on Fedora Atomic (where a container may see both), so spell the
# toolchain's home the way the project's path spells it.
if [[ "${project_root}" == /var/home/* && "${toolchain_root}" == /home/* ]]; then
  toolchain_root="/var${toolchain_root}"
fi
toolchain=(--toolchain-root "${toolchain_root}")

# The matching build's inputs; the source lock keeps this selection.
"${rbit}" source lock "${project_root}" \
  --path src --path CMakeLists.txt --path cross/msvc5-importlibs

# libc.lib and oldnames.lib are on the link line; the C++ units' objects
# also name libcp.lib as a default library.
GIT_CEILING_DIRECTORIES="${project_root}/.reprobit-state/runs" \
"${rbit}" import cmake "${project_root}" "${toolchain[@]}" \
  --configuration RelWithDebInfo \
  --cmake-define PLATFORM=winapi \
  --cmake-define LRR_CLASSIC_MSVC5=ON \
  --cmake-define LRR_ORIGINAL_UNIT_FLAGS=ON \
  --cmake-define BUILD_TESTING=OFF \
  --directive-input program=libcp.lib

"${rbit}" build "${project_root}" "${toolchain[@]}"

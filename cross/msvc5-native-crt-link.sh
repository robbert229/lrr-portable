#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# LINK.EXE for the matching build, run on the toolchain's own MSVCRT 5.00
# instead of Wine's builtin msvcrt. LINK orders each DLL's import address
# table with qsort, and Wine's qsort orders equal keys differently from
# Microsoft's (finding 694). msvc5_native_crt.py stages the copies.
#
# usage: msvc5-native-crt-link.sh TOOLCHAIN_ROOT STAGE_DIR [link arguments]
set -euo pipefail

toolchain_root=$1
stage_dir=$2
shift 2

python3 "$(dirname -- "${BASH_SOURCE[0]}")/msvc5_native_crt.py" \
  "${toolchain_root}/bin" "${stage_dir}"

# msvcenv.sh finds the toolchain from $0, so run it with $0 set to the
# toolchain's own link wrapper; wine-msvc.sh then runs the staged copy.
exec bash -c '. "$(dirname -- "$0")/msvcenv.sh"
exec "$(dirname -- "$0")/wine-msvc.sh" "$@"' \
  "${toolchain_root}/wine/x86/link" "${stage_dir}/link.exe" "$@"

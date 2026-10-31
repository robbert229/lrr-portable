#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
toolchain_root="${REPROBIT_MSVC_5_0_RTM_ROOT:-${HOME}/.local/share/reprobit/toolchains/msvc_5_0_rtm}"
build_dir="${RECCMP_BUILD_DIR:-${project_root}/build-reccmp}"
wine_bin="${RECCMP_WINE_BIN:-$(command -v wine || true)}"
reccmp_bin="${RECCMP_RECCMP:-$(command -v reccmp-reccmp || true)}"
winepath_exe="${RECCMP_WINEPATH_EXE:-}"

if [[ "${build_dir}" != /* ]]; then
  build_dir="${project_root}/${build_dir}"
fi

if [[ ! -x "${toolchain_root}/wine/x86/cl" ]]; then
  echo "MSVC 5.0 compiler wrapper not found under ${toolchain_root}" >&2
  exit 2
fi
if [[ -z "${wine_bin}" || ! -x "${wine_bin}" ]]; then
  echo "Wine was not found; set RECCMP_WINE_BIN to its executable path" >&2
  exit 2
fi
if [[ -z "${reccmp_bin}" || ! -x "${reccmp_bin}" ]]; then
  echo "reccmp-reccmp was not found; activate the environment where Reccmp is installed" >&2
  exit 2
fi

export PATH="$(dirname -- "${wine_bin}"):${PATH}"
winepath_shim_dir=""
if ! command -v winepath >/dev/null 2>&1; then
  if [[ -z "${winepath_exe}" ]]; then
    wine_bin_dir="$(dirname -- "${wine_bin}")"
    for candidate in \
      "${wine_bin_dir}/../lib/wine/x86_64-windows/winepath.exe" \
      "${wine_bin_dir}/../lib/wine/i386-windows/winepath.exe" \
      "${wine_bin_dir}/winepath.exe"; do
      if [[ -f "${candidate}" ]]; then
        winepath_exe="${candidate}"
        break
      fi
    done
  fi
  if [[ -n "${winepath_exe}" && -f "${winepath_exe}" ]]; then
    winepath_shim_dir="$(mktemp -d)"
    cat > "${winepath_shim_dir}/winepath" <<'EOF'
#!/usr/bin/env bash
exec "${RECCMP_WINE_BIN}" "${RECCMP_WINEPATH_EXE}" "$@"
EOF
    chmod +x "${winepath_shim_dir}/winepath"
    export RECCMP_WINE_BIN="${wine_bin}"
    export RECCMP_WINEPATH_EXE="${winepath_exe}"
    export PATH="${winepath_shim_dir}:${PATH}"
    trap 'rm -rf -- "${winepath_shim_dir}"' EXIT
  fi
fi
cd "${project_root}"

cmake -S "${project_root}" -B "${build_dir}" \
  -DCMAKE_TOOLCHAIN_FILE="${project_root}/cross/msvc5-toolchain.cmake" \
  -DLRR_MSVC5_ROOT="${toolchain_root}" \
  -DPLATFORM=winapi \
  -DLRR_CLASSIC_MSVC5=ON \
  -DLRR_RECCMP=ON \
  -DBUILD_TESTING=OFF \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "${build_dir}" --target program

cat > "${project_root}/reccmp-build.yml" <<EOF
project: .
targets:
  LEGORR:
    path: ${build_dir}/LegoRR.exe
    pdb: ${build_dir}/LegoRR.pdb
EOF

if [[ ! -s "${build_dir}/LegoRR.exe" || ! -s "${build_dir}/LegoRR.pdb" ]]; then
  echo "Build completed without the expected LegoRR.exe and LegoRR.pdb outputs" >&2
  exit 1
fi

report_dir="${build_dir}/reccmp-report"
mkdir -p "${report_dir}"
"${reccmp_bin}" --target LEGORR \
  --html "${report_dir}/index.html" \
  --json "${report_dir}/results.json"
"${reccmp_bin}" --target LEGORR --nolib \
  --html "${report_dir}/game-only.html" \
  --json "${report_dir}/game-only.json"
python3 "${project_root}/tools/clean_reccmp_report_names.py" "${report_dir}"
echo "Reccmp HTML report: ${report_dir}/index.html"
echo "Reccmp game-only report: ${report_dir}/game-only.html"

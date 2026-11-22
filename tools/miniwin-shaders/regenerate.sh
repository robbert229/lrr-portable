#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Regenerates miniwin's SDL GPU shader headers on Linux, with the commands
# miniwin's ISLE_COMPILE_SHADERS build runs, and ShaderIndex.{h,cpp} for
# every shader. Commit the result in the submodule and export the patches
# (tools/miniwin-patches.sh export).
#
#   tools/miniwin-shaders/regenerate.sh <Name.stage>...
#
# Name.stage is a shader in miniwin/src/d3drm/backends/sdl3gpu/shaders/src
# without .hlsl, such as ImmediatePixel.frag. Shaders not named keep their
# headers, so a new shader does not rewrite the others' bytes.
#
# Needs:
#   SHADERCROSS  SDL_shadercross's CLI, built against DXC
#                (DirectXShaderCompiler's release libdxcompiler and libdxil
#                on LD_LIBRARY_PATH)
#   vkd3d-utils  libvkd3d-utils.so.1 (Fedora: libvkd3d-utils), for DXBC,
#                on LD_LIBRARY_PATH or the linker's path
#   cc, python3
set -euo pipefail
cd "$(dirname "$0")/../.."
[ $# -gt 0 ] || { sed -n '3,22p' "$0" >&2; exit 2; }
SHADERCROSS="${SHADERCROSS:-shadercross}"
SHADERS=3rdparty/isle-portable/miniwin/src/d3drm/backends/sdl3gpu/shaders
SRC="$SHADERS/src"
GEN="$SHADERS/generated"
PY="$SHADERS/gencshadersource.py"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

VKD3D_DIR=""
for dir in ${LD_LIBRARY_PATH//:/ } /usr/lib64 /usr/lib /usr/lib/x86_64-linux-gnu; do
    if [ -e "$dir/libvkd3d-utils.so.1" ]; then VKD3D_DIR="$dir"; break; fi
done
[ -n "$VKD3D_DIR" ] || { echo "libvkd3d-utils.so.1 not found" >&2; exit 1; }
cc -O1 -o "$WORK/dxbc" tools/miniwin-shaders/dxbc.c -L"$VKD3D_DIR" -Wl,-rpath,"$VKD3D_DIR" \
    -Wl,-rpath-link,"$VKD3D_DIR" -l:libvkd3d-utils.so.1

for name in "$@"; do
    case "$name" in
    *.vert) stage=vertex profile=vs_5_0 ;;
    *.frag) stage=fragment profile=ps_5_0 ;;
    *) echo "$name: expected Name.vert or Name.frag" >&2; exit 2 ;;
    esac
    src="$SRC/$name.hlsl"
    out="$WORK/$name"
    "$SHADERCROSS" "$src" -o "$out.dxbc" -s HLSL -d DXBC -t "$stage" -I "$SRC"
    "$SHADERCROSS" "$src" -o "$out.d3d11.hlsl" -s HLSL -d HLSL -t "$stage" --pssl -DNO_REGISTER_SPACE -I "$SRC"
    "$WORK/dxbc" "$profile" "$out.d3d11.hlsl" "$out.d3d11.dxbc"
    "$SHADERCROSS" "$src" -o "$out.dxil" -s HLSL -d DXIL -t "$stage" -I "$SRC"
    "$SHADERCROSS" "$src" -o "$out.msl" -s HLSL -d MSL -t "$stage" -I "$SRC"
    "$SHADERCROSS" "$src" -o "$out.spv" -s HLSL -d SPIRV -t "$stage" -I "$SRC"
    python3 "$PY" header --output "$GEN/$name.h" --stage "$stage" --variable "${name//./_}" \
        --dxbc "$out.dxbc" --dxbc-d3d11 "$out.d3d11.dxbc" --dxil "$out.dxil" --msl "$out.msl" --spirv "$out.spv"
done

# The index lists every shader, in the order miniwin/CMakeLists.txt does.
names=() variables=() headers=() stages=() jsons=()
for src in $(sed -n '/^set(miniwin_shaders$/,/^)/s|.*"${shader_src_dir}/\(.*\)\.hlsl".*|\1|p' \
    3rdparty/isle-portable/miniwin/CMakeLists.txt); do
    names+=("${src%.*}")
    variables+=("${src//./_}")
    headers+=("$GEN/$src.h")
    case "$src" in *.vert) stages+=(vertex) ;; *) stages+=(fragment) ;; esac
    jsons+=("$SRC/$src.hlsl.json")
done
python3 "$PY" index --output "$GEN/ShaderIndex.cpp" --header "$GEN/ShaderIndex.h" \
    --shader-names "${names[@]}" --shader-variables "${variables[@]}" --shader-headers "${headers[@]}" \
    --shader-stages "${stages[@]}" --shader-jsons "${jsons[@]}"
echo "regenerated: $* and ShaderIndex"

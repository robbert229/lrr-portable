project('lrr-portable', 'cpp',
  version : '0.1.0',
  meson_version : '>= 1.0.0',
  default_options : [
    'buildtype=debug',
    'warning_level=2',       # -Wall -Wextra on GCC/Clang
    'cpp_std=c++17',
    'b_ndebug=if-release',
  ])

# --- Backend selection (see docs/adr/0004) -----------------------------
#   native : Linux/macOS via miniwin (SDL3) — the primary end-goal build.
#   winapi : 32-bit Windows via MinGW + real DX headers — the reference build,
#            used to A/B our rebuild against the shipped binary under the A.2
#            harness (docs/adr/0002, 0005).
# Choice is fixed at setup time (a meson option, defaulting to 'native');
# the winapi backend additionally needs the cross/i686.ini cross file.
platform = get_option('platform')

if platform == 'native'
  add_project_arguments('-DLEGORRR_BACKEND_MINIWIN=1', language : 'cpp')
elif platform == 'winapi'
  add_project_arguments('-DLEGORRR_BACKEND_WINAPI=1', language : 'cpp')
else
  error('unknown platform: ' + platform)
endif

# `werror` is a Meson built-in option (off by default). Decompiled/reconstructed
# code is structurally noisy (opaque casts, dead stores); warnings are surfaced
# via -Wall -Wextra (warning_level=2) but errors opt-in: `-Dwerror=true`.

# Reconstruction source tree. Holds compile-checking stubs today; the other
# agent's decompiled modules land inside it later (docs/adr/0003).
subdir(get_option('src_root'))
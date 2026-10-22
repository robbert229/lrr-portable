# SPDX-License-Identifier: LGPL-3.0-or-later
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
#            harness (docs/adr/0002, 0004).
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

# --- AVI movie runtime (finding 14 §4 / 23) --------------------------------
# Movie playback is a *runtime* capability: src/gods98/movie.* dlopens the
# FFmpeg shared libraries on first use (no link dependency, no headers —
# LGPL-clean). Meson owns the soname lists and passes them to the module and
# to a configure-time probe, so the build knows — and the user is told —
# whether movies will play, and `-Dmovies=enabled` makes that a hard
# requirement. Details in meson_options.txt.
cpp = meson.get_compiler('cpp')
movies = get_option('movies')

# Single source of truth for the FFmpeg runtime libraries: newest-first,
# followed by an unversioned name. The probe and the module both consume
# these as ';'-joined compile defines (module falls back to Linux names when
# built outside meson).
if host_machine.system() == 'darwin'
  # Homebrew keeps formula libraries in its opt prefix (which differs between
  # Apple Silicon and Intel). Embed that prefix when available so both the
  # configure probe and runtime dlopen can find dylibs without DYLD_LIBRARY_PATH.
  ffmpeg_lib_prefix = ''
  brew = find_program('brew', required : false)
  if brew.found()
    brew_ffmpeg_prefix = run_command(brew, '--prefix', 'ffmpeg', check : false)
    if brew_ffmpeg_prefix.returncode() == 0
      ffmpeg_lib_prefix = brew_ffmpeg_prefix.stdout().strip() / 'lib'
    endif
  endif
  movie_lib_names = {
    'LEGORRR_MOVIE_LIBAVFORMAT' : ['libavformat.63.dylib', 'libavformat.62.dylib',
                                   'libavformat.61.dylib', 'libavformat.dylib'],
    'LEGORRR_MOVIE_LIBAVCODE'   : ['libavcodec.63.dylib',  'libavcodec.62.dylib',
                                   'libavcodec.61.dylib',  'libavcodec.dylib'],
    'LEGORRR_MOVIE_LIBAVUTIL'   : ['libavutil.61.dylib',   'libavutil.60.dylib',
                                   'libavutil.59.dylib',   'libavutil.dylib'],
    'LEGORRR_MOVIE_LIBSWSCALE'  : ['libswscale.10.dylib',  'libswscale.9.dylib',
                                   'libswscale.8.dylib',   'libswscale.dylib'],
  }
  movie_libs = {}
  foreach name, candidates : movie_lib_names
    paths = []
    if ffmpeg_lib_prefix != ''
      foreach candidate : candidates
        paths += [ffmpeg_lib_prefix / candidate]
      endforeach
    endif
    paths += candidates
    movie_libs += {name : paths}
  endforeach
else
  movie_libs = {
    'LEGORRR_MOVIE_LIBAVFORMAT' : ['libavformat.so.63', 'libavformat.so.62',
                                   'libavformat.so.61', 'libavformat.so'],
    'LEGORRR_MOVIE_LIBAVCODE'   : ['libavcodec.so.63',  'libavcodec.so.62',
                                   'libavcodec.so.61',  'libavcodec.so'],
    'LEGORRR_MOVIE_LIBAVUTIL'   : ['libavutil.so.61',   'libavutil.so.60',
                                   'libavutil.so.59',   'libavutil.so'],
    'LEGORRR_MOVIE_LIBSWSCALE'  : ['libswscale.so.10',  'libswscale.so.9',
                                   'libswscale.so.8',   'libswscale.so'],
  }
endif
movie_defines = []
foreach name, list : movie_libs
  movie_defines += '-D@0@="@1@"'.format(name, ';'.join(list))
endforeach

# dlopen/dlsym live in libc on glibc >= 2.34 (and musl); older glibc needs
# libdl. Probe for it and link when present (a no-op stub on modern glibc).
movie_dl_dep = cpp.find_library('dl', required : false)

if platform == 'native'
  add_project_arguments(movie_defines, language : 'cpp')

  if movies.auto() or movies.enabled()
    # Configure-time probe: compile + run a tiny dlopen of the same soname
    # lists the module uses at runtime, so the build reports (or, when the
    # option is 'enabled', requires) a working FFmpeg runtime up front.
    movie_probe_args = movie_defines
    if movie_dl_dep.found()
      movie_probe_args += ['-ldl']
    endif
    movie_probe = cpp.run('''
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
static int probe(const char* j)
{
    if (j == 0) return 0;
    char b[256];
    size_t n = strlen(j), i = 0;
    while (i < n) {
        while (i < n && j[i] == ';') ++i;
        if (i == n) break;
        size_t k = 0;
        while (i < n && j[i] != ';' && k + 1 < sizeof(b)) b[k++] = j[i++];
        b[k] = 0;
        if (dlopen(b, RTLD_LAZY | RTLD_LOCAL) != 0) return 1;
    }
    return 0;
}
int main(void)
{
    if (!probe(LEGORRR_MOVIE_LIBAVFORMAT)) return 1;
    if (!probe(LEGORRR_MOVIE_LIBAVCODE)) return 1;
    if (!probe(LEGORRR_MOVIE_LIBAVUTIL)) return 1;
    if (!probe(LEGORRR_MOVIE_LIBSWSCALE)) return 1;
    return 0;
}
''', args : movie_probe_args)
    movie_found = movie_probe.returncode() == 0
  else
    movie_found = false
  endif

  if movies.disabled()
    add_project_arguments('-DLEGORRR_MOVIES_CFG_DISABLED=1', language : 'cpp')
    message('movies: disabled by option (-Dmovies=disabled)')
  elif movies.enabled() and not movie_found
    error('movies=enabled but the FFmpeg runtime is not dlopen-able on this '
          + 'host (probed ' + ';'.join(movie_defines) + ')')
  elif movie_found
    message('movies: FFmpeg runtime found (configure-time dlopen probe) — '
            + 'playback enabled')
  else
    message('movies: FFmpeg runtime not found — playback disabled at runtime')
  endif
else
  # winapi reference build: the real game plays AVIs through AVIFIL32 on
  # Windows; the port module is a stub there, so the option is inert.
  if not movies.auto()
    message('movies: option ignored on the winapi reference build '
            + '(real AVIFIL32 at runtime)')
  endif
endif

# `werror` is a Meson built-in option (off by default). Decompiled/reconstructed
# code is structurally noisy (opaque casts, dead stores); warnings are surfaced
# via -Wall -Wextra (warning_level=2) but errors opt-in: `-Dwerror=true`.

# Reconstruction source tree. Holds compile-checking stubs today; the other
# agent's decompiled modules land inside it later (docs/adr/0003).
subdir(get_option('src_root'))

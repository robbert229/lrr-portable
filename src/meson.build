# Reconstruction source tree.
#
# Today: one compiling stub translation unit per planned subsystem so `meson
# compile` proves the toolchain, backend seam, static-lib split, resources
# and executable shell end to end. Later the other agent's decompiled modules
# land inside the existing subdirectories (docs/adr/0003).
#
# Subsystems mirror the gods98 codeline:
#   gods98/    engine core (Main, Viewports, Images, Input, Sounds, Movies…)
#   game/      game layer (entities, missions, commands, selection…)
#   platform/  the portability seam — backend entry points + platform.h

proj_inc = include_directories('.')

subdir('gods98')
subdir('game')
subdir('platform')   # defines platform_entry, platsub_deps, miniwin_inc

platform = get_option('platform')
if platform == 'native'
  exe_kwargs = {}
  extra_sources = []
elif platform == 'winapi'
  windows = import('windows')
  exe_kwargs = {'win_subsystem' : 'windows'}
  extra_sources = [windows.compile_resources('version.rc')]
else
  error('unknown platform: ' + platform)
endif

executable('LegoRR',
  sources : ['main.cpp', platform_entry, extra_sources],
  include_directories : [proj_inc, miniwin_inc],
  dependencies : [platsub_deps, gods98_dep, game_dep],
  install : true,
  kwargs : exe_kwargs)

# Output name stays `LegoRR` (becomes LegoRR.exe on the winapi reference
# build) so the A.2 harness can launch it from the staged game dir by the
# exact case-sensitive name it expects (tools/compat/a2.py, docs/adr/0002).
# The native build produces the same-named ELF for drop-in A/B runs.
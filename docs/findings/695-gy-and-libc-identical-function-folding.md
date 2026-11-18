# 695. The original's units were compiled with `/Gy`; its folded LIBC copies are not ICF over the game

Part of the identity work in PR #861. Follows findings 637, 642 and 692.
Measured with scratch compiles and relinks of the matching build's objects.
Nothing under `build-reccmp/` or the sources was changed.

## Question

The original folds four identical LIBC function pairs into one copy each:
- `_CallMemberFunction1`/`2` into `0`;
- `_matherr` into a no-op;
- `__fpclear`.

That is 48 bytes of `.text` (finding 692). The gate links with
`/OPT:NOICF`, so the rebuild keeps them apart and calls each copy.
`/OPT:ICF` folds them, but it also folds six groups of the game's own
identical functions, which the original keeps apart. Every function in
those groups is a 100% match at its own original address:
- `engine_memory_allocate`/`file_malloc`;
- `engine_memory_free`/`engine_heap_free_wrapper`/`ui_render_record_free`;
- `always_accept_place_command_cell`/`flic_opcode25_success_stub`;
- `radarmap_set_zoom`/`scene_current_object_set_original`;
- `mesh_get_group_count`/`scene_object_wrapped_pointer_get`/`file_get_system`.

LINK folds only COMDATs. So the obvious explanation would be game units
compiled without `/Gy`, with LIBC (built with `/Gy`) folded by
`/OPT:ICF`.

## Results

1. **MSVC 5.0 ignores `/Gy-`.** Appending it after `/O2` still emits one
   no-duplicates COMDAT per function. A relink of such objects with
   `/OPT:ICF` folds the game's groups exactly as before.
2. **`/O2` spelled out without `/Gy` gives one plain `.text` per object.**
   The flags are `/Og /Oi /Ot /Oy /Ob1 /Gs /Gf`. Only `extern __inline`
   copies, and C++ members defined in a class, stay COMDATs. The code is
   nearly unchanged: in aitask.c, frontend_unit.c, legoobject_unit.c and
   files.c, 581 of 582 functions are byte-identical with relocations
   masked. The exception, `legoobject_turn_to_vector_activity_issue`
   (`00444fb0`), schedules its x87 code differently and is 10 bytes
   shorter. Its `/Gy` build fills the original's 0x230-byte slot; the
   other build does not. (Finding 692's "9 functions drop" came from
   `/Ox`, not from dropping `/Gy`.)
3. **Without `/Gy`, `/OPT:ICF` folds only LIBC.** With all 212 units
   relinked this way, it folds `_CallMemberFunction0`/`1`/`2`, `_strncnt`
   and the three port-only `operator delete` members in avi_player.cpp.
   It folds none of the game groups.
4. **But the layout says the original used `/Gy`.** Inside a non-COMDAT
   `.text`, MSVC 5.0 does not keep every function on its own 16-byte
   boundary, so some functions are packed. The padded-size gaps against
   the original grow from 45 functions (net −384 bytes) to 55 (net −393).
   The new gaps are mostly odd-sized (−7, −1, +3): lego.c's
   `lego_default_transform_apply_*`, `lego_selected_*`,
   `lego_misc_object_resources_load`, camera_unit's `camera_update` and
   `camera_move`, and `lego_panel_selection_highlight_toggle`. None of them
   has a gap under `/Gy`. The original's function starts are on 16-byte
   boundaries in 2,961 of 3,022 annotated functions, the COMDAT layout.
   (`.text` is 315 bytes short instead of 224, and 284 annotated functions
   are at their original address instead of 250. That second count is a
   side effect of the shorter image, not a better match.)

## Conclusion

The original compiled its units with `/Gy`, and a link option did not
fold the game's identical functions apart from LIBC's. The folded LIBC
copies most likely come from a different `LIBC.LIB`, for example a Visual
C++ 5.0 service pack's, in which `trnsctrl.obj`, `matherr.obj` and the
`__fpclear` member are already shared. The RTM `libc.lib` the toolchain
supplies has separate 7-byte `_CallMemberFunction0`/`1`/`2` COMDATs
(select type 1). The 48 bytes stay open; the gate keeps `/O2` (with
`/Gy`) and `/OPT:NOICF`.

## Reproduce

The scripts are in the coordinator's scratchpad
(`icf/allgy.py`, `icf/relink.py`, `icf/cmp.py`):
1. Compile every `build-match/cmake/compile_commands.json` entry with `/O2`
   replaced by its expansion without `/Gy`.
2. Rebuild `late_init`, `late_rnc`, `lrr_tail` and `unit_gods98` from their
   own response files.
3. Relink with `program.dir/link.txt`, changing `/OPT:NOICF` to `/OPT:ICF`.
   Paths in the response files need Wine's `z:` prefix, because LINK reads
   a path that starts with `/` as an option.
4. Compare the map's multi-name addresses and the per-function padded sizes
   with the original's.

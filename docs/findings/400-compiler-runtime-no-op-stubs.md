# Finding 400 — Compiler/Runtime No-op Stubs

This finding records entries previously mistaken for no-op stubs. Instruction-level comparison identifies the public runtime entries and private helpers that implement their behavior; no unresolved CRT no-op entry remains in this group.

| Address | Name | Observed body | Caller/state flow |
| --- | --- | --- | --- |
| `0x00491ff5` | `__load_CW` | Exact VC5 `LIBC.LIB` `common.obj` match; masks the requested rounding bits, adds the standard exception masks, and loads the x87 control word. | The reference callers pass the saved control word as one cdecl argument. The MSVC build links the public runtime entry directly. |
| `0x004931c0` | `__except1` | Exact VC5 `LIBC.LIB` `fpexcept.obj` match outside relocation fields; the MSVC build now calls this entry directly instead of compiling a second implementation. | Called from the x87 exception-dispatch adapter after it extracts the caller's status and operation context. |
| `0x00493c40` | `__set_statfp` | Exact VC5 `LIBC.LIB` `fpctrl.obj` match. It executes x87 operations selected by a status-byte argument to generate floating-point exceptions. | VC5's `__handle_exc` calls it with status bits on these paths. The portable fallback remains a no-op because it does not model VC5's x87 exception machinery. |
| `0x00497770` | `__control87` | Exact VC5 `LIBC.LIB` `ieee87.obj` match; merges abstract control flags and updates the x87 control word. | Its private `__abstract_cw` and `__hw_cw` dependencies are included by the public runtime object. The MSVC build links this entry directly; see Finding 399. |
| `0x00497870` | `__hw_cw` | Private `ieee87.obj` converter included through `__control87`; not independently linkable. | Converts abstract control flags back to the hardware control word. |
| `0x004939d0` | `__matherr` | The target's operation-id lookup supplies the `__matherr` symbol used by VC5 `fpexcept.obj`'s `__umatherr`. This deliberately overrides `matherr.obj`'s default `xor eax,eax; ret` stub, which would always report no handler. | VC5 LIBC.LIB symbol and relocation records show `__umatherr` calls `__matherr`; target disassembly shows the lookup body and handler table. |

## Data paths and boundaries

- The `0x491ff5` callers pass a saved control word to `__load_CW`; the former no-argument stub signature lost that argument and the x87 side effect.
- VC5's `__handle_exc` calls `__set_statfp` (`0x00493c40`) with status bits on selected branches; the portable fallback does not model those hardware exception triggers.
- `__control87` calls private `__abstract_cw` and `__hw_cw` helpers from `ieee87.obj`; those helpers are not separately exported, but the public runtime call carries their implementations into the MSVC build.
- VC5's `matherr.obj` defines a no-op `__matherr`, but the target executable's `0x004939d0` operation-id lookup is the implementation expected by `fpexcept.obj`'s `__umatherr`. The MSVC CRT shim now exports that exact C-linkage symbol so the archive's no-op member is not selected.
- `__load_CW` (`0x00491ff5`), `__set_statfp` (`0x00493c40`), and `__control87` (`0x00497770`) now link from VC5's public runtime exports. `__hw_cw` is supplied transitively with `__control87`.

## Open questions and focused follow-up

- The non-MSVC `crt_load_control_word` fallback deliberately leaves the host floating-point environment unchanged; the reference-compatible behavior is provided by VC5 on MSVC.

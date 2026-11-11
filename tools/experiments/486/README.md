# Issue #486 experiments: MSVC 5.0 symbol-counter drift

Kit for [finding 538](../../../docs/findings/538-msvc5-symbol-counter-codegen-drift.md).
Each experiment compiles variants with the matching build's MSVC 5.0 flags and
fingerprints one function's `/FAs` listing (counter-numbered `$S`/`$L`/`$T`
names normalised). Wine runs directly when it is on `PATH`, otherwise in the
`developer` distrobox (`MSVC486_DISTROBOX`; `MSVC486_JOBS` sets parallel
compiles, default 4). Unit flags come from `tools/match_function.py`'s cached
configure, so run `make match-function ADDR=0x004869a0` once first. Output goes
to `build-486/` (gitignored). No game data is needed.

| Script | What it does |
| --- | --- |
| `run.py counter` | counter cost per declaration kind (finding section 4) |
| `run.py order` | rule A: `fmul` order against declaration order (section 7) |
| `run.py isolate` | rule B in the 10-line `pixel24.c` (edges 171, 427, 683, 939) |
| `run.py phase` | rule B in `draw.c`, `lava_erosion.cpp`, `steering.cpp`, `cmd.cpp` (section 5; a few minutes) |
| `sweep.py SOURCE ANCHOR FUNC… [--range A B STEP] [--decl FMT]` | one sweep: N inert declarations after the line starting with ANCHOR, one letter per distinct code |
| `ddmin.py BASE.c ANCHOR INSERT.txt FUNC FLIPPED_FP OUTDIR [PREFIX]` | delta-debug which top-level declarations of an inserted context flip FUNC |
| `msvc.py` | shared compile and listing helpers |

To score a sweep variant with Reccmp, pass it to
`python3 tools/match_function.py ADDR --candidate FILE`. For a C++ function,
keep the line numbers unchanged (for example, put all the inserted
declarations on the anchor's line), because the tool reads the annotated
signature by line.

Reproducing the `d3drm.h` bisection (section 2):

```sh
python3 tools/experiments/486/sweep.py src/gods98/draw.c '#include "crt/crt.h"' _Draw_Pixel24 --range 0 520 1
# preprocess `#include <windows.h>` with and without `#include <d3drm.h>`
# (msvc.compile_many(..., preprocess=True)), keep the d3drm-only suffix as
# delta.txt, then:
python3 tools/experiments/486/ddmin.py src/gods98/draw.c '#include "crt/crt.h"\n' delta.txt \
    _Draw_Pixel24 <flipped fingerprint> build-486/dd '#include <windows.h>\n'
```

# `tools/seamrec/` — seam recorder for the shipped `LegoRR.exe`

The recording oracle from [ADR 0007](../../docs/adr/0007-port-on-develop-units-on-miniwin.md)
("Validation"). Proxy DLLs placed beside the shipped binary log every call the
game makes to the DX6/Win32 *seam* (the import surface of
[finding 15](../../docs/findings/15-compat-audit.md)), with arguments and return
values, in a normalized text form. They also inject a *scenario*'s time and
input readings, so a run replays deterministically. The port's miniwin will
later log the same calls in the same form, and the two logs are diffed.

```sh
make seamrec                                            # build the proxies (podman)
make seamrec-check                                      # replay boot-menu-quit twice, logs must match
python3 tools/seamrec/seamrec.py run --mode replay \
    --scenario tools/seamrec/scenarios/boot-menu-quit.scn --name mine
python3 tools/seamrec/seamrec.py run --mode script \
    --script tools/seamrec/scenarios/boot-menu-quit.script --shot-every 25
python3 tools/seamrec/seamrec.py check boot-menu-quit --rescript   # re-record from the script
```

Runs land in `${RR_A2_HOME:-~/.local/share/rock-raider-a2}/seamrec/runs/<name>/`:
`seam.log` (the log), `scenario.scn` (readings), `warnings.txt` (process-level
notes), `wine.log`, and `shots/` (BMP framebuffer captures).

## Build

`make seamrec` runs `seamrec.py build`. It builds the tool image
`lrr-seamrec-tools:1` once from [`Containerfile`](Containerfile): a pinned
Debian 13 digest with mingw-w64 i686 GCC 14, Xvfb and PulseAudio. It then runs
`make` inside the image. The output is in `tools/seamrec/build/`: `seamrec.dll`
(the core) and the proxies `ddraw.dll`, `d3drm.dll`, `dinput.dll`,
`dsound.dll`, `winmm.dll` and `avifil32.dll`. Nothing here uses the MSVC 5.0
toolchain or `cross/`.

The sources are generated from two tables:

| Input | What | Produced by |
| --- | --- | --- |
| `methods.txt` | every method of the 85 wrapped COM interfaces, in vtable order, and the callback typedefs they take | `extract.py`, from the image's mingw-w64 headers (`make -C tools/seamrec methods` inside the image); committed |
| `flat.txt` | the flat functions LegoRR.exe imports: proxy exports and import-table hooks | hand-written; each signature is type-checked against the header prototype |
| `exports/*.txt` | the real DLLs' export tables (ordinal, name), for pass-through | `pe.py exports` on the runner's DLLs (Microsoft's for d3drm) |

`gen.py` classifies every parameter by its declared type, for example
interface in or out, `REFIID` + `void**`, callback, float, handle, string or
struct pointer. It emits one logging wrapper per method, the callback
trampolines, the IAT hooks and one pass-through proxy per DLL. Per-method
overrides live at the top of `gen.py`:

- `HOOKS`: the methods whose real call goes through `src/hooks.c` (injection).
- `PARAM_KIND`, `COUNTS`, `SPARSE`: parameters whose type does not say enough.
- `HANDLE_NEW`: functions that create handles.

## How a run works

- **Stage.** `reference/` is copied to `…/seamrec/game/`. Microsoft's
  `D3DRM.DLL` becomes `d3drm_ms.dll`, and the proxies sit beside the exe. The
  exe must still hash to the evidence digest. `Saves/` is reset before every
  run.
- **Loading.** `WINEDLLOVERRIDES=ddraw,d3drm,dinput,dsound,winmm,avifil32=n,b`
  makes Wine load the proxies from the game directory. Each proxy forwards
  the entry points the game imports to `seamrec.dll`, and passes every other
  export through to the real DLL. The real DLLs are Wine's from `system32`,
  and Microsoft's `d3drm_ms.dll`. `seamrec.dll` patches the exe's own import
  table for USER32, GDI32, ADVAPI32, ole32 and KERNEL32 `Sleep`/`ExitProcess`.
- **Sandbox (default `--display xvfb`).** Every run gets a fresh container
  from the tool image, on the host network namespace:
  - Xvfb at 640×480. Wine renders with Mesa llvmpipe, so the host GPU and
    desktop are not involved.
  - A PulseAudio with a single null sink, so the game sees the same audio
    devices on every host and nothing plays.

  `--display gamescope` (headless) or `gamescope-window` runs on the host GPU
  through the A.2 harness's gamescope path instead. Software rendering is the
  default for two reasons: recordings should not depend on the GPU driver,
  and on this box a GPU fault left the NVIDIA driver unable to create new
  devices.
- **Teardown.** The run ends when the log's `-- end` line appears. The
  harness then kills the process group and anything left in the A.2 prefix,
  so every run starts from a cold wineserver.
- **Arguments.** Scenarios carry the game's command line (`args -best`).
  `-best` skips the driver/mode dialog (`init.c`).

## Only the game's calls are logged

- Flat proxy exports check their return address. Callers outside
  `LegoRR.exe`'s image, such as Microsoft's D3DRM calling `DirectDrawCreate`
  or Wine's DLLs calling winmm, are passed straight through.
- The game only ever holds *wrapper* objects. A wrapper passed into a real
  call is unwrapped first, for example the `IDirectDraw` handed to
  `CreateDeviceFromSurface` or `AddMediaStream`. So D3DRM's and AMStream's own
  calls go to real objects and never reach a wrapper. A wrapper method called
  from outside the exe is passed through and counted ("foreign wrapper calls"
  in `warnings.txt`; 0 for boot-menu-quit).
- Callbacks into the game are routed through trampolines that wrap the real
  objects they carry. The game's calls made inside a callback are logged,
  nested under it. This covers D3DRM user visuals, frame-move, destroy and
  load callbacks, and the enumeration callbacks.

## Log format

```
seamrec-log 1
DirectInputCreateA(h6, 1280, &#1, NULL) = 0x00000000
IDirectInputA::CreateDevice(#1, GUID_SysKeyboard, &#2, NULL) = 0x00000000
IDirectDraw4::CreateSurface(#12, {size=124 flags=0x21 backbuffers=1 caps=0x2218/0x0}, &#13, NULL) = 0x00000000
IDirect3DRMFrame3::SetPosition(#52, #29, 0, 0, -200) = 0x00000000
IDirect3DRMViewport2::Render(#82, #29) = 0x00000000
  <- D3DRMUSERVISUALCALLBACK(exe:004827f0, #76262, @9231, 0, #77820, #77821) = 1
    IDirect3DRMFrame3::GetParent(#76207, &#76206) = 0x00000000
IDirectInputDeviceA::GetDeviceState(#2, 256, [256]{39:80}) = 0x00000000
<- WndProc(h11, WM_LBUTTONDOWN, 0x1, 0x17a0140) = 0
ExitProcess(0) = noreturn
-- end (ExitProcess) calls=485358 frames=914 objects=81638
```

- **Lines.** One call per line: `Interface::Method(this, args…) = ret`, or
  `Function(args…) = ret` for flat functions. A line is complete when the call
  returns, but the lines are written in call order (pre-order). Two spaces of
  indent per nesting level.
- **Callbacks** are `<- NAME(fn, args…) = ret`. `fn` is the game's callback
  address. Calls the game makes inside a callback are nested one level deeper.
- **Window messages** that change game state (activation, close/destroy, mouse
  buttons) are `<- WndProc(...)`. The game's procedure gets them at the end of
  the call during which they arrived, so they land at the same point in every
  run.
- **Objects** are `#N`: wrapper ids in order of creation. The same object
  always prints the same id. A D3DRM object keeps its id until it is
  destroyed, detected through a destroy callback of the recorder's own. Any
  other object keeps it until the game's own references (out-parameters,
  `AddRef`/`Release`) drop to zero. Ids therefore never depend on heap
  addresses being reused.
- **Out-parameters** print the value the game received, prefixed `&`
  (`&#13`, `&0`).
- **Pointers.** Code and data inside the exe are `exe:VA` (the image base is
  fixed). Any other data pointer is `@N`, numbered by first sighting.
- **Handles** are `hN`. Functions that create handles (`mmioOpenA`,
  `RegCreateKeyA`, `CreateWindowExA`…) give a fresh id each time.
- **Numbers.** Floats print with full precision (`%.9g`, which round-trips
  float32), doubles with `%.17g`, and HRESULTs as `0x%08x`.
- **Strings** are quoted with C escapes. Paths under the game directory print
  as `<game>\\…`.
- **Structs** have field-wise formatters for the common DX/Win32 types
  (`DDSURFACEDESC2`, `RECT`, `DDBLTFX`, `DSBUFFERDESC`, `MIXERLINEA`…). Other
  structs print as `<hex>`, or `<size:fnv=…>` when larger than 64 bytes.
- **Matrices** and `D3DVECTOR`s print as float lists.
- **Problems are loud.** `!! …` lines mark anything the recorder could not
  wrap, for example an interface without a wrapper (`UNWRAPPED interface`), an
  unsupported callback, or a scenario divergence. The boot-menu-quit log has
  none.
- **End.** `-- end (why) …` closes every log.

## Scenarios

A scenario is the sequence of *readings* a run consumed, one channel each:

| Channel | Source | Recorded |
| --- | --- | --- |
| `time` | `timeGetTime` (the game's only clock import) | value |
| `cursor` | `GetCursorPos` | result, x, y |
| `di` | DirectInput `GetDeviceState` / `Acquire` / `Unacquire` | HRESULT, nonzero key bytes |
| `ds` | `IDirectSoundBuffer::GetCurrentPosition` / `GetStatus` | HRESULT, values |
| `av` | AMStream `IDirectDrawStreamSample::Update` (movie frames arrive on their own clock) | HRESULT |
| `msg` | state-changing window messages | the call they arrived during, message, wParam, lParam |
| `rnd` | `D3DRMVectorRandom` (Microsoft's D3DRM seeds its own generator) | the vector's float32 bits |

Keys include the object id, so a replay that drifts stops at once with
`!! SCENARIO DIVERGENCE` and exit code 3, instead of silently consuming the
wrong reading.

There are three ways to get one:

- **`record`** captures a live run: real clock, real cursor and keyboard,
  real sound positions.
- **`script`** synthesizes the readings from a `.script` file. It has a
  virtual clock (`frame_ms` per message pump, plus 1 ms per 16 reads, so wait
  loops that never pump still advance) and scripted input:
  ```
  args -best
  frame_ms 40
  cursor 320 240
  at 100 key 0x39 down        # DIK code
  at 850 cursor 320 378
  at 860 lbutton down         # also: rbutton, close (WM_CLOSE)
  ```
  Sound positions come from a model of each buffer that advances with the
  virtual clock. AMStream results are live.
- **`replay`** serves every reading from a `.scn` file. The real calls are
  still made, but their results are replaced. `Sleep` is shortened to 1 ms.

A *frame* is one game `PeekMessageA` pump. The recorder always shows the game
an empty queue and dispatches the real queue itself, see "Window messages"
above.

`scenarios/boot-menu-quit.*` is the first scenario. It boots with `-best`,
taps Space to skip the intro movies, waits on the main menu, clicks *Quit
Game*, then *Yes*, and the game exits through `ExitProcess(0)` at frame 914.
The `.scn` (68 KB of readings) and the seam log's digest are committed. The
log itself (485,360 lines, 32 MB) is not; `check` regenerates it.

`scenarios/level1-start.*` is the oracle for level rendering
([ADR 0008](../../docs/adr/0008-level-rendering-on-miniwin-renderer.md),
step 0). It skips the intro, clicks *Start Game*, picks the first training
mission in the level select, pages through the mission brief with Space,
and then leaves the level running without input. The level is drawing from
about frame 1210; the run stops at frame 2000 (`stop_after`, which the
`.scn` carries, so a replay stops there too). Its log is 7.6M lines
(about 600 MB). `stats` summarizes a log's level-rendering calls: the
user-visual callbacks and `DrawIndexedPrimitive` calls per viewport
`Render`, the post-render draws, `Clear2` arguments, render states, and
light and fog calls. `scenarios/level1-start.render-stats.txt` is its output
for this scenario:

```sh
python3 tools/seamrec/seamrec.py check level1-start
python3 tools/seamrec/seamrec.py stats ~/.local/share/rock-raider-a2/seamrec/runs/check-level1-start/replay1/seam.log
```

## Determinism

The script run of boot-menu-quit and two replays of its scenario give
byte-identical logs, and the digest is in `scenarios/boot-menu-quit.seam.sha256`.
A live `record` run of 400 frames, replayed, also matched byte for byte.
For level1-start the two replays are byte-identical, but the script run's
log differs from them in `@N` pointer ids: the game's heap is laid out
differently in script mode, so a freed address is reused (and keeps its
id) in one mode and not the other. `check` accepts a script log that
differs from the replays only in pointer ids. The
digest also depends on the environment that recorded it: the Proton runner
that `a2.py find_runner()` picks, Mesa (llvmpipe), and the tool image. A
replay on another setup that is self-consistent but misses the digest
indicates drift in one of those.

## Not wrapped yet

- **KERNEL32** apart from `Sleep`/`ExitProcess`: file I/O, heap and the CRT.
  This is the future `[library:filesystem]` seam.
- **USER32 `wsprintfA`** (varargs formatting).
- **The `DLGPROC` of the mode-selection dialog**: logged loudly if reached;
  `-best` avoids it.
- **Pixel data.** Surface and texture contents are not hashed in the log.
  Screenshots (`--shots`, `--shot-every`) are separate, and
  framebuffer-checkpoint comparison belongs to `make port-check`.
- **DirectInput mouse devices and `GetDeviceData`.** The game uses only the
  keyboard's `GetDeviceState`. A mouse device would be wrapped and logged,
  but its state is not injected.
- **Interfaces outside the 85 in `methods.txt`.** These include
  `IGraphBuilder`, `IMediaStreamFilter` and `IDirectInputEffect`; they print
  as `!! UNWRAPPED` if the game ever receives one. To add one, list it in
  `extract.py` and regenerate `methods.txt`.
- **Array arguments.** Most print only their first element. Add a `COUNTS`
  entry where a full dump matters.

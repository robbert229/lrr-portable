# Win32 Wine runtime flow: source evidence map

2026-11-02; inspected HEAD `dc7f167b1a4b19efe65a35fcb5523fdf61e62fc1`.
Scope: PE32/MSVC 5.0 WinAPI/Reccmp candidate only. No builds, tests, launches,
reports, source edits, or launch/config changes performed. Only this note is owned
by this investigation. Existing dirty submodule and concurrent goal note preserved.

**Runtime status supplied by supervisor:** fresh build passed; reference settings
accepted and main menu visible after user-approved KDE input. Candidate settings
are visible but have **not yet been accepted**. This audit establishes no runtime
gate. In particular, historical `surface_create`/input failures do not establish a
failure of the current candidate. The earliest next observation is candidate
settings acceptance → first menu, not a speculative downstream repair.

Read: `AGENTS.md`, `hack/port-loop-2.md`, chief-of-staff skill, findings 02/03,
startup/frame-loop sources and relevant canonical/engine-region decompiles.
The user's no-delegation/read-only instructions govern this research assignment.

## Production route and owners

Pointers below are repository-relative `file:line`; addresses identify the
reference target, not the rebuilt binary's address. `FULL` means
`docs/decompiles/full/LEGORR.full.c`; `ENGINE` means
`docs/decompiles/legacy/engine-region.c`.

| Gate | Actual candidate caller/state owner | Reference and concrete observation for integrator |
| --- | --- | --- |
| Startup/data | `src/platform/winapi/entry.cpp:199` `WinMain` installs registry hooks, single-instance mutex, WAD basename, parses command line plus registry `StandardParameters`; calls `winmain_application_run:164`. `src/gods98/frameloop.cpp:2650` owns application state and mount. Explicit `LEGORRR_WAD_DIR` uses `resource_mount`; absent override uses retail-shaped startup/CD services. `block_open("Lego.cfg")` follows mount. | `FULL:77825`, WinMain `0x4777a0`; mount `0x47f130`. Keep integrator's established launch context. Mount/config failures precede the dialog; visible candidate settings already implies that invocation got past mounting/window creation, not that later init succeeded. |
| S → M | `frameloop.cpp:2775` creates window, installs `surface_winapi_startup_ops_install`, calls `directdraw_device_initialize(1,0,0,0,...)`, then **separately** creates fixed 640×480 `app.surface`. Installer: `src/gods98/surface.cpp:5122`. Selection dialog: `:5072`; renderer callback: `:5099`; WinAPI surface creator: `:4758`; lock: `:5204`. | `FULL:110851`, `0x49d030` calls renderer init `0x47c610`; see finding 111. Capture chosen controls and whether log reaches `DirectDraw mode selection accepted`, then `surface_create rc=0`, config/frontend messages and visible menu. Selection failure returns 3; surface failure returns 2. No current failure established. |
| Menu | `frameloop.cpp:2477` installs game config/name/Stats/HUD state through the registered init callback. `:3007` builds frontend, attaches options and opens `menu_enter(...,0)` if config enables it and no level override exists. `:2514` captures events, calls `frontend_tick`, registered engine tick, then menu render `:2365`. | Registration `0x41fa20` → slots `0x50592c/930/934`: `src/gods98/engine.cpp:158`. `FULL:23429` shows reference GameConfigInit `0x41fb50` creating scene root before config/type loading. Visible menu response, not `frontend built=1`, closes M. |
| Tutorial event | `src/game/frontend.cpp:2054` builds campaign/tutorial chains from `Main.StartLevel`/`TutorialStartLevel` and `NextLevel`; `:1174` grafts rows onto `LevelPickT` art. `fe_activate:2827` → selected-action `:2804` → `frontend_start_level:2815`, setting `pending_level`, closing menu, returning `FE_EVT_START_LEVEL`. `frameloop.cpp:2611` copies that name; `:3947` advances MENU → GAMEPLAY_START. | `FULL:15197` (`0x415220`), `:16420` (`0x416c80`); chain `0x4154b0`, selected action `0x416d70`. Record the actual row and pending level name. Direct-start overrides do not establish this path. |
| Level load | `frameloop.cpp:3084` prepare → `:2082` `frameloop_load_level` → `src/game/level.cpp:2934` `level_load`. Current level is published to `GameContext` before loaders. NERP/objective setup `:3149`; mandatory PTL remap/map pipeline `:3202`; OList parse `:3234`. Failure returns NULL. Wrapper then binds engine level/voxels/terrain, seeds mission/reward and camera; tutorial briefing holds simulation until click/Enter/Space (`frameloop.cpp:3247`). | `0x429890` LevelLoad; `0x42b510` MapLoadPipeline; `0x42e020` OList parser (`FULL:30537`). Inspect the **first failing return**, required resource path and type-table count if load fails. A NULL level is logged but application continues with fallback drawing, so a live window is not T. |
| Entity selection/motion | OList creates through `src/game/entity.cpp:6049` → `entity_create_with_position:5033`, then binds type name, Stats, activity/scene slots. `frameloop.cpp:3718` projects terrain/picks Raider, then `selection_add` or `selection_cmd_move`; `src/game/select.cpp:1326` → producer `src/game/cmd.cpp:3598` → queue executor `:3666`, opcode 5/6 → move constructor `:2707`. `ai_tick:1864` consumes chains; engine ticker `src/gods98/tickers.cpp:90` drains queue. | Producer `0x452060`, queue `0x4520d0`, constructor `0x4035c0`; `FULL:52480`/`:2049`. Game-layer AI runs **before** this frame's engine queue drain (`frameloop.cpp:3790`, `:3865`), so a new move normally starts next frame. See candidate 2. |

## Top three candidate blocker tasks, in acceptance order

### 1. Settings success must establish the actual drawing contract (S → M)

**Observed source:** installed `surface_winapi_startup_renderer_initialize`
(`surface.cpp:5099`) applies window mode, creates a selected-format **probe**
offscreen surface, destroys it, and returns success. It never calls existing
`directdraw_renderer_init` (`:2372`, `0x47c610`), which would retain primary/back
surfaces and initialize RM/device. The later application surface uses
`surface_create(...,640,480)` (`:5139`) with unspecified bit depth, normal
cooperative level and no retained association with that probe. Menu/scene pixel
writers and `platform_present_pixels` assume four bytes per pixel.

**Owners/installers:** application owns `app.surface`; `surface.cpp` owns COM
lifetime and pixel format; `src/platform/winapi/frameloop.cpp:306` presents locked
RGBA through GDI. No production `platform_render_surface_ops_set` installer was
found. GDI is a real existing presentation path, so missing DirectDraw present
callbacks alone do **not** imply a blank menu. WinAPI `surface_lock` is implemented;
the older handoff's “cannot lock” statement is stale for this source snapshot.

**Suspected failure, not observed:** probe rejection or subsequent creation/lock
failure may stop S → M; a non-32-bit application surface violates the software
writers' contract. Selecting 16-bit in the dialog does not prove the later surface
is 16-bit: the probe is discarded and the later format is unspecified.

**Bounded acceptance:** first accept the current candidate dialog and identify the
first failing call/return, if any. Verify the application surface's actual format,
pitch ≥ 640×4, successful lock/unlock, and visibly responsive menu. Repair that
real owner/format/lifetime contract only if evidence requires it. If retained RM
consumers are needed, complete device creation/commit, not an inert success hook:
`direct3drm_device_initialize:2499` requires five installed operations, and no
production installer for those operations was found. `renderer_begin_frame_once_retail`
(`:2905`, `0x477fa0`) dereferences that device without a null check when reached
through modal `main_loop_frame_pump`; ordinary application rendering uses the
separate guarded helper. Do not claim the modal dereference is already reached
by the boot-menu caller.

### 2. Tutorial entities need one spatial/visual contract (T → E)

**Confirmed spatial mismatch:** `entity.cpp:6018` converts one-based OList values
to grid values, then terrain **world** x/y (including block size, centered origin
and inverted Y). `entity_world_position_set:7927` stores those values unchanged.
`level.cpp:1205`/`:1218` already own forward/inverse terrain transforms.
But software renderer `src/game/scene_render.cpp:945`, picker `:1052` and AI
`src/game/ai.cpp:1650` use `entity->x/y` directly as grid coordinates. This can
place loaded Raiders off-screen/off-grid before a player can select them.
OList heading is radians (`entity.cpp:6033`), whereas software figure rendering
also applies a degrees-to-radians factor. These are source contract conflicts;
their exact symptom in the chosen tutorial remains unobserved.

**Confirmed command mismatch:** `selection_cmd_move` casts destination to an
`int pair[2]`, then `cmd_produce` copies those integer words. Opcode 5 reads them
as `float*`; tile `(8,9)` therefore becomes float bit patterns `0x00000008/09`,
not numeric `8.0f/9.0f`. `FULL:52480` → `:2049` confirms payload words are copied
unchanged into the move record. Fix the producer's payload and coordinate space,
not the reference queue ABI. AI currently treats distances/speeds as grid/seconds.

**Visual prerequisite:** actual boot callback calls `entity_names_config_load`
(`src/game/entity_config.cpp:973`), which loads all four type tables and Stats.
AE parse `:394` accepts a row with scene token zero when `visual_load` is absent.
`entity_type_config_runtime_ops_install:68` would install visual clone/resource/
scene-slot owners, but repository search found no production caller. Runner bind
(`entity.cpp:3927`) can set `bound=1` without a child scene. `scene_root_install`
likewise has no production caller. Reference `0x41fb50` → root `0x472710` → type
loader `0x42cf50` → `0x406990` → ACT `0x472cd0` requires real scenes; reference
creation `0x437d00` clones the type scene and binds its entity owner.
Existing `src/gods98/entity.cpp:64` parses AE assemblies but explicitly does not
construct the scene pipeline. Current `scene_render.cpp:894` draws procedural
Raiders/buildings from entity slots, so absent authored visuals alone does not
prove T/E is blocked. A nonexistent shipped OList `Model` key is not a substitute
for the type's AE/ACT binding.

**Bounded acceptance:** from the real tutorial menu, require non-NULL level,
expected minifigure count, reversible OList-world ↔ terrain-cell mapping, visible
entity/pick agreement, selected-bit feedback, and a move record containing the
numeric destination in the agreed space. Observe arrival at two distinct open
tiles with matching scene/selection feedback. If authored visuals are required,
separately require nonzero template token, live cloned child, owner binding,
activity update and balanced release; adding unused callbacks does not satisfy it.

### 3. Make the frame-time boundary explicit before diagnosing tutorial stalls (T/E)

**Confirmed unit mismatch:** `entry.cpp:164` calculates seconds as elapsed/1000,
fixed step 1/30, override `(25/fps)*0.04`, clamped to **3 seconds**. Reference
WinMain (`FULL:77962`, `0x4777a0`) starts with 1.0, then uses elapsed-ms×0.025,
fixed 0.8333333, override 25/fps and clamp **3 ticks**. These are 25-Hz ticks
(one tick = 40 ms), not seconds; finding 03's “seconds-delta” prose is misleading.
The decompile also puts fixed-step before pause/override, unlike the port.

**Actual consumers:** `EngineTick` (`engine.cpp:221`) receives the same seconds
value as game-layer AI; `options_scale_frame` (`options.cpp:221`) only multiplies
game speed. Tutorial hook (`frameloop.cpp:923`) invokes timer advance
`tutorial_hud.cpp:567` (`0x453bb0`), which converts its argument to milliseconds
by ×1000×0.04; objective countdown `objective.cpp:219` does the same. Thus those
reference-shaped timers advance at 1/25 wall time on this path. Menu sound clock
(`frameloop.cpp:63`) separately uses ms×0.025, so multiple conventions coexist.

**Bounded acceptance:** keep AI's seconds contract explicit and convert at the
engine/reference boundary; do not multiply every subsystem blindly. Integrator
can later sample a 1000-ms interval: reference delta sum ≈25, tutorial timer
advance ≈1000 ms, AI motion ≈one second at speed 1. Check fixed/override/frozen
paths, then pause without catch-up. This task is justified by source conversions,
not proof that the current unaccepted settings dialog is blocked by a timer.

## Later gates: concise traced hazards

- **Input:** WinAPI `platform_pump_events:246` supplies messages to
  `input_capture_events` (`src/gods98/input.cpp:319`); application binds its input
  via `input_direct_state_bind`, rather than installing a DirectInput device.
  Mouse press is a transient event; held state persists. `input_begin_frame:307`
  copies then clears keyboard current state. WinAPI letters arrive as uppercase
  VK codes, but gameplay checks lowercase `'m'`, `'n'`, etc.; key lookup `:497`
  does no normalization. Test real key-down/held/release separately from clicks.
  Cursor coordinates use actual client dimensions while rendering/picking uses
  fixed 640×480; verify agreement if settings resize the window.
- **Pause/resume — confirmed wrong caller:** `frameloop.cpp:3367` calls
  `menu_enter(&fe,2)`. `frontend.cpp:2560` maps mode 2 to **MainMenuFull submenu 2**
  (training), not PausedMenu. Continue and root-Escape resume branches require
  `FE_MENU_PAUSED` (`frontend.cpp:2866`, `:3074`). Open the actual parsed pause
  menu through its existing frontend API; preserve retail MenuEnter's training
  semantics. The nested loop (`frameloop.cpp:3382–3479`) owns event capture,
  overlay gate `0x415370`, menu drawing, sound tick, Sleep and input boundaries;
  it does not run AI/EngineTick. Entry's previous-time clock spans this blocking
  loop, so resume can feed a large next delta. Also consume the opening Escape
  edge before the nested menu, initialize every `FeInput` field (left/right are
  currently unset in boot/pause), and verify a second press resumes rather than
  quits. Tests that directly open PausedMenu bypass the broken production caller.
- **Shutdown:** `frameloop_application_shutdown:4006` logs stop reason then
  dispatches the registered shutdown callback **before** audio/surface/window/
  frontend disposal. Both registered wrappers call `EngineShutdown`; normal
  context (`engine.cpp:470`) disables CD watchdog and calls `exit(0)`, so the
  later explicit cleanup is unreachable on normal exit. This matches reference
  normal callback `0x424cf0` → `0x4250a0` (`FULL:24831`), distinct from loader
  teardown `0x424d00` (`ENGINE:517`). Check normal UI quit closes the window,
  returns success and leaves no process; do not require later cleanup markers
  as evidence or remove the reference exit merely to make those markers appear.

Current unknowns: candidate settings result; actual application surface format
and lock HRESULT; menu response after acceptance; selected tutorial/resources and
first failing loader return; resulting entity positions/visibility. Resolve these
in gate order using the integrator's current binary/launch evidence. No additional
callback scaffolding or historical report score can close those unknowns.

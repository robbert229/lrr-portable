# Divergences from the original game

The port aims to behave like the reference binary, the 1999 `LegoRR.exe`.
This file lists every place where it deliberately does not, and why.
AGENTS.md requires an entry here for each such change. New divergences
should, where they can:

- come with a `LegoRR.ini` key in the `[tweaks]` section that restores the
  original (cheats, off by default, go in a `[cheats]` section instead);
- mark their call site `// [divergence:<name>]`.

A key is one row of the settings table, `kTable` in
`src/platform/native/config.cpp`, with its `LRR_Setting` and a typed getter
in `config.h` for the unit to read. The row gives the key's default, which
the new `LegoRR.ini` is written with, and its kind, which sets the
spellings it accepts. A test sets the key with `LRR_SetSetting`. To show
the key in RecompLauncher, add a setting to `packaging/recomplauncher.json`;
`tools/tests/test_packaging.py` checks its keys, values and defaults against
the table.

Older ones carry a `PORT-ONLY:` comment instead.

The list has three parts:

1. Divergences a player can notice.
2. Fixes for bugs in the original that the port would otherwise reproduce
   or turn into crashes.
3. Platform substitutions that are meant to behave the same.

A change that only restores the original's behaviour, such as a 64-bit
layout fix, is not a divergence. Part 2 lists the larger ones for
reference.

## 1. Divergences a player can notice

### Save after a training mission

- **Original:** a level whose `Lego.cfg` block has no `Reward` section shows
  no rewards screen, and so no Save button. The eight training missions
  (`Levels::Tutorial01` to `Tutorial08`, the first entries on the mission
  select screen) are such levels. `Reward_CheckSkip` (0x00462780) goes
  straight to `Reward_GoToAdvance` (0x00462270):
  - In a new game, the progress goes to slot 5, the scratch slot. The Load
    menu never lists slot 5, so finishing a training mission and quitting
    loses that progress.
  - With a slot already loaded, the progress is kept only in memory until a
    later rewards screen's Save.
- **Port:** when such a level is completed, the game opens the Save Game
  screen that the Save button opens (`Reward_SaveMenu`, 0x00462030). If the
  player picks a slot, the level is recorded there and written to disk. If
  they back out, play goes on as in the original. Levels started from the
  separate Tutorials menu are not offered a save, the same guard the
  original's save menu has.
- **Why:** the reference binary's behaviour looks like an oversight in the
  game data rather than a design choice. Players lose the progress they have
  just made, and nothing tells them so.
- **Configurable:** `[tweaks]` `Training Save = prompt` (the default) or `off`, which
  keeps the original.
- **Where:** `reward_check_skip` in `src/game/reward.c`;
  `LRR_GetTrainingSave` in `src/platform/native/config.h`.

### Saves in a per-user directory

- **Original:** keeps `Saves\<n>.sav`, `Saves\<n>.osf` and the `.dat`
  thumbnails in `Data\Saves` under the install.
- **Port:** maps every name under `Saves` into `savepath`. The default is
  SDL's preference directory, e.g. `~/.local/share/lrr-portable/LegoRR/` or
  `%APPDATA%\lrr-portable\LegoRR\`.
- **Why:** an install directory is often read-only. Following
  isle-portable keeps the two ports alike.
- **Configurable:** `savepath`. An empty value keeps `Data\Saves`.
- **Where:** b4af5d1f (#911); `src/platform/native/files_native.cpp`,
  `src/platform/native/config.cpp`.

### Display settings from LegoRR.ini, not the Mode Selection dialog

- **Original:** unless started with `-best`, `Init.c` asks for the driver,
  device, screen mode and window or full screen in a dialog.
- **Port:** takes those choices from the ini file. The dialog code stays in
  the unit but is not called. An invalid resolution falls back to the
  dialog's default mode. The dialog's default device was the last hardware
  device; unless the file names one, the port instead picks the platform's
  default device (SDL GPU on the desktop, falling back to the software
  renderer) or the first device that draws the level, and falls back to
  another if the display can't be set up on it. `-best` keeps the
  original's choice.
- **Why:** miniwin cannot show Win32 dialogs. This follows isle-portable,
  which replaces Lego Island's CONFIG.EXE with `isle.ini`.
- **Configurable:** `Full Screen`, `Horizontal Resolution`,
  `Vertical Resolution` and `3D Device`. `-best` and `-window` still work.
- **Where:** 7590243a (#913); `src/gods98/init.c` (`[library:config]`).

### A resizable window, and the 3D view at the window's resolution

- **Original:** draws a 640x480 picture. In a window, the window is 640x480
  and cannot be resized. Full screen switches the display to 640x480.
- **Port:**
  - The window can be resized to any size, and opens at `LegoRR.ini`'s
    resolution. Full screen fills the desktop at the desktop's resolution;
    the display mode is never changed.
  - The game keeps its 640x480 layout. miniwin fits it into the window: the
    largest 4:3 area, centred, with black bars at the sides or above and
    below when the window is not 4:3.
  - The 3D view is drawn at the window's resolution, inside that 4:3 area,
    so it is sharper; it shows the same part of the scene as at 640x480.
  - The interface is the 640x480 art scaled up: nearest-neighbour when the
    scale is a whole number (1280x960, 1920x1440), so every pixel stays
    square and sharp, and bilinear at other scales, where nearest-neighbour
    would draw pixels of uneven widths. There is no setting for it.
  - The mouse, its clicks and edge scrolling follow the fit. A pointer over
    a bar counts as at the picture's nearest edge, which scrolls the view.
  - The software renderer, which the port uses only where SDL GPU is not
    available, still draws at 640x480 and scales the whole picture to fit:
    at 1600x1200 it ran at about 9 frames a second instead of 41.
- **Why:** a 640x480 window is small on today's displays, and a full screen
  of stretched 640x480 pixels is blurry. This follows isle-portable, which
  renders Lego Island's 3D at the window's resolution in the same way.
- **Configurable:** `[tweaks]` `display = fit` (the default) or
  `original`, which keeps a fixed window of the mode's size and draws the
  whole picture at the mode's resolution. Full screen still fills the
  desktop, with the picture scaled to fit, since the port does not change
  the display mode.
- **Where:** `src/gods98/init.c` and `src/gods98/main.c`
  (`[divergence:display-fit]`); `patches/miniwin` 0037 to 0040 (the fit,
  the filtering, the mouse and frame-buffer locks,
  `miniwin/miniwinpresentation.h`).

With `widescreen = true`, the default, a window wider than 4:3 is filled
instead of fitted with bars; see the next entry.

### Widescreen

- **Original:** draws a 4:3 picture: 640x480, or 800x600 and 1024x768 with
  `-debug`.
- **Port:** with `display = fit`, when the window is wider than 4:3 (the
  desktop, in full screen) the game's mode takes the window's aspect at 480
  lines, up to 4:1: 853x480 for 16:9, 768x480 for 16:10, about 1140x480 for
  21:9.
  - A level fills the screen. The 3D view keeps the 4:3 view's vertical
    field of view and shows more at the sides; picking follows it.
  - The level's interface keeps to the screen's edges. The radar, the
    messages, the information panel and the info dock stay at the left. The
    top panel, the icon panel, the crystal side bar, the priority list, the
    encyclopedia and the camera control move to the right edge. The mission
    briefing, the help window, the in-level dialog, the pause and options
    menus, and the advisor beside the briefing are centred. Lego.cfg's
    640x480 layout is moved as it is read; there are no new data files.
  - The 640x480 screens around the levels are drawn as before, in a 4:3
    frame centred on the screen, with black at the sides: the front end,
    level select, saving and loading, the credits, the rewards, the loading
    and shutdown screens, and the movies. The mouse works in the frame's
    coordinates. The front end's rock wipe, drawn in 3D, covers the whole
    screen.
  - A save game's thumbnail shows the centre of the screen at 4:3.
  - The shape is chosen when the game starts. Resizing the window to
    another shape afterwards fits the picture into it with bars.
- **Why:** on a widescreen display the 4:3 picture leaves black bars at the
  sides, a third of a 16:9 screen (#915).
- **Configurable:** `[tweaks]` `widescreen = true` (the default) or `false`,
  which keeps the 4:3 picture with bars. It needs `display = fit`. The 3DS
  defaults to `false`, as a wider view draws more of the level each frame.
- **Where:** `src/gods98/init.c` (the mode), `directdraw.c` (the 4:3 frame
  and the margin the interface moves by), `input_unit.c`, `viewports.c`,
  `images_unit.c` and `main.c`; `src/game/lego_unit.c`, `panel_unit.c`,
  `advisor.c`, `objective_unit.c`, `helpwindow.c`, `frontend_unit.c`,
  `reward.c`, `screen_effect_quads.c`, `tooltip.c` and `loader.c`
  (`[divergence:widescreen]`); `patches/miniwin` 0052
  (`Miniwin_SetViewportFieldAspect`).

### Keyboard and mouse camera controls

- **Original:** the top-down view pans only when the pointer touches the
  edge of the screen. The arrow keys tilt and rotate it, `-` and `=` zoom,
  and the middle mouse button and the wheel do nothing.
- **Port:** the top-down view also takes:
  - `W`, `A`, `S` and `D` to pan along the screen's axes, at `Lego.cfg`'s
    `CameraSpeed` and `CameraAcceleration`, as edge panning does;
  - `Q` and `E` to rotate, as the left and right arrows do;
  - the mouse wheel to zoom, a tenth of the zoom range per notch, within
    `Lego.cfg`'s `MinDist` and `MaxDist`;
  - a middle-button drag to pan, keeping the ground under the pointer
    where it was grabbed.

  Edge panning still works. The camera stays as far from the map's edge
  as edge panning keeps it (`MouseScrollIndent`), and none of the controls
  work where the original stops the camera (a level's script can do so).
  While debug keys are on (`Lego.cfg` `AllowDebugKeys`), `W`, `A`, `S`,
  `D` and `E` keep their debug meanings and do not move the camera; the
  wheel and the drag still work.
- **Why:** with edge panning alone, looking around means moving the
  pointer away from what it was doing, and in a window the pointer leaves
  the window instead. Today's strategy games pan with the keyboard and the
  middle button and zoom with the wheel. The original reads none of these
  keys or buttons outside debug mode, so nothing the original does changes.
- **Configurable:** `[tweaks]` `controls = modern` (the default) or
  `original`, which keeps only the original's controls.
- **Where:** `src/game/lego_unit.c` and `src/gods98/main.c`
  (`[divergence:modern-controls]`); `patches/miniwin` 0046 (the middle
  button and the wheel as `WM_MBUTTONDOWN`, `WM_MBUTTONUP` and
  `WM_MOUSEWHEEL`).

### Idle raiders take the jobs waiting for them

- **Original:** raiders sometimes stand idle while jobs wait for them, in
  the original as much as in the port. Players call it the "idleness
  syndrome"; it is worse with more raiders, and on maps with parts a
  raider cannot reach. On every tick, `AITask.c`'s job assignment
  (0x00405c10) hands each waiting job to the nearest idle unit that can
  do it. A unit that must walk to the job first is sent there
  (`Message_Dig`, `Message_Clear` and the rest, which the level's `.ptl`
  turns into a Goto), and the job records it as its unit on its way. In
  three places, a job makes no use of idle units it could have:
  - A job whose unit is on its way still takes the nearest idle unit on
    every tick. It gives it nothing, but takes it off the tick's idle
    list, so no later job can have it. Each unit on its way keeps one idle
    unit standing, and lower-priority jobs get none.
  - A unit that cannot reach a job pauses the job for 75 ticks. The pause
    also makes the job refuse every unit after it in the idle list on that
    tick. One raider cut off on an island can keep jobs from all the
    raiders listed after it, again and again.
  - The unit on its way stops being the job's only when it takes a job or
    its queue is flushed. An interrupted route flushes the queue
    (`LegoObject_Route_End`, 0x00441940), but not while the AI update
    runs, and a Goto whose route cannot be found (the cell beside the job
    is picked again when the unit is sent, and may differ from the one
    checked) is dropped without one. The job then waits for the unit until
    it takes another job, and keeps taking an idle unit on every tick; if
    that is the same unit, it waits forever.
- **Port:** a unit leaves the idle list only when it is given a job or
  sent to one. A job pauses only when no idle unit can reach it, as the
  original does. An idle unit stops being any job's unit on its way.
  Checking the walk to a job's side cell also uses the unit being
  checked; the original used the best unit found so far.
- **Why:** to fix a long-known bug. What a raider does with a job, and
  which job it takes first, are unchanged.
- **Configurable:** `[tweaks]` `fix_idleness_syndrome = true` (the
  default) or `false`, which keeps the original's job assignment.
- **Where:** `src/game/aitask.c` (`[divergence:idleness-syndrome]`);
  `src/game/test_idleness_syndrome.cpp`.

### Raiders take the job they were sent to

- **Original:** when the player sends a raider to dig a wall, clear rubble
  or pick something up, the raider walks there, and the job is marked as
  asked for by the player (it then has the top priority). The raider takes
  the job only once it stands at it, and only if the job is not paused. A
  job pauses for 75 ticks when an idle raider cannot reach it, and a
  dynamite job for 100 ticks when a raider gives up on it. If the job is
  paused when the raider arrives, `AITask.c`'s idle unit update
  (0x00404280) turns the raider away, and job assignment gives it other
  work in the same tick, so the order is lost. With no other work, the
  raider waits on the spot and takes the job when the pause ends.
- **Port:** a raider standing at a job the player asked for takes it even
  while the job is paused. The pause still keeps raiders from being sent
  to the job, and jobs the player did not ask for keep it.
- **Why:** a raider the player sent to a job should do it.
- **Configurable:** the same `[tweaks]` `fix_idleness_syndrome` key;
  `false` keeps the original.
- **Where:** `src/game/aitask.c` (`[divergence:player-job-pause]`);
  `src/game/test_idleness_syndrome.cpp`.

### Cheat: unlimited raiders

- **Original:** the Teleport Rock Raider button greys out once the level
  holds as many raiders as its Support Stations support. The check
  (`LegoObject.c` 0x0043c200) adds up `Lego.cfg`'s `OxygenCoef` over the
  level's objects: -1.0 for each raider and 10.0 for each Support Station.
  A teleporter also holds back a queued raider while the check fails.
- **Port:** with the cheat on, the check always passes. The button stays
  lit while a teleporter that takes raiders stands and the level's
  building dependencies are met, and the teleporters take every queued
  raider. The level's oxygen still drains as the original's does.
- **Why:** a cheat players asked for, off unless they turn it on.
- **Configurable:** `[cheats]` `unlimited_raiders = false` (the default,
  the original's limit) or `true`.
- **Where:** `src/game/legoobject_unit.c`
  (`[divergence:unlimited-raiders]`).

### Cheat: unlimited oxygen

- **Original:** a level starts with full air (100), which drains while
  its raiders breathe more than its Support Stations make. The level's
  `OxygenRate` sets the pace, and each object's `Lego.cfg` `OxygenCoef`
  its share: -1.0 for a raider, 10.0 for a Support Station (`Lego.c`
  0x00424560). The level is lost
  when the air runs out.
- **Port:** with the cheat on, nothing drains the air, so it stays full.
  The low-air warnings and heartbeat never play, and the reward screen's
  oxygen score is 100%.
- **Why:** a cheat players asked for, off unless they turn it on.
- **Configurable:** `[cheats]` `unlimited_oxygen = false` (the default,
  the original's air supply) or `true`.
- **Where:** `src/game/lego_unit.c` (`[divergence:unlimited-oxygen]`).

### Frame rate overlay

- **Original:** shows no frame rate.
- **Port:** with the overlay on, the frames a second (`15 FPS`) are drawn in
  the top right corner of the screen, in the `Font5_Hi2` font, over
  whatever the game drew. The number is counted over half a second, so it
  is steady enough to read. It is drawn every frame the game presents,
  while a level loads and in movies too, where it says little.
- **Why:** to see how fast the port runs, on a 3DS above all, where a slow
  frame rate is the first thing to find out.
- **Configurable:** `[tweaks]` `fps_overlay = false` (the default, no
  overlay) or `true`. `LegoRR.ini` written before the key existed does not
  have it; add `fps_overlay = true` under `[tweaks]`.
- **Where:** `main_fps_overlay_draw` in `src/gods98/main.c`
  (`[divergence:fps-overlay]`); `src/platform/native/fps_counter.cpp`.

### Touch controls

- **Original:** a mouse and a keyboard. SDL's own mapping makes a touch the
  left button, held while the finger is down.
- **Port:** where `touch` is on (by default on Android and iOS), touches
  are mapped onto the mouse and keys:
  - a tap clicks;
  - a drag holds the left button;
  - holding a finger still moves the pointer without clicking, so
    tooltips show;
  - a two-finger drag scrolls the map, a pinch zooms, and a two-finger twist
    turns the view;
  - a two-finger tap is Space;
  - a three-finger tap opens or closes the on-screen keyboard.

  On every host, the on-screen keyboard also opens while a unit's name is
  typed, and Android's Back button is Escape. A tap, or a key typed on the
  on-screen keyboard, that is shorter than a frame is held until the game
  has read it, so it is never lost.
- **Why:** a touch screen has no right button, no pointer that can rest
  without pressing, and no keys. Rock Raiders uses the right button as the
  left one unless it is started with `-dualmouse`, so no gesture is spent
  on it by default.
- **Configurable:** `[tweaks]` `touch = true` or `false`, which keeps
  SDL's mapping. The other keys are `touch_long_press = hover`,
  `touch_scroll`, `touch_zoom` and `touch_rotate = true`,
  `touch_two_finger_tap = space` (or `right click`),
  `touch_three_finger_tap = keyboard`, `touch_keyboard = auto` and
  `back_button = escape`; `off` turns each one off. A desktop touch screen
  works with `touch = true`, where the system lets SDL move the pointer.
- **Where:** `src/platform/native/input.cpp`, `touch_gestures.cpp` and
  `input_latch.cpp`; `src/gods98/input_unit.c` (`[library:input]`);
  `src/game/lego_unit.c` and `src/game/frontend_unit.c`
  (`[divergence:touch-keyboard]`); #986.

### Movies fill a larger screen mode

- **Original:** the front end plays its movies at 640x480. In a larger
  mode, 800x600 or 1024x768 with `-debug`, they play in its top-left corner.
- **Port:** they fill the mode.
- **Why:** a movie in one corner of the screen looks broken. At 640x480,
  the only mode outside `-debug`, nothing changes.
- **Configurable:** no; the larger modes are debug modes, reachable only
  with `-debug` and `[tweaks]` `display = original`.
- **Where:** `front_play_movie` in `src/game/frontend_unit.c`
  (`[divergence:movie-fill]`).

### LegoRR.ini and `--ini`

- **Original:** has neither.
- **Port:** writes `LegoRR.ini` with defaults on first run. `--ini <path>`
  names another file; on desktop a path that does not load is a fatal error.
- **Why:** this holds the settings above, as isle-portable does.
- **Where:** `src/platform/native/entry.cpp`, `src/platform/native/config.cpp`.

### `--version`

- **Original:** has no version option.
- **Port:** `--version` prints `lrr-portable <version>` and exits before
  the game starts. Every run also logs that line at start-up.
- **Why:** players and bug reports can name the release or commit a build
  came from (docs/releasing.md).
- **Configurable:** no.
- **Where:** `src/platform/native/entry.cpp`, `tools/version.cmake`.

### No registry

- **Original:** reads `HKLM\SOFTWARE\LEGO Media\Games\Rock Raiders`:
  - `StandardParameters` adds extra command-line flags.
  - `NoHALMessage`, `CDMissing` and `SetupError` supply message text.
- **Port:** every registry read fails, so the flags never apply and the
  default messages are used.
- **Why:** there is no registry off Windows. ADR 0007 plans an ini
  substitution for these values.
- **Configurable:** no; pass the flags on the command line.
- **Where:** `src/platform/native/seam_stubs.cpp`.

### The LEGO logo movie can be skipped

- **Original:** a key or click skips `LegoAvi` only in programmer mode.
  `DDIAvi` and `RRAvi` can always be skipped.
- **Port:** `LegoAvi` can always be skipped too.
- **Why:** consistency with the other two intro movies.
- **Configurable:** no.
- **Where:** d8dda7c7 (#894); `src/game/lego_unit.c` (`PORT-ONLY`).

### Closing the window quits from menus, movies and splashes

- **Original:** keeps running a front-end menu after a close request. Its
  intro movies and splash screens never check for one.
- **Port:** a close request leaves the menu as Quit Game does, and ends
  movies and splashes. A close request is the close button, Alt+F4, or
  SIGINT/SIGTERM.
- **Why:** otherwise closing the window, or stopping the process, left the
  port sitting at the main menu.
- **Where:** 132071ef (#865), 5759c088 (#877);
  `src/game/frontend_unit.c`, `src/game/lego_unit.c`.

### Suspending the 3DS pauses the level

- **Original:** a full-screen game never loses focus, and the original ran
  on PCs, which have no HOME menu or sleep mode.
- **Port:** on the 3DS, sleep mode and the HOME menu count as losing focus,
  and returning counts as one frame of it first. The level pauses when the
  configuration's `LoseFocusAndPause` is on, as it does for the original
  in a window, and its sounds stop. The player returns to a paused level.
  The system's exit request closes the game as the window's close button
  does, so saves and settings are written.
- **Why:** a level kept running its timers across a suspend, and the
  player came back to a changed mission.
- **Configurable:** the pause follows the original's `LoseFocusAndPause`
  key in `Lego.cfg`; there is no `[tweaks]` key, because without a suspend
  the port behaves as the original.
- **Where:** `src/platform/native/lifecycle.cpp`, `src/gods98/main.c`
  (`[divergence:suspend-pause]`).

### Front-end timing on elapsed time

- **Original:** the full-screen flip waited for vblank, so per-frame
  front-end effects ran about 60 times a second.
- **Port:** the frame rate is not capped. These effects now run on elapsed
  time at the original's 60 Hz rate:
  - the menu overlay roll
  - the text cursor blink
  - the menu edge scroll
  - the rewards screen scroll and its debug speed keys

  Front-end effects that have not been converted still count frames, so
  they can run faster than in the original.
- **Why:** to match the original's speed at any frame rate.
- **Where:** 55fa915f (#871), 6f1fc773 (#873).

### Music from track files

- **Original:** plays the CD's Red Book audio through MCI.
- **Port:** plays `Music/TrackNN.{ogg,flac,mp3,wav}`. Without the files there
  is no music, as with no CD in the drive.
- **Why:** few hosts have a CD drive.
- **Where:** 7c0872d1 (#874); `src/library/audio_sdl.cpp`.

### Movies need FFmpeg

- **Port:** loads FFmpeg 7 or newer at run time. Without it the movies are
  skipped.
- **Why:** it replaces DirectShow and AVIFile.
- **Where:** e6a2510d (#868); `src/library/video_ffmpeg.cpp`.

### Debug-build assert on the .osf magic

- **Port:** a debug build asserts the `RROS` magic when loading a `.osf`, as
  isle-portable asserts its save version. Release builds keep the
  original's check.
- **Where:** b4af5d1f; `src/game/objectrecall.c`.

### Format codes in game text

- **Original:** several kinds of text from `Lego.cfg` (dialogs, level and
  NERPs messages, tooltips, menus, info messages) are used as printf
  formats. A `%n` in such text writes to memory.
- **Port:** a `%n` in game text does nothing. A `%` followed by a letter
  that is not a standard printf conversion prints as that letter alone, on
  every host. Standard codes such as `%d` and `%s` print as C defines them.
- **Why:** mod and custom-level text must not be able to write memory or
  crash the game.
- **Configurable:** no.
- **Where:** `src/crt/safe_format.h` (`[divergence:printf-n]`,
  `[divergence:printf-unknown-letter]`); #954.

## 2. Bug fixes in the original

These change the original's code where it misbehaves. Each one is marked
`PORT-ONLY` at its site.

| Fix | Original | Port | Where |
| --- | --- | --- | --- |
| Restart Mission level name | frees `levelName`, which the new level keeps as its name; Win32's heap hid this | doesn't free it | 62f44679 (#903), `lego_unit.c` |
| NaN move probe | a NaN facing gives `INT_MIN` block coordinates, which 32-bit arithmetic wraps to block 0 | clamps out-of-range coordinates to block 0, the original's result, instead of reading outside the grid | 62f44679, `lego_unit.c` |
| Missing `.flh` animation | `Flic_Setup` fails but leaves its caller a half-built flic with no surface; the main menu's overlay animation (`Data\AVI\*.flh`) then calls through it and crashes a few seconds after the menu appears (a wasm `null function` trap on the web build); the reward screen and cursors would do the same (#983) | a flic that cannot be opened is skipped: the menu shows no overlay animation, the reward screen no flic, a cursor falls back to the standard one. No `[tweaks]` key: the original had no behaviour to restore | `flic_unit.c`, `pointer.c`, `reward.c` |
| Animated cursor flic | walks past the drawn frame and over-reads | stops after the frame; bounds the walk by the loaded size | e04b5f1b (#895), `flic_unit.c` |
| Texture free order | releases the surface before the texture that uses it; leaks the record | releases the texture first; frees the record | 16d561af (#896) |
| Smoke emitter frame ownership | `Smoke_Group_Show` resets the emitter's flags to 4 (finding 585), dropping ownership of its internally created frame; removing the emitter leaves that frame attached | preserves only the ownership bit so cleanup releases the frame. The one-shot/ending flags and sound retry latch are reset just as before. No `[tweaks]` key: this is resource cleanup, not a gameplay tweak | `sfx.c` (`[divergence:smoke-owned-container]`) |
| Unopened movie size | reads a NULL surface | guards it | `movie_unit.cpp` |
| Region data on failure | frees it twice after a `SetClipList` failure | frees it once | `directdraw.c` |
| RNC unpacked length | read as a signed long | read as unsigned | `rnc.c` |
| Sound sample volume marker | `SFX_LoadSampleProperty` copies the digits of a `#volume#` marker into an unterminated temporary string (finding 585); the interpreted volume can include leftover digits | parses the marked number in place, rejects an unclosed or out-of-range marker. Sample selection, stream-marker inheritance and queue behaviour are unchanged. No `[tweaks]` key: the original read uninitialised memory | `sfx.c` (`[divergence:sfx-volume-parse]`) |
| Info text image tokens | `info_compile_text` writes an uninitialised byte for a `$name$` token that names no image, overruns its image slots past the eighth token and its name buffer past 31 characters, and reads past the text for an unclosed token; Font draws any `@[0x` + 8 characters + `]` as an image | writes nothing for an imageless token, keeps five images and drops the rest, truncates the name, stops at the text's end; Font draws only a full hex address (#904) | `info_unit.c`, `font.c` |
| Scene animation last key | `Lws_FindPrevKey` and `Lws_Interpolate` blend a node's last key with the slot after it. `Lws_Parse` leaves that slot unwritten when the last key is on the scene's last frame, and allocates no room after it when it fills the slot itself. In Tutorial01 the slot held NaN bytes, so a raider's nodes went NaN at the end of an animation, and a power crystal on its carry null took a NaN position | holds the last key, as `Lws_FindPrevDissolve` does. That is the original's result whenever the slot held finite values on another frame. No `[tweaks]` key: the original read memory it never wrote (#943) | `lws.c` |
| Interface layout in other modes | `Lego.cfg` has the interface layout (`Panels`, `Buttons`, `AdvisorPositions`, `ObjectiveImage`, `NextButton`, `BackButton`, ... with a `640x480` suffix) only for 640x480. In another mode, 800x600 or 1024x768 with `-debug`, the lookups find nothing; the port crashed at start-up in `panel_item_flag32_set` (#915). | reads the 640x480 keys when the mode has none of its own. The interface stays at the 640x480 positions, as in the original's debug modes; the port's own scaling is the `display` tweak above. No `[tweaks]` key: without it the mode does not start | `lego_unit.c`, `advisor.c`, `objective_unit.c` (`[divergence:layout-fallback]`) |
| Carried facing with a still carrier | `legoobject_attached_direction_vector_update` (0x00440a10) divides by the distance the carried object moved, so a still carrier gives it a NaN facing. Crystals and ore never read it. A unit routing with a NaN facing skips its turn on the spot and sets off along +x; by the code, a vehicle unloaded from a still LargeCat would | keeps the previous facing. No `[tweaks]` key: the original's facing is a division by zero (#961) | `legoobject_unit.c` |
| `Data\Saves` directory | (n/a: only POSIX paths are affected) | makes `Data/Saves`, not a directory named with a backslash | b4af5d1f |

Many 64-bit and ABI fixes are not listed here. These include reused
`va_list`s, LP64 `RECT`s, struct offsets, linker-folded getters, misaligned
loads and declaration mismatches. They restore the original's behaviour
rather than change it; their commits (#863–#898) describe them.

## 3. Platform substitutions meant to behave the same

Each one is marked `// [library:<area>]` at its call site (ADR 0007).

| Area | Original | Port |
| --- | --- | --- |
| Audio | DirectSound, DirectSound 3D, winmm mixer | SDL3 audio streams |
| CD music | MCI cdaudio | track files through miniaudio |
| Time | winmm | `SDL_GetTicks` |
| Video | DirectShow, AVIFile | FFmpeg |
| Keyboard | DirectInput | SDL keyboard state through a scancode to `DIK_*` table |
| Filesystem | Windows paths | host paths, case-insensitive, `SDL_IOStream`, `SDL_GlobDirectory` |
| Config | registry, Mode Selection dialog | iniparser (`LegoRR.ini`) |
| Display | DirectDraw, Direct3DRM, USER32 | miniwin over SDL3 (ADR 0004, 0008, 0009) |
| C library | VC5 LIBC, linked statically | the host's C library, with the VC5 names and behaviours in `crt/vc5names.h` |

Known small differences in substitutions meant to behave the same:

- 3D sound pans with DirectSound's pan law. There is no Doppler, which the
  original never used either, because it sets no velocities.
- miniwin presents its software renderer as a hardware device, in RGB565.
- `entry.cpp` builds the Windows command line from `argv`, so the original's
  flags (`-best`, `-window`, `-nosound`, `-debug`, ...) work unchanged.
- `crt/vc5names.h` keeps the VC5 `rand`/`srand`, so random sequences match.

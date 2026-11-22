# lrr-portable

A decompilation and portable reconstruction of **Lego Rock Raiders** (1999).
The goal is a native port using SDL3/miniwin. I would currently classify it as a
vibe-coded slop fest, however things are developing rapidly. I hope to have it
in a playable state before the end of November 2026.

## Decompilation status

This is the `develop` branch, where the port runs the original-shape units
on miniwin ([ADR 0007](docs/adr/0007-port-on-develop-units-on-miniwin.md)).
The decompilation and its matching build continue on `main`. Figures from
the reports committed on `main` at `77f65f61` (#848, **2026-11-15**), built
with MSVC 5.0 and compared with Reccmp:

| Measure | Value |
| --- | ---: |
| Accuracy: mean Reccmp score of mapped functions, game-only / full | 98.04% / 98.23% |
| Mapped functions, game-only / full | 2,723 / 3,019 |
| Exact (100%) functions, game-only / full | 2,418 / 2,707 |

## What works, what remains

The focus so far has been on roughly implementing every single function. As a
result the game is not stable. You can play through some of the missions, however
the game is likely to crash before you end up completing a mission. We are nearing 100% 

<img width="2242" height="1440" alt="image" src="https://github.com/user-attachments/assets/9dae6667-cb3c-4ea5-a882-a837f23f350a" />
<img width="2242" height="1440" alt="image" src="https://github.com/user-attachments/assets/714ff741-c2b2-4093-8257-8386c593f194" />
<img width="2037" height="1440" alt="image" src="https://github.com/user-attachments/assets/7c544e7b-fb49-4450-9ac1-0ef29ecafcb2" />


## Build and run

Requires a legally owned game copy, CMake, a C++ compiler, and Git. SDL3 comes
from an installed package when there is one, and is fetched otherwise. Game
assets are not included. Place the game assets into the 
`reference` folder.

```sh
$ ls reference/
CLGen.dat  D3DRM.DLL  LegoRR0.wad  LegoRR.exe  legoRR-stub.log  Saves
CLGen.exe  Data       LegoRR1.wad  LegoRR.icd  Readme.txt
```

You can then build and run the port, and run the tests:

```sh
make run
make test
```

### Saves and configuration

The original game keeps its saves in `Data\Saves` inside the install. Like
[isle-portable](https://github.com/isledecomp/isle-portable), the port keeps
them in a writable per-user directory instead, named by `LegoRR.ini`. The port
writes that file with defaults on first run, in SDL's preference directory:

| Host | Configuration and saves |
| --- | --- |
| Linux | `~/.local/share/lrr-portable/LegoRR/` |
| macOS | `~/Library/Application Support/lrr-portable/LegoRR/` |
| Windows | `%APPDATA%\lrr-portable\LegoRR\` |

```ini
[legorr]
savepath = /home/me/.local/share/lrr-portable/LegoRR/
diskpath =
full screen = false
horizontal resolution = 640
vertical resolution = 480
3d device =

[tweaks]
training save = prompt
```

Point `savepath` elsewhere to move the saves. Leave it empty to keep the
original's `Data\Saves`.

The original only runs when started inside its install, where it finds `Data`
and `LegoRR0.wad` to `LegoRR9.wad`. Set `diskpath` to the install directory
to start the port from anywhere, as isle-portable's `diskpath`. Empty (the
default) keeps the working directory.

The original asked for the display settings in a "Mode Selection" dialog at
start-up. The port starts without it and reads them from the same file, as
isle-portable does for Lego Island's `CONFIG.EXE`:

- `full screen`: `true` for full screen, `false` (the default) for a window.
- `horizontal resolution` and `vertical resolution`: the game's mode, 640x480.
  800x600 and 1024x768 work with `-debug`; any other size falls back to
  640x480.
- `3d device`: a device as the original's dialog listed it, such as
  `Miniwin driver (SDL3 GPU HAL)` (the GPU, through SDL GPU) or
  `Miniwin driver (Miniwin Emulation)` (the software renderer). Empty (the
  default) picks the platform's default device. On the desktop that is SDL
  GPU when it can create a device (a Vulkan, Metal or Direct3D 12 driver),
  and the software renderer otherwise, for example on SDL's offscreen or
  dummy video driver. A device named here wins; one that isn't available
  falls back to the default. The OpenGL and paletted devices don't draw the
  level yet, so naming one shows no 3D, and the log says so. If the display
  can't be set up on the chosen device, the port logs it and tries the next
  one. The `-best` flag keeps the original's choice, the last hardware
  device.

The `[tweaks]` section switches the port's deliberate changes to the
original game:

- `training save`: `prompt` (the default) opens the Save Game screen after a
  completed training mission, which the original never offers; `off` keeps
  the original's behaviour.

Every way the port deliberately differs from the original game is listed in
[docs/divergences.md](docs/divergences.md).

Keys added after the file was written take their defaults. The shipped exe's
`-best` flag still skips the settings and picks a mode by itself. `--ini <path>` reads another configuration file. Save
files keep the original's format, so you can copy saves from an install into
the save directory. The file formats are described in
[finding 08](docs/findings/08-save-and-checkpoints.md).

[Build guide](docs/building.md) · [Findings](docs/findings/README.md) ·
[Pseudocode](docs/decompiles/README.md) · [Tools](tools/README.md) ·
[Compatibility harness](tools/compat/README.md) ·
[Repository layout](docs/repository-layout.md) · [Contributor rules](AGENTS.md)

## License

Code, tooling, and documentation: [LGPL-3.0-or-later](LICENSE). Original game
assets and binaries belong to their respective owners and are not included.
Keep your game files in the ignored `reference/` directory. This project is
not affiliated with or endorsed by the LEGO Group.

#!/usr/bin/env python3
"""A.2 live compat-test harness for LegoRR.exe on Linux.

Drives the shipped Win32 build under a Proton (Wine + DXVK) prefix so the
static compat audit (docs/findings/15-compat-audit.md) gets runtime evidence:

    python3 tools/compat/a2.py doctor                 # probe the box
    python3 tools/compat/a2.py setup                  # stage game
    python3 tools/compat/a2.py run [--dry-run]        # launch, log, auto-triage
    python3 tools/compat/a2.py triage [LOGFILE]       # rescan newest log
    python3 tools/compat/a2.py patch-avi | unpatch    # mod the staged .wads
    python3 tools/compat/a2.py shell <args...>        # raw exec under the prefix

State lives under ${XDG_DATA_HOME:-~/.local/share}/rock-raider-a2/ (prefix
under compatdata/, staged game under game/, logs under logs/). The repo's
source/ tree is never modified: staging copies, and wad patches keep an
original backup. Runner discovery honours $RR_A2_PROTON / $UMU_RUN, then
Steam / ProtonUp-Qt installs. See tools/compat/README.md.
"""
import argparse
import datetime
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys

TOOLS_DIR = pathlib.Path(__file__).resolve().parent.parent
REPO = TOOLS_DIR.parent
sys.path.insert(0, str(TOOLS_DIR))
import rrwad  # noqa: E402  (tools/ is on sys.path)

EXE_SHA256 = "c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f"
GAME_FILES = ["LegoRR.exe", "D3DRM.DLL", "LegoRR0.wad", "LegoRR1.wad"]
DATA = pathlib.Path(os.environ.get("RR_A2_HOME",
                                   pathlib.Path.home() / ".local" / "share" / "rock-raider-a2"))
COMPATDATA = DATA / "compatdata"
STAGE = DATA / "game"
LOGS = DATA / "logs"
BACKUPS = DATA / "backups"
GAME_SRC = REPO / "source"
MODS = STAGE / ".mods"

WINEDEBUG_DEFAULT = "+ddraw,+winmm,-fixme-all"

# Ordered so the triage summary hits the important classes first.
# (pattern, hint)  — hint phrasing maps each observation back to finding 15.
TRIAGE_SIGNATURES = [
    (re.compile(r"wine: chdir|wine: created| wineserver.*fail", re.I),
     "wine failed to start — prefix bootstrap incomplete (run `a2.py setup`)"),
    (re.compile(r"corrupted double-linked list|malloc.*corrupt", re.I),
     "allocator corruption — treat as env/wine failure, NOT game behaviour"),
    (re.compile(r"c0000005|segmentation fault|unhandled exception", re.I),
     "CRASH — attach the log to a new finding, do not continue blind"),
    (re.compile(r"err:module:import_dll", re.I),
     "missing DLL — import-surface gap vs finding 15 §1 table"),
    (re.compile(r"err:ddraw", re.I),
     "DDraw API errors — translation layer misses; consider dgVoodoo override"),
    (re.compile(r"fixme:ddraw", re.I),
     "unimplemented DDraw calls — usually benign, note which methods"),
    (re.compile(r"cdaudio", re.I),
     "CDDA music path active (finding 15 §4.1) — silent tracks are expected"),
    (re.compile(r"avifil|AVIFile|err:vfw", re.I),
     "AVI decode activity (finding 15 §4.2) — if black, `patch-avi` exists"),
    (re.compile(r"d3drm", re.I),
     "bundled D3DRM.DLL active (finding 15 §1) — judge 3D view quality visually"),
    (re.compile(r"dinput", re.I),
     "DInput activity (finding 15 §3) — mouse grab needs manual verdict"),
    (re.compile(r"cd\.key|DataMissing|Missing config", re.I),
     "install-data check fired (finding 01) — staging likely incomplete"),
    (re.compile(r"err:", re.I),
     "generic errors — inspect the log body for the first one"),
]


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# -- runner discovery ---------------------------------------------------------

def find_runner():
    """Return {'kind': 'proton'|'umu', 'path': Path} or None."""
    if p := os.environ.get("RR_A2_PROTON"):
        return {"kind": "proton", "path": pathlib.Path(p)}
    home = pathlib.Path.home()
    roots = [home / ".local/share/Steam/compattool*",
             home / ".var/app/com.valvesoftware.Steam/.local/share/Steam/compattool*",
             home / ".local/share/ProtonUp-Qt",
             home / ".local/share/lutris/runners/wine"]
    for pat in roots:
        for root in (r for r in sorted(home.glob(str(pat.relative_to(home)))) if r.is_dir()):
            proton = sorted(root.rglob("files/bin/proton")) or sorted(root.rglob("bin/proton"))
            if proton:
                return {"kind": "proton", "path": proton[-1]}
            wine = sorted(root.rglob("files/bin/wine")) or sorted(root.rglob("bin/wine"))
            if wine:
                return {"kind": "wine", "path": wine[-1]}
    for c in (shutil.which("umu-run"), os.environ.get("UMU_RUN")):
        if c and pathlib.Path(c).exists():
            return {"kind": "umu", "path": pathlib.Path(c)}
    return None


def runner_version(runner):
    try:
        out = subprocess.run([str(runner["path"]), "--version"],
                             capture_output=True, text=True, timeout=30)
        return (out.stdout or out.stderr).strip().splitlines()[0]
    except Exception:
        return "unknown"


def runner_env(runner):
    env = dict(os.environ)
    env.setdefault("WINEDEBUG", WINEDEBUG_DEFAULT)
    if runner["kind"] == "proton":
        env["STEAM_COMPAT_DATA_PATH"] = str(COMPATDATA)
        env["STEAM_COMPAT_CLIENT_INSTALL_PATH"] = str(pathlib.Path.home())
    elif runner["kind"] == "wine":
        env["WINEPREFIX"] = str(COMPATDATA / "pfx")
        env["PATH"] = f"{runner['path'].parent}{os.pathsep}{env.get('PATH', '')}"
    else:  # umu-run manages its prefix cache itself
        env["WINEPREFIX"] = str(COMPATDATA / "pfx")
    return env


def bootstrap_wine(runner, env):
    """A cold wine binary does not bootstrap its prefix — wineboot must run first.
    (Proton runners do this internally; the bare `wine` kind does not.)"""
    pfx = pathlib.Path(env["WINEPREFIX"])
    if (pfx / "drive_c" / "windows").is_dir():
        return
    print(f"bootstrapping wine prefix {pfx} (one-time)")
    # this wine chdirs into $WINEPREFIX before populating it — dir must exist first
    pfx.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(runner["path"]), "wineboot", "-u"], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, timeout=600)
    if not (pfx / "drive_c" / "windows").is_dir():
        raise SystemExit("prefix bootstrap failed — check wine installation")


# -- staging ------------------------------------------------------------------

def stage_game(force=False):
    if STAGE.exists() and not force:
        return
    STAGE.mkdir(parents=True, exist_ok=True)
    for f in sorted(GAME_SRC.iterdir()):
        if f.name == "unpacked":
            continue
        target = STAGE / f.name
        if force or not target.exists():
            if f.is_dir():
                shutil.copytree(f, target, dirs_exist_ok=True)
            else:
                shutil.copy2(f, target)
    print(f"staged {STAGE}")


def stage_and_check():
    stage_game()
    digest = sha256(STAGE / "LegoRR.exe")
    if digest != EXE_SHA256:
        print(f"!! staged LegoRR.exe sha256 {digest[:16]}… != evidence digest "
              f"{EXE_SHA256[:16]}… — test validity broken; re-copy from media")


# -- wad patching --------------------------------------------------------------

def patch_mods_tree():
    """Extract LegoRR1.wad once into game/.mods/L1 so repacks have a base tree."""
    tree = MODS / "L1"
    manifest_path = tree / ".rrwad.json"
    wad = rrwad.Wad(STAGE / "LegoRR1.wad")
    if manifest_path.exists():
        return tree, manifest_path
    tree.mkdir(parents=True, exist_ok=True)
    manifest = rrwad.manifest_for(wad, data_dir=tree)
    for i, entry in enumerate(manifest["entries"]):
        target = tree / rrwad.local_path(entry["name"])
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(wad.payload(i))
    manifest_path.write_text(json.dumps(manifest, indent=1) + "\n")
    return tree, manifest_path


def backup(path):
    BACKUPS.mkdir(parents=True, exist_ok=True)
    bak = BACKUPS / path.name
    if not bak.exists():
        shutil.copy2(path, bak)
        print(f"backed up original -> {bak}")


def cmd_patch_avi(args):
    stage_and_check()
    tree, mpath = patch_mods_tree()
    cfg = tree / "Lego.cfg"
    data = cfg.read_bytes()
    off = re.search(rb"(?im)^(\s*DontPlayAvis\s+)FALSE\b", data)
    if off:
        data = data[:off.start()] + off.group(1) + b"TRUE" + data[off.end():]
    elif re.search(rb"(?im)^\s*DontPlayAvis\s+TRUE\b", data):
        print("DontPlayAvis already TRUE — nothing to do")
        return
    else:
        data += b"\nDontPlayAvis\t\tTRUE\n"  # flat key lookup (FUN_00478f50), position-free
    cfg.write_bytes(data)
    backup(STAGE / "LegoRR1.wad")
    manifest = json.loads(mpath.read_text())
    out = STAGE / "LegoRR1.wad"
    out.write_bytes(rrwad.pack_bytes(manifest, tree))
    print(f"repacked {out} ({out.stat().st_size:,} B); "
          "DontPlayAvis TRUE active — flat key lookup, FUN_00478f50 @ 0x414ce0")


def cmd_unpatch(args):
    if not BACKUPS.exists():
        print("no backups present — nothing restored")
        return
    for bak in sorted(BACKUPS.iterdir()):
        shutil.copy2(bak, STAGE / bak.name)
        print(f"restored {STAGE / bak.name}")


# -- run / triage ---------------------------------------------------------------

def log_path():
    ts = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    return LOGS / f"run-{ts}.log"


def newest_log():
    logs = sorted(LOGS.glob("run-*.log")) if LOGS.exists() else []
    return logs[-1] if logs else None


def triage_file(path):
    text = pathlib.Path(path).read_text(errors="replace")
    hits = [(len(rx.findall(text)), hint) for rx, hint in TRIAGE_SIGNATURES]
    hits = [(n, hint) for n, hint in hits if n]
    print(f"-- triage {path}")
    if not hits:
        print("   no known signature lines — clean run or manual inspection needed")
    for n, hint in hits:
        print(f"   {n:6d} x  {hint}")
    return hits


def build_launch(args, game_cmd, bootstrap=True):
    runner = find_runner()
    if runner is None:
        raise SystemExit("no Proton/umu runner found — install one via ProtonUp-Qt "
                         "(or set RR_A2_PROTON / UMU_RUN) and rerun")
    if runner["kind"] == "proton":
        cmd = [str(runner["path"]), "run"] + game_cmd
    else:  # wine runs exes directly; umu-run takes the exe as its sole argument
        cmd = [str(runner["path"])] + game_cmd
    env = runner_env(runner)
    if runner["kind"] == "wine" and bootstrap:
        bootstrap_wine(runner, env)
    if getattr(args, "gamescope", True) and shutil.which("gamescope"):
        cmd = ["gamescope", "-w", "640", "-h", "480", "-r", "60", "-F", "nearest", "--"] + cmd
    return cmd, env


def cmd_run(args):
    if args.dry_run:
        cmd, env = build_launch(args, ["__preview__"], bootstrap=False)
        print("command:", " ".join(map(str, cmd)))
        keys = ("WINE", "DXVK", "VKD", "STEAM", "UMU", "GAMESCOPE")
        print("env:", {k: v for k, v in sorted(env.items()) if k.startswith(keys)})
        return
    stage_and_check()
    cmd, env = build_launch(args, [str(STAGE / "LegoRR.exe")])
    LOGS.mkdir(parents=True, exist_ok=True)
    log = log_path()
    print(f"launch: {' '.join(map(str, cmd))}\nlogging -> {log}")
    with open(log, "w") as lf:
        proc = subprocess.Popen(cmd, env=env, cwd=STAGE, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, errors="replace")
        try:
            for line in proc.stdout:
                sys.stdout.write(line)
                sys.stdout.flush()
                lf.write(line)
        except KeyboardInterrupt:
            proc.terminate()
            proc.wait(timeout=10)
        ret = proc.wait()
    print(f"\n-- exit {ret}; triage:")
    triage_file(log)


def cmd_setup(args):
    if args.restage:
        shutil.rmtree(STAGE, ignore_errors=True)
    stage_and_check()
    runner = find_runner()
    if runner is None:
        raise SystemExit("runner missing — run `a2.py doctor` first")
    if args.winecfg:
        cmd, env = build_launch(args, ["winecfg.exe"])
        raise SystemExit(subprocess.run(cmd, env=env, cwd=STAGE).returncode)
    if runner["kind"] == "wine":
        bootstrap_wine(runner, runner_env(runner))
    print(f"prefix under {COMPATDATA} (bootstraps automatically on first run)")
    print("next:   python3 tools/compat/a2.py run")


def cmd_doctor(args):
    r = find_runner()
    print("runner:", "not found — install Proton (GE) via ProtonUp-Qt flatpak"
          if r is None else f"{r['kind']} {runner_version(r)}  {r['path']}")
    vk = subprocess.run(["vulkaninfo", "--summary"], capture_output=True, text=True)
    devs = [ln.split(": ", 1)[-1].strip() for ln in vk.stdout.splitlines()
            if "deviceName" in ln]
    print("vulkan:", ", ".join(devs) if devs else "vulkaninfo reported no devices")
    print("gamescope:", shutil.which("gamescope") or "absent (wrapping skipped)")
    for f in GAME_FILES:
        p = GAME_SRC / f
        print(f"{'ok ' if p.exists() else 'MISS'} {f}")
    exe = GAME_SRC / "LegoRR.exe"
    print("exe digest:", "matches analysis evidence" if exe.exists()
          and sha256(exe) == EXE_SHA256 else "!! DIFFERS from c3d8ce23…e0f")
    last = newest_log()
    print("logs:", last if last else "none yet")


def cmd_shell(args):
    stage_and_check()
    cmd, env = build_launch(args, list(args.argv))
    raise SystemExit(subprocess.run(cmd, env=env, cwd=STAGE).returncode)


def cmd_triage(args):
    log = pathlib.Path(args.log) if args.log else newest_log()
    if log is None or not log.exists():
        raise SystemExit("no logs yet — run `a2.py run` first")
    triage_file(log)


def main(argv=None):
    ap = argparse.ArgumentParser(prog="a2", description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("doctor", help="probe box + game artifacts").set_defaults(fn=cmd_doctor)
    p = sub.add_parser("setup", help="stage game (+optional winecfg)")
    p.add_argument("--restage", action="store_true")
    p.add_argument("--winecfg", action="store_true")
    p.set_defaults(fn=cmd_setup)
    p = sub.add_parser("run", help="launch under Proton, log + triage")
    p.add_argument("--dry-run", action="store_true")
    p.add_argument("--no-gamescope", dest="gamescope", action="store_false", default=True)
    p.set_defaults(fn=cmd_run)
    p = sub.add_parser("triage", help="rescan a log (default newest)")
    p.add_argument("log", nargs="?")
    p.set_defaults(fn=cmd_triage)
    sub.add_parser("patch-avi", help="activate DontPlayAvis in staged wad") \
        .set_defaults(fn=cmd_patch_avi)
    sub.add_parser("unpatch", help="restore original wads from backups") \
        .set_defaults(fn=cmd_unpatch)
    p = sub.add_parser("shell", help="raw command under the prefix")
    p.add_argument("argv", nargs="+")
    p.set_defaults(fn=cmd_shell)
    args = ap.parse_args(argv)
    args.fn(args)


if __name__ == "__main__":
    main()

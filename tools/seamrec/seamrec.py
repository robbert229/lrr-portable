#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Seam recorder driver: build the proxies, stage the shipped game behind
them, and record / replay scenarios under the A.2 harness (tools/compat/a2.py).

    python3 tools/seamrec/seamrec.py build                 # podman + mingw-w64 i686
    python3 tools/seamrec/seamrec.py stage [--restage]
    python3 tools/seamrec/seamrec.py run --mode pass|record|script|replay \\
            [--script S] [--scenario F] [--name N] [--shots 100,200] [--maxframes N]
    python3 tools/seamrec/seamrec.py check NAME            # script run + 2 replays + diffs

Runs land in ${RR_A2_HOME:-~/.local/share/rock-raider-a2}/seamrec/runs/<name>/
(seam.log, scenario.scn, warnings.txt, wine.log, shots/). See README.md.
"""
import argparse
import datetime
import filecmp
import os
import pathlib
import shutil
import signal
import subprocess
import sys
import time

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE.parent / "compat"))
import a2  # noqa: E402

IMAGE = "lrr-seamrec-tools:1"
BUILD = HERE / "build"
PROXIES = ["ddraw", "d3drm", "dinput", "dsound", "winmm", "avifil32"]
ROOT = a2.DATA.resolve() / "seamrec"  # resolved: the exe path and the cwd must spell the same directory
STAGE = ROOT / "game"
RUNS = ROOT / "runs"
SCENARIOS = HERE / "scenarios"
SANDBOX = ROOT / "sandbox"


def winpath(p):
    return "Z:" + str(pathlib.Path(p).resolve()).replace("/", "\\")


def cmd_build(args):
    have = subprocess.run(["podman", "image", "exists", IMAGE]).returncode == 0
    env = dict(os.environ)
    # buildah's scratch space defaults to /var/tmp, which may be a small system volume
    env.setdefault("TMPDIR", str(pathlib.Path.home() / ".cache" / "seamrec-buildtmp"))
    pathlib.Path(env["TMPDIR"]).mkdir(parents=True, exist_ok=True)
    if not have or args.rebuild_image:
        subprocess.run(["podman", "build", "-t", IMAGE, "-f", str(HERE / "Containerfile"), str(HERE)],
                       check=True, env=env)
    subprocess.run(["podman", "run", "--rm", "--userns=keep-id", "-v", f"{REPO}:/w:Z", "-w", "/w/tools/seamrec",
                    IMAGE, "make", *(["-B"] if args.clean else [])], check=True)
    for d in ["seamrec"] + PROXIES:
        print(f"built {BUILD / (d + '.dll')}")


def cmd_stage(args):
    if args.restage:
        shutil.rmtree(STAGE, ignore_errors=True)
    if not STAGE.exists():
        STAGE.mkdir(parents=True)
        for f in sorted(a2.GAME_SRC.iterdir()):
            if f.name in ("unpacked", "Saves") or f.name.lower() == "d3drm.dll":
                continue
            (shutil.copytree if f.is_dir() else shutil.copy2)(f, STAGE / f.name)
        # Microsoft's D3DRM.DLL under another name: d3drm.dll is now the proxy
        shutil.copy2(a2.GAME_SRC / "D3DRM.DLL", STAGE / "d3drm_ms.dll")
        print(f"staged {STAGE}")
    if a2.sha256(STAGE / "LegoRR.exe") != a2.EXE_SHA256:
        raise SystemExit("!! staged LegoRR.exe does not match the evidence digest")
    for d in ["seamrec"] + PROXIES:
        src = BUILD / f"{d}.dll"
        if not src.exists():
            raise SystemExit(f"{src} missing: run `seamrec.py build` first")
        shutil.copy2(src, STAGE / f"{d}.dll")


def reap_prefix():
    """Kill whatever still runs in the A.2 prefix (wine services, a stuck game):
    every run starts from a cold wineserver, and Proton never lingers waiting
    for a persistent one."""
    prefix = str(a2.COMPATDATA.resolve())
    for pid in os.listdir("/proc"):
        if not pid.isdigit():
            continue
        try:
            cwd = os.readlink(f"/proc/{pid}/cwd")
        except OSError:
            continue
        if cwd.startswith(prefix) or cwd.startswith(str(STAGE.resolve())):
            try:
                os.kill(int(pid), signal.SIGKILL)
            except OSError:
                pass


XVFB_NUM = os.environ.get("SEAMREC_XVFB_DISPLAY", "77")
LAVAPIPE = ":".join(f"/usr/share/vulkan/icd.d/lvp_icd.{a}.json" for a in ("i686", "x86_64"))


def start_xvfb():
    """The run's sandbox, fresh per run, from the tool image on the host
    network namespace (so the abstract X socket is reachable):
      - a 640x480 Xvfb: software rendering (Mesa llvmpipe), so no host GPU or
        desktop is involved and runs do not depend on the GPU driver;
      - a PulseAudio with one null sink: the game sees the same audio devices
        on every host, and nothing is audible."""
    name = f"seamrec-xvfb-{XVFB_NUM}"
    SANDBOX.mkdir(parents=True, exist_ok=True)
    (SANDBOX / "pulse").unlink(missing_ok=True)
    subprocess.run(["podman", "rm", "-f", "-t", "0", name], capture_output=True)
    pulse = ("pulseaudio -n --daemonize=no --exit-idle-time=-1 --disallow-exit --disable-shm=yes "
             "-L 'module-null-sink sink_name=seamrec' "
             "-L 'module-native-protocol-unix socket=/run/seamrec/pulse auth-anonymous=1' 2>/dev/null & ")
    subprocess.run(["podman", "run", "-d", "--rm", "--name", name, "--network", "host",
                    "-v", f"{SANDBOX}:/run/seamrec:Z", "-e", "HOME=/tmp", "-e", "XDG_RUNTIME_DIR=/tmp", IMAGE,
                    "sh", "-c", pulse + f"exec Xvfb :{XVFB_NUM} -screen 0 640x480x24 -ac -nolisten tcp +extension GLX"],
                   check=True, capture_output=True)
    import socket
    for _ in range(100):
        s = socket.socket(socket.AF_UNIX)
        try:
            s.connect(f"\0/tmp/.X11-unix/X{XVFB_NUM}")
            if (SANDBOX / "pulse").exists():
                return name
        except OSError:
            pass
        finally:
            s.close()
        time.sleep(0.1)
    raise SystemExit(f"sandbox (Xvfb :{XVFB_NUM} + PulseAudio) did not come up")


def stop_xvfb(name):
    subprocess.run(["podman", "rm", "-f", "-t", "0", name], capture_output=True)


def reset_saves():
    """Every run starts from the shipped save state."""
    shutil.rmtree(STAGE / "Saves", ignore_errors=True)
    if (a2.GAME_SRC / "Saves").exists():
        shutil.copytree(a2.GAME_SRC / "Saves", STAGE / "Saves")


def file_args(path):
    """`args ...` line of a script or scenario: the game's command line."""
    for line in pathlib.Path(path).read_text().splitlines():
        if line.startswith("args "):
            return line[5:].split("#")[0].split()
    return []


def run_game(mode, outdir, script=None, scenario_in=None, shots="", shot_every=0, maxframes=0,
             timeout=600, game_args=(), display="xvfb"):
    cmd_stage(argparse.Namespace(restage=False))
    reset_saves()
    scenario_bytes = pathlib.Path(scenario_in).read_bytes() if mode == "replay" else None
    script_bytes = pathlib.Path(script).read_bytes() if script else None
    shutil.rmtree(outdir, ignore_errors=True)  # a stale seam.log would look like a finished run
    outdir.mkdir(parents=True)
    scn = outdir / "scenario.scn"
    if mode == "replay":
        scn.write_bytes(scenario_bytes)
    env = {
        "WINEDLLOVERRIDES": "ddraw,d3drm,dinput,dsound,winmm,avifil32=n,b",
        "WINEDEBUG": os.environ.get("SEAMREC_WINEDEBUG", "-all,err+module"),
        "SEAMREC_MODE": mode,
        "SEAMREC_LOG": winpath(outdir / "seam.log"),
        "SEAMREC_WARN": winpath(outdir / "warnings.txt"),
        "SEAMREC_SCENARIO": winpath(scn),
    }
    if script:
        (outdir / "script.txt").write_bytes(script_bytes)
        env["SEAMREC_SCRIPT"] = winpath(outdir / "script.txt")
    if shots or shot_every:
        (outdir / "shots").mkdir(exist_ok=True)
        env["SEAMREC_SHOTDIR"] = winpath(outdir / "shots")
        env["SEAMREC_SHOTS"] = shots
        env["SEAMREC_SHOTEVERY"] = str(shot_every)
    if maxframes:
        env["SEAMREC_MAXFRAMES"] = str(maxframes)
    if not game_args:
        game_args = file_args(outdir / "script.txt") if script else file_args(scn) if mode == "replay" else []
    xvfb = None
    if display == "xvfb":
        env.update({"DISPLAY": f":{XVFB_NUM}", "LIBGL_ALWAYS_SOFTWARE": "1", "__GLX_VENDOR_LIBRARY_NAME": "mesa",
                    "VK_DRIVER_FILES": LAVAPIPE, "PULSE_SERVER": f"unix:{SANDBOX / 'pulse'}"})
        cmd, outer = a2.build_launch(argparse.Namespace(gamescope=False), [str(STAGE / "LegoRR.exe"), *game_args],
                                     extra_env=env)
        outer.pop("WAYLAND_DISPLAY", None)  # wine must not pick the host compositor
    else:
        gs = ["--backend", "headless"] if display == "gamescope" else []
        cmd, outer = a2.build_launch(argparse.Namespace(gamescope=True), [str(STAGE / "LegoRR.exe"), *game_args],
                                     extra_env=env, gamescope_args=gs)
        if "gamescope" not in cmd[0]:
            outer = dict(outer, **env)
    reap_prefix()
    if display == "xvfb":
        xvfb = start_xvfb()
    started = time.monotonic()
    log = outdir / "seam.log"
    with open(outdir / "wine.log", "w") as lf:
        proc = subprocess.Popen(cmd, env=outer, cwd=STAGE, stdout=lf, stderr=subprocess.STDOUT,
                                start_new_session=True)
        ret, ended = None, None
        while ret is None:
            ret = proc.poll()
            if ret is not None:
                break
            if ended is None and log.exists() and b"\n-- end (" in log.read_bytes()[-512:]:
                ended = time.monotonic()  # the game is done; Proton may still wait on wine services
            if ended is not None and time.monotonic() - ended > 5:
                ret = "game ended"
            elif time.monotonic() - started > timeout:
                ret = "timeout"
            else:
                time.sleep(0.5)
        if proc.poll() is None:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait()
    reap_prefix()
    if xvfb:
        stop_xvfb(xvfb)
    took = time.monotonic() - started
    lines = sum(1 for _ in open(log, errors="replace")) if log.exists() else 0
    print(f"{mode}: exit {ret} after {took:.0f}s, {lines} seam lines -> {outdir}")
    return ret


def cmd_run(args):
    name = args.name or datetime.datetime.now().strftime(f"%Y%m%d-%H%M%S-{args.mode}")
    run_game(args.mode, RUNS / name, script=args.script, scenario_in=args.scenario, shots=args.shots,
             shot_every=args.shot_every, maxframes=args.maxframes, timeout=args.timeout,
             game_args=args.game_args, display=args.display)


def digest(path):
    data = pathlib.Path(path).read_bytes()
    lines = data.count(b"\n")
    return f"{a2.hashlib.sha256(data).hexdigest()}  {lines} lines"


def cmd_check(args):
    """The determinism gate for one scenario: replay scenarios/NAME.scn twice;
    both seam logs must be byte-identical. --rescript first regenerates the
    scenario from scenarios/NAME.script (and its log must equal the replays').
    The replay digest is compared with scenarios/NAME.seam.sha256, which
    --rescript rewrites; a digest mismatch with identical replays means the
    environment (runner, Mesa, stage) differs from the one that recorded it."""
    base = RUNS / f"check-{args.name}"
    scn = SCENARIOS / f"{args.name}.scn"
    expected = SCENARIOS / f"{args.name}.seam.sha256"
    shutil.rmtree(base, ignore_errors=True)
    logs = []
    if args.rescript:
        run_game("script", base / "script", script=SCENARIOS / f"{args.name}.script", timeout=args.timeout)
        shutil.copy2(base / "script" / "scenario.scn", scn)
        logs.append(base / "script" / "seam.log")
    for i in (1, 2):
        run_game("replay", base / f"replay{i}", scenario_in=scn, timeout=args.timeout)
        logs.insert(i - 1, base / f"replay{i}" / "seam.log")
    ok = True
    for a, b in zip(logs, logs[1:]):
        same = a.exists() and b.exists() and filecmp.cmp(a, b, shallow=False)
        print(f"{'identical' if same else 'DIFFERENT'}: {a.relative_to(RUNS)} vs {b.relative_to(RUNS)}")
        ok &= same
    got = digest(logs[0]) if logs[0].exists() else "missing"
    print(f"seam log: {got}")
    if args.rescript and ok:
        expected.write_text(got + "\n")
        print(f"wrote {scn.relative_to(REPO)} and {expected.relative_to(REPO)}")
    elif expected.exists():
        want = expected.read_text().strip()
        print("matches the committed digest" if want == got else
              f"differs from the committed digest ({want}): environment drift, or a recorder change")
    raise SystemExit(0 if ok else 1)


def main(argv=None):
    ap = argparse.ArgumentParser(prog="seamrec", description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("build")
    p.add_argument("--rebuild-image", action="store_true")
    p.add_argument("--clean", action="store_true")
    p.set_defaults(fn=cmd_build)
    p = sub.add_parser("stage")
    p.add_argument("--restage", action="store_true")
    p.set_defaults(fn=cmd_stage)
    p = sub.add_parser("run")
    p.add_argument("--mode", choices=["pass", "record", "script", "replay"], required=True)
    p.add_argument("--script", type=pathlib.Path)
    p.add_argument("--scenario", type=pathlib.Path)
    p.add_argument("--name")
    p.add_argument("--shots", default="")
    p.add_argument("--shot-every", type=int, default=0)
    p.add_argument("--maxframes", type=int, default=0)
    p.add_argument("--timeout", type=int, default=600)
    p.add_argument("--display", choices=["xvfb", "gamescope", "gamescope-window"], default="xvfb",
                   help="xvfb (default): software rendering on a private Xvfb; gamescope: headless "
                        "gamescope on the host GPU; gamescope-window: visible gamescope window")
    p.add_argument("game_args", nargs="*")
    p.set_defaults(fn=cmd_run)
    p = sub.add_parser("check")
    p.add_argument("name")
    p.add_argument("--rescript", action="store_true", help="regenerate scenarios/NAME.scn from NAME.script")
    p.add_argument("--timeout", type=int, default=600)
    p.set_defaults(fn=cmd_check)
    args = ap.parse_args(argv)
    if args.cmd == "run" and args.mode == "script" and not args.script:
        ap.error("--mode script needs --script")
    if args.cmd == "run" and args.mode == "replay" and not args.scenario:
        ap.error("--mode replay needs --scenario")
    args.fn(args)


if __name__ == "__main__":
    main()

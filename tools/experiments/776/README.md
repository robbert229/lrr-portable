# Issue #776 experiments: what drives MSVC 5.0's integer register choice

Kit for [finding 663](../../../docs/findings/663-register-choice-cause-c.md).
Everything compiles with the matching build's flags through
`tools/match_function.py` (run `make match-function ADDR=0x004869a0` once to
cache them) and writes to `build-776/` (gitignored). No game data is needed
beyond `reference/LegoRR.exe`. Never run these while a matching build or an
in-place search (`make match-search`) is running: they read the real sources.

| Script | What it does |
| --- | --- |
| `classify.py` | reads `match_function.py --json` (with diffs) on stdin; per function: `NAME` (only symbol names differ, an unlinked-compile artefact), `REG` (the diff vanishes once register names are normalised), `REGOP` (also once memory-operand terms and instruction order are normalised: register choice or scheduling), `OTHER` |
| `flags.py UNIT[,UNIT…] [--sets '/Oa\|/G6']` | scores every function of each unit under alternative cl options; prints rises and falls per option set |
| `phase_probe.py UNIT ADDR:name… [--range A B STEP]` | finding 538's `sweep.py` over N inert externs, then scores each distinct code: is the function a rule B phase case? |
| `pads_at.py UNIT 'LINE:N,…' …` | pads before chosen lines, whole-unit scores: which functions a counter shift at that point moves |

```sh
python3 tools/match_function.py $(cat addrs.txt) --json | python3 tools/experiments/776/classify.py
python3 tools/experiments/776/flags.py src/gods98/lwo.c,src/game/aitask.c
python3 tools/experiments/776/phase_probe.py src/game/terrain_map.c 0x44fab0:terrain_map_world_normal_get
python3 tools/experiments/776/pads_at.py src/game/terrain_map.c '16:52' '16:52,901:3'
```

Pads are a diagnostic only. ADR 0005 entropy rule 3 forbids committing them.

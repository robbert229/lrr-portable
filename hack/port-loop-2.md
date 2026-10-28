# LegoRR.exe recomp handoff

## Objective and scope

Continue improving the `lrr-portable` reconstruction toward an exact Reccmp match of `reference/LegoRR.exe`, especially by identifying and annotating original DAT globals. The user has repeatedly asked for batches of eight functions and explicitly prefers GPT-6 Luna High subagents supervised by the primary agent. Treat a follow-up “another 8” as eight *fresh* functions. No PR has been created.

Read the repository's `AGENTS.md` before editing: C/C++ must remain MSVC 5.0 compatible, validate with `make reccmp-report` when available, and never merge/squash/auto-merge a PR. Preserve all existing uncommitted changes. Many dirty files predate this work; inspect scoped diffs before editing. The recent batches primarily added `// GLOBAL: LEGORR 0x...` mappings and a few small source fixes. See `git diff` rather than copying prior changes into a new patch.

## Current verified state

- Latest report: `build-reccmp/reccmp-report/results.json` and `index.html`. Of 1,125 nonstub functions, 86 are exact, 990 partial, 49 zero. The most recent batch raised exact matches from 76 to 86: eight selected functions plus two related frontend functions; no nonstub score regressions.
- DAT comparison: 31 annotated variables, 30 matches, one issue. `s_bank_sound_on` maps to original `0x00533878`, but the original is zero-initialized and the port intentionally defaults it to 1 until its sound-init/config seam is completed. Avoid silently changing this default and disabling port audio. This issue predates the last two batches.
- `make reccmp-report` and `git diff --check` passed after the latest batch. Focused CRT and entity native tests passed. The frontend test has four failing UI assertions (overlay open/retry, selection highlight, font-key transparency); this batch changed only comments in `frontend.cpp`, so those failures are unrelated to its new annotations. The engine-bind native test has a pre-existing link failure for `util_atof`/`util_atoi`.
- The current working tree contains extensive pre-existing user changes in frontend, level, entity and build/docs files; do not reset or overwrite them. Recent agent edits are interleaved with them. There are no active subagents.

## Useful evidence and workflow

- `docs/dat/globals.csv` and `docs/dat/annotations.csv` inventory original DAT locations; `docs/decompiles/full/LEGORR.full.c` is the checked-in full decompile. `docs/coverage/ledger.csv` tracks ported/stubbed coverage, not match scores. A `GLOBAL` annotation tells Reccmp that a candidate variable corresponds to an original DAT object even when the two virtual addresses differ. Confirm object identity, width, and initial bytes before annotating. The numeric columns immediately after the address in `globals.csv` are reference counts, **not object sizes**.
- Before a batch, copy `build-reccmp/reccmp-report/results.json` to a new `/tmp` baseline. The JSON `data` entries include address, name, matching, stub and diff. Select fresh partial nonstub targets; group agents by disjoint source files. Do not let agents run the shared `build-reccmp` report concurrently. Primary agent runs it after edits, compares *all* nonstub scores against baseline, and reverts any regressions or changes that do not help and carry semantic risk.
- Host toolchain command: `make reccmp-report RECCMP_WINE_BIN="$HOME/.local/share/lutris/runners/wine/wine-ge-8-26-x86_64/bin/wine" RECCMP_RECCMP="$HOME/.local/bin/reccmp-reccmp"`.
- DAT check: `env WINEDEBUG=-all PATH="$HOME/.local/share/lutris/runners/wine/wine-ge-8-26-x86_64/bin:$PATH" reccmp-datacmp --target LEGORR --no-color --all --print-rec-addr`.
- Focused native tests: `meson test -C build <names> --print-errorlogs`; relevant names include `crt`, `entity`, `frontend`, `level`, `savegame`, `simtick`, `sound`, `media`, `cmd`. The native Meson build was previously reconfigured against system SDL3 pkg-config; do not needlessly reconfigure again.

## Picking the next batch

Use the latest report rather than repeating completed exact functions. Likely address-only candidates worth investigating include `global_value_0050080c_set` (`0x45c4a0`, `crt.cpp`), other unannotated CRT accessors, and standalone frontend state globals. Avoid blindly mapping `global_config_value_set` (`0x40f5e0`): original `0x004a1ee4` has initial dword 4, while the port currently initializes its counterpart to 0. Likewise `deferred_callback_set` (`0x453900`) targets a file-backed original dword 1 while the port's counterpart is zero-initialized. These require a runtime-design decision rather than a comment-only fix.

Previously attempted partial targets with known barriers: `menu_item_record_alloc` (`0x413f90`, string/CRT import placement); entity threshold/type flag helpers (`0x437cc0`, `0x4436f0`, compact entity offsets); `matrix4_set_identity` (`0x479e70`, float-constant labels); `message_queue_find_id` (`0x419b20`, original 0x14 record stride vs port 4-byte array); `audio_menu_volume_set` (`0x414df0`, constant placement); `selection_clear_request_and_hud_reset` (`0x41e750`, port five-argument command API vs original four); `sound_object_duration_get` (`0x465160`, original slot-zero sentinel conflicts with valid port slot 0); `vec_dot` and ray-plane helper (`0x4016f0`, `0x479a30`, operand-order rewrites did not improve scores). An array-search rewrite for `cmd_array_find_value_index` (`0x452650`) regressed its score and was reverted.

## Suggested skills

- Call the Skill tool for `reverse-engineer-anything` when inspecting `reference/LegoRR.exe` or original assembly/decompile evidence. Its source in this environment is the installed `reverse-engineer-anything` skill; follow it if available.
- No additional skill is required for routine report comparison or `GLOBAL` annotation work.

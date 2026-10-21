# Reconstruction is a tree of per-subsystem static libraries mirroring gods98

The reconstruction source tree (`src/`, relocated via the `src_root` meson
option) is split into one static library per subsystem — `gods98/` for the
engine core, `game/` for the game layer — with a thin final `LegoRR.exe`
consumer linking them. This deliberately mirrors the original codeline seen in
dev-tree strings (`gods98_dx6\gods98\src\{Main,Viewports,Images,Input}.c`,
finding 15) and the engine/game division the findings already document.

The alternative was a single monolithic target. We chose per-subsystem libs
because the other agent is landing decompiled modules one at a time: each
subdirectory's `meson.build` compiles whatever lives in it, so a module can
arrive incrementally — an empty subdir still compiles to an empty lib and the
whole tree stays green — and link order/symbol ownership for the eventual exe
is decided once, up front, rather than reverse-engineered from one big blob.

**Consequences:** `main.c` is the only file in the top-level `src/` link unit;
subsystem symbols are owned by their libs. A future sub-split (e.g. render/
or input/ carved out of `gods98/`) is a new directory + meson target, not a
rewrite. The stub tree is tracked in git for now; the gitignore-vs-track
question for the *real* decompiled modules is deferred until the other agent
produces a first buildable cut (Q2, 2026-10-20) — at which point it is a
one-line `.gitignore` change either way.
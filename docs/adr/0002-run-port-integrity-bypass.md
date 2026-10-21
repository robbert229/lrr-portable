# Rebuilt binaries bypass the A.2 integrity gate only via an explicit `--run-port` flag

The A.2 harness (`tools/compat/a2.py`) gates every launch on the staged
`LegoRR.exe` matching the evidence SHA-256 (`c3d8ce23…`, finding 01). A binary
rebuilt from the reconstruction will *never* match that digest, so it can
never run through the harness unless the gate is bypassed. When we wire
rebuilt-binary support into `a2.py`, the interface is a dedicated
`run --run-port <exe>` argument (not a generic `--exe` switch): it explicitly
skips the SHA gate, runs the given binary in place of the staged one, and
says so loudly in the run log.

Implementation is deferred until a real entry point exists — running the stub
`src/main.c` binary through Proton adds nothing today. The policy is recorded
now so nobody silently weakens the gate when wiring it. A launch of a rebuilt
binary is marked as `run-port` in the log and triage summary, so evidence
collected from a port run can never be mistaken for evidence from the shipped
binary.

**Consequences:** the shipped-binary integrity check stays strict and is the
only state `a2.py run` accepts by default. Port runs are opt-in, audited, and
labeled — preserving the provenance guarantee that all repo evidence traces
to the `c3d8ce23…` build (README §Reproducibility).
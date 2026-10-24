# Finding 400 — Compiler/Runtime No-op Stubs

This finding maps the three functions assigned to `runtime-support-unresolved`. The checked-in pseudocode shows each as an immediate return. Their caller contexts suggest expected runtime work, but do not establish what the original machine code did or whether the empty bodies reflect the intended executable behavior.

| Address | Name | Observed body | Caller/state flow |
| --- | --- | --- | --- |
| `0x00491ff5` | `CrtNoOpStub` | Takes no recovered parameters and returns immediately. | Called by `CrtAcosCore` (`0x0048e0ad`) when the incoming x87 control word differs from `0x027f`. The caller pseudocode assigns the call result to `in_EAX`, despite the callee's recovered `void` signature; the intended return/calling contract is unresolved. |
| `0x00493c40` | `CrtNoOpStubVariant2` | Takes no recovered parameters and returns immediately. | Called by `CrtDoubleToIntegerRound` (`0x004935c0`) with constants `1`, `4`, `8`, `0x10`, and `0x20` along classified status paths. The caller starts `uVar4` from `param_1 & 0x1f`, narrows that local mask in several branches, and returns whether it is zero; the stub itself does not visibly alter the local mask or FPU state. |
| `0x00497870` | `CrtNoOpStubVariant` | Takes no recovered parameters and returns immediately. | Called by `CrtFpuControlWordUpdate` (`0x00497770`) with the calculated merged control-word value. That caller returns the calculated value, but no x87 control-word write is visible in this path. Finding 399 describes the enclosing precision-update chain. |

## Data paths and boundaries

- `CrtAcosCore` reaches `CrtNoOpStub` only when the observed control word is not `0x027f`; the visible caller does not pass an explicit argument. The pseudocode is insufficient to identify a missing operation or its intended return contract.
- `CrtDoubleToIntegerRound` (`0x004935c0`) tests status bits in `param_1` against masks in `param_3`, calls `CrtNoOpStubVariant2` with constants on selected branches, narrows the local `uVar4` mask, and returns whether the mask is zero. No direct FPU-status mutation appears in the stub body.
- `CrtFpuControlWordUpdate` translates and merges control-word values before calling `CrtNoOpStubVariant`. The merged value is returned to its caller, but the checked-in stub contains no visible write instruction or side effect.
- These findings describe the repository's recovered pseudocode. Confirming whether the executable contains these same empty bodies, and what CRT/runtime operation the call sites expect, requires disassembly or runtime evidence.

## Open questions and focused follow-up

- Compare each address's machine code with the generated pseudocode and identify whether any body was lost during extraction or decompilation.
- Determine the intended x87 control-word update semantics on the `0x00497870` path and whether default precision should actually be changed; Finding 399 identifies this path as reached during CRT math initialization.
- Determine whether the calls to `0x00493c40` are intended to clear hardware exception flags or perform another status-related operation, using instruction-level evidence.
- Resolve the calling convention and return-value contract around `0x00491ff5` in the acos-like path.

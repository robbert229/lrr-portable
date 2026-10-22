# Finding 125 — Negative Entity Health Delta Raises the Screen-Shake Flag

Finding 10 left the `+0x3e4` `0x40000000` flag and `DAT_004a48b0` path as an
open screen-shake TODO. A focused Ghidra read of the shipped `LegoRR.exe`
shows one concrete raiser in `FUN_004475c0`:

- The function obtains the entity pointer from its caller's `+0x2fc` field.
- If that entity is present and the relevant `FUN_00469e80` mode bit `0x80000`
  is clear, it reads a value from `FUN_00469f20`.
- A nonnegative result is accumulated into entity `+0x340`. A negative result
  instead calls `FUN_0043f6a0(entity, -result, 1, 1.0f)` and ORs
  `0x40000000` into the entity's `+0x3e4` word.
- Afterward the function continues its `+0x340` progression and can clear the
  caller's active entity pointer when the progression finishes.

The code establishes that this flag is raised on the negative-result branch of
this evaluator, adjacent to a call that receives the negated magnitude. The
screen-shake interpretation is consistent with finding 10's earlier naming of
this bit and `DAT_004a48b0`; the pseudocode alone does not establish that the
call actually animates the camera or that the flag controls that global.

A fresh incoming-reference query for `DAT_004a48b0` returned four instruction
addresses: `0x43cf13`, `0x43cf93`, `0x43cf99`, and `0x43cfac`. This census does
not identify the references' read/write semantics or prove they are the full
runtime consumer chain. Their function-level interpretation remains open.

## Evidence and limits

`docs/evidence/legorr-screen-shake-raiser.bundle` contains the fresh full
function observation for `FUN_004475c0` and the incoming-reference inventory
for `DAT_004a48b0`. Both target the exact shipped executable digest recorded
in the bundle. The analysis is static Ghidra output. It does not establish
runtime camera motion, the meaning of the `FUN_0043f6a0` parameters, or a
complete consumer census for `+0x3e4` bit `0x40000000`.

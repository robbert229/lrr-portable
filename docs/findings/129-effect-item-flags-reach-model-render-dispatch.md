# Finding 129 — Effect item flag can enter the model render passes

This follows the indexed item state update in [finding 126](126-screen-space-effect-lane-dispatch-and-opacity-update.md) into the model render dispatcher described by [finding 49](49-model-render-pass-dispatch.md).

## Observed

`FUN_00483420(container, index, alpha, 4)` resolves its item as
`*(container + 8) + index * 0x70`. It edits that item's flags at `+0x6c` and,
for intermediate alpha values, sets bit `0x80000`. It also writes the clamped
alpha to a related record's `+0x10` field.

`FUN_00482d60` walks a container whose count is at `+0`, whose item-array
pointer is at `+8`, and whose items also use a `0x70` stride. For each item it
checks bit `0x80000` at `+0x6c`; when the bit is set, it invokes
`FUN_00483a90` in one of two ordered passes, distinguished by whether item
`+0x64` is zero. The renderer evidence establishes these predicates and calls,
not the visual meaning of the bit or the callee's complete effect on pixels.

## Conclusion and limits

The matching container layout, item stride, flag offset, and bit value provide
evidence that an item marked by the mode-4 update can be selected by the later
model-render dispatch. This is a structural link between the fade update and a
render-stage consumer of its flag; it does not establish screen-space
coordinates, a particular rendering backend, or the visual result.

The scalar written to the related record at `+0x10` is not traced here to a
later read. Therefore, this finding closes the indexed flag-consumer gap only;
the alpha scalar's consumer and the screen-space interpretation remain open.

## Evidence

- `ev_0f2a7d58b1f97fe75770cbcce87155c2beaac4bbe78f34b1c2a5d8ff31343bf4` —
  `FUN_00483420`, retained in
  [`legorr-screen-space-effect-lane.bundle`](../evidence/legorr-screen-space-effect-lane.bundle).
- `ev_d722ec9e85e35a60f03ef74728a5881604fcb8b9f863c83b5f024ae0b55211f8` —
  `FUN_00482d60`, retained in
  [`legorr-engine-expansion.bundle`](../evidence/legorr-engine-expansion.bundle).

Both source bundles passed REA `evidence-import` validation during this pass.

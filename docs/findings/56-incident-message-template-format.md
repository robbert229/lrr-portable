# 56 — Incident message text is precompiled into a small formatting record

Finding 47 follows entity transitions into message IDs `0x0a`, `0x0b`, and
`0x0c` (`FUN_0043bc40`,
`ev_cb67dd44b4a1a803dee82f4abe7eb06a9d4e5aad251bbfab9e77bcd31969d456`). The
catalog maps them to the three death-message slots (`FUN_00419230`,
`ev_f63affba6f717b3403a0ec7f529f5985850cddf6e753c2bba389a08b2c0ebab2`);
finding 22 covers their configured entries. The text path transforms each
configured entry before display. `FUN_00434c10` reads each `InfoMessages` text
entry and passes it to
`FUN_004194b0`, which frees the previous compiled buffer and calls
`FUN_00419530` to compile the replacement. [Evidence:
`ev_a6759eaccb7c5e9c735e17389ce51778182323c5c2e19d770ff301d74ae43841`,
`ev_33f8c423e3a6c732b36e888a9da6502deb3ff28e3bda521ed13c62ff7ef282b1`]

The compiler applies three visible rules:

- `_` becomes a space, and the two-character sequence `\n` becomes a line
  break.
- A `$token$` span is looked up through the `FUN_0045cbd0` path first; if that
  path does not resolve it, the compiler tries the message-name catalog via
  `FUN_004193e0` and `FUN_00419420`. A resolved value is emitted as `%b`, with
  a corresponding byte recorded in a small argument-type trailer.
- The output allocation contains the compiled NUL-terminated format followed
  by a count byte and five trailer slots. The binary evidence shows the
  representation; it does not identify every token's user-facing meaning or
  prove the compiler's maximum safe token count.

Evidence: `FUN_00419530` (`ev_30b260cc7f32ec2e698212bc42148e03fe086b34d8bb99203adecc9a0526e8e6`)
and the catalog lookup (`ev_0f14e4feb0e8190485f6db0c8e0b6862661287f67f296f84feb9e55c72a919d4`).

This is separate from the producer behavior in finding 47: the entity tick
chooses a message ID and a position, while the configured message text is
compiled by the config loader as a display template. The downstream formatter
accepts that
compiled string through `FUN_0048e1e0` before the text widget consumes it
(`ev_610c5351d7eea4aeb5322ae783137cc175e1802d245a31365ee20bb01dbb8462`).
All cited records are retained in
[`legorr-incident-message-rendering.bundle`](../evidence/legorr-incident-message-rendering.bundle).

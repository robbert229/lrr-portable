# Focused Win32 cancellation regression probe

Prepared by root as verification support for the two reviewed P2 pointer
cancellation corrections. **Not compiled or executed.** This does not pass a
runtime milestone, independently re-review the patch, or authorize a shared
build/launch. The combined candidate remains frozen at a16f672d.

Source lives outside all repository/Reccmp scan roots:
[/var/tmp/wine-pointer-cancel-probe/pointer_cancel_probe.cpp](/var/tmp/wine-pointer-cancel-probe/pointer_cancel_probe.cpp).
SHA256: `c6e36119aa00fbe191a08e703b5e51dbcf77db4c1b5157deb65c3c82fe6f9d88`.
Do not copy raw C/C++ backups or this probe into the repository; annotation
scanning has previously included backup source files.

This opt-in PE32 fixture exercises the actual integrated `input_capture_events`
and `platform_pump_events` APIs. It does not reimplement their algorithms:

- Empty-held cancellation followed by a fresh left DOWN must preserve that edge.
- A genuinely inherited cancelled hold stays suppressed through release; the
  next complete gesture is available.
- An owned-window subclass arranges a cross-thread sent WM_CANCELMODE after a
  DOWN has been dispatched. The next production PeekMessage must process that
  sent message even if no posted message remains, and return cancellation
  instead of the stale DOWN.
- A thread-local WH_GETMESSAGE fixture injects synchronous WM_CANCELMODE while
  the final keyboard event fills the batch. Earlier DOWN is removed, keyboard
  order retained, and cancellation delivered.
- When keyboard events fill every output slot, they remain intact; cancellation
  must survive into the next pump without an intervening pointer action.

The sent-message and hook checks assert their own trigger conditions. A fixture
failure or timeout must be retained and diagnosed separately from a production
failure. These synthetic Win32 messages do not validate physical held-button
polling, tutorial gestures, UI hit ownership, world actions, or gameplay parity.
No click on an invisible actor, fake terrain ray, forced game state, or test-only
application fallback is supplied.

After root authorizes pointer integration, the single designated integrator
should rebuild every production owner of the enlarged InputState and compile
this source as a separate MSVC5 PE32 executable using the established external
input-boundary probe link procedure. Link the real integrated input/platform
objects; do not include the game's entry-point object or substitute mock
implementations. Run only while reference/candidate are stopped, under the same
recorded Wine runner/prefix/environment and preservation protocol. This probe
creates and operates only its own named window. Archive source/object/executable
identities, exact command, stdout, exit status, fixture failures and restoration.
A passing result is exit0 with failures=0; no execution result currently exists.

# NERP SetMessage pointer contract correction

Prepared bounded root fix; **production source unchanged, build/test/runtime NOT
RUN**. Do not apply during the current objective validation freeze.

[One-hunk patch](wine-nerp-message-pointer.patch), SHA256
`c3288b9d98fa2a268822041c31c04b6fbfd95318c9dcccf1b821adc5c76c30aa`.
Baseline/result source and manifest are outside scan roots at
`/var/tmp/wine-nerp-message-pointer-fix/`. Read-only application against the current
parent passed. It composes with the separate two-leaf144/254 callback patch;
neither is integrated or runtime-verified.

Confirmed source defect: tutorial_builtin_command16 obtains the actual token
text but passes token_index to advisor_text_request_setup. That frontend entry
casts its dword argument to char* and calls advisor_inline_token_render; it does
not resolve a token ID. Message1 becomes NULL and later message IDs become small
addresses. The typed production API advisor_text_request_setup_text already
accepts the exact looked-up string and retains the correct owner; the patch uses
it. No fake text, range-of-address heuristic, guard swallowing invalid messages,
callback injection, forced state, or NEXT bypass is added.

Reference SetMessage0x4566d0 displays its looked-up string through0x46ac20; see
complete exported REA disassembly in /var/tmp/wine-tutorial-gate-audit/ and the
completed callback worker's handoff. The private token table owns that string
until token reload/release. This correction does not port history, sound-index
aliasing, timer acknowledgement, NEXT drawing/hits, or command200; those remain
separate explicit contracts. It does not turn missing tutorial progression into
a passing gameplay milestone.

The current instrumented candidate exited with make Error5 shortly after resume.
This pointer defect is a plausible independent application fault, **not a
runtime-established cause of that exit**. Do not attribute the termination to it
or to debugger interaction without the fault/caller evidence. Preserve the
current single debugger-free replay before changing the binary.

After a new authorized integration boundary, the sole integrator should build
MSVC5/reccmp once for this source delta and any reviewed leaf callbacks, archive
all score changes and exact identities, and exercise actual first/subsequent
SetMessage values through normal tutorial progression. A focused PE32 boundary
check may use real loaded token strings and the production text API; it must
not reimplement inline parsing or claim UI progression from direct calls.

Scope clarification for subsequent UI acceptance: scene_render.cpp's production
renderer draws real OList EntitySlot records and projects/picks their current
world pose without requiring attached_scene. Existing procedural rendering of
those real actors is a disclosed graphical difference, not an injected test
actor. A missing authored scene handle can invalidate the earlier strict probe's
precondition, but must not alone disqualify ordinary GUI evidence that shows the
actual selected OList actor moving. Full authored visual parity is not a new
requirement of the user's flow goal. Preserve every original outcome: permitted
normal selection, persistent feedback, chosen destinations, displacement/arrival,
pause/resume and clean shutdown. No dummy actor, teleported pose, invisible click,
scripted race or debug command can substitute.

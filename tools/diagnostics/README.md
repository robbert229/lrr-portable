# Win32 Wine runtime probes

These diagnostic fragments compile only for the MSVC5 x86 WinAPI backend and are intentionally included by the current
paused checkpoint at the user's request. They have no Reccmp address annotations.
The UI probe is included in gods98/frameloop.cpp; the NERP owner probe is included
at the end of game/tutorial_hud.cpp to inspect that translation unit's actual
private owner. Keep their inclusion positions when editing them.

The frame-loop invocation is enabled only when LRR_MESSAGE_UI_PROBE names a
writable Windows-visible log path and the initialized game has a real message
and NEXT-enable state. It runs once. Both probes deliberately manipulate UI/
NERP fixture state; that process is diagnostic and cannot establish ordinary
user-flow acceptance. An unset environment variable leaves the invocation idle.

Probe verdict0 means the specific assertions passed,1 means a failure,2 means
missing prerequisites/inconclusive. See the probe code and the ignored phase
packet for exact coverage and executed results. No assertion or build success
independently proves real entity selection or commanded movement.

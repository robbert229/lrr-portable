# 94 — Objective advisor rendering consumes inline resource and sound tokens

`FUN_00456830` parses the objective/advisor text against the two token tables
described in finding 93. A known `<name>` token selects its resource handle and
places that handle through `FUN_0047dcb0`, distributing multiple selected
handles using the configured spacing values. A known `#name#` token selects a
sound handle. The sound path only proceeds when the token index differs from
the last one and the audio availability check succeeds; it then submits the
sound through `FUN_0047ad70`. If `FUN_00456240(0)` is zero, the path also
requests advisor animation slot `0x14` through `FUN_004018c0`. Finding 211
refines the ordering: the parser stores the new token index before checking
whether sound is enabled, so the same token is not retried if sound becomes
enabled while that token remains active.

The display worker `FUN_0046ad00` passes the active stream string through this
parser into a scratch buffer, then sends the resulting text to the text widget
with `FUN_00485300`. The request setup helper `FUN_0046ac20` installs the active
stream string and initializes the widget's request state before the worker
processes it. These are static data/control-flow observations; they do not
show which level stream is selected or whether a submitted sound is audible at
runtime.

Evidence: `ev_08a76216129fbf8417817ac2ffbda7a26a9f0e5175e59f6fe706d93cbf1ee324`,
`ev_6ca2790e9b0b5f91aa356f3625f0cab7acdba622b279f3dbddbd63636a725807`,
`ev_5bb2ff083e57cf615da2c826a6e276c53e4cebf53cfc83a3f0a0ce7805eb8ddf`, and
`ev_1eee8ff453c8380c00d0888f5c8669953106b577dea44a9c50178ea5311c23bd` in
[`legorr-objective-advisor-inline-markup.bundle`](../evidence/legorr-objective-advisor-inline-markup.bundle).

| Claim | State |
| --- | --- |
| Known angle-bracket tokens select and position resource handles | observed (`FUN_00456830`) |
| Known hash-delimited tokens use the sound table and suppress consecutive repeats by token index | observed (`FUN_00456830`) |
| A successful sound path may request advisor slot `0x14` under the observed state gate | observed (`FUN_00456830`) |
| Parsed text is passed to the text widget by the display worker | observed (`FUN_0046ad00`) |
| Runtime display and audibility for any specific stream | unknown; static submission does not establish playback |

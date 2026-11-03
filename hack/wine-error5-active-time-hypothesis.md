# Error5: active-time hypothesis

Historical/source inference, not an observed fault identity. This changes the
next diagnostic test; it does not establish the cause or justify a forced pass.

Current command16 passes numeric token_index to an API that parses it as char*.
The first message maps to0; the next maps to1. M01.nrn Intro shows message1,
waits through GetMessageTimer, resetsR0 and incrementsR5, then submits message2.
The current timer decrements1000 by delta*1000*0.04; the application dispatches
EngineTick through engine_callback_triplet_tick_dispatch(dt) using seconds, and
EngineTick passes dt to frame_tutorial_script_dispatch. Thus with normal scale
and no other state changes,1000 timer units correspond to roughly25 unpaused
seconds. The startup audit already identified the broader seconds/25Hz mismatch;
this note is not authorization for a global timing change.

Relevant evidence from completed50cc objective packet:

- Instrumented final ack returned23:25:26.867266. Fresh pause DOWN completed
 23:25:50.229646. Continue was delivered23:26:39.276504; exit was23:26:40.537376.
 Excluding the long pause, that is approximately25 active seconds. Exact event
 send/processing time and debugger stops prevent an exact timer reconstruction.
- Ordinary final verified fresh acceptance action completed23:31:49.614864,
 scene confirmed23:31:51.679221. Pause action23:32:02.512091; Continue23:32:11.977556;
 second pause23:32:22.923406; fresh Escape resume23:32:29.125170; exit23:32:31.249329.
 The corresponding active intervals sum to about26 seconds. Earlier screenshot
 labels/failed input attempts cannot be assumed to identify acknowledgement.
- New unchanged50cc diagnostic run red-tutorial showed scene23:42:05.973686,
 pauses/resumes in the next12 seconds, then close request23:42:17.678268/exit0.
 Its unpaused interval was substantially shorter than25 seconds. It used+seh
 diagnostics and is not a clean equivalent final acceptance pair.

Ranked falsifiable hypotheses:

1. Timed SetMessage2 reaches invalid text pointer1. Predict an unchanged ordinary
 tutorial left unpaused for over30 seconds after final acknowledgement fails
 without WM_CLOSE; fault/caller is inline message parsing with the small request.
2. True teardown lifetime fault. Predict no idle-time failure and the actual
 fault follows close/shutdown entry in a cleanup owner.
3. Wine window/display interaction. Predict fault in that interface and a
 relationship to focus/window/destruction state rather than the NERP transition.

The sole Q integrator should prioritize ONE captured unpaused dwell without close
if the current diagnostic has not already established the fault. Use normal menu,
real briefing and no debugger controls/forced flags. Mark exception/exit timing
against the close send and real timer/message state where passive evidence is
available. Do not repeatedly run short green close cycles as a red-capable test.
A pointer-cause result would authorize the already-prepared one-hunk real typed
API correction; otherwise keep that repair separate. Preserve all attempts and
restore assets/prefix before any next run.

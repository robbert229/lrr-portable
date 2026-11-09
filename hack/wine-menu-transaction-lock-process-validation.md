# Scratch transaction lock: process validation

2026-11-03. This is a tooling prerequisite experiment, not menu parity acceptance. The executable seam is `/tmp/lrr-lock-process-validation-60890/validate.py` (temporary, not a production helper). It creates private disposable fixture directories and removes them. No Wine, game, build, canonical lock, or protected state was accessed.

Command and actual output:

```text
$ python3 /tmp/lrr-lock-process-validation-60890/validate.py
PASS separate process contender denied
PASS missing and wrong capability denied by holder
PASS forged stale state cannot authorize
PASS pidfd binds actual holder process
PASS lock retained through dummy stages and verification
PASS unrelated exec child does not retain lock after owner exit
PASS killed holder cancels cooperative in-flight fixture; lock released; stale token denied
PASS replaced inode refuses admission
PASS symlink refused
PASS fixture processes exited
```

The controller is a real subprocess holding an exclusive nonblocking flock and serving a Unix socket in a private temporary directory. A random capability is sent to it over an inherited socketpair and to each separately executed client over stdin; it is not put in argv, environment, logs, or a fixture file. The holder checks same-UID peer credentials, capability, and lock pathname/descriptor identity before replying. A separate process tries the flock at every dummy stage. A real client waits on its admitted connection; killing the holder causes EOF, which that cooperative client treats as cancellation. A forged file and a stale capability do not substitute for the live endpoint. A pidfd was opened for the first holder and became readable when that process exited; this binds the observed process identity without relying on PID alone. The socket connection, not a PID read from a file, is the admission endpoint.

The listed stages are requests, not enforcement of an ordered transaction state machine, and the dummy verification is only an acknowledged request. The fixture does not verify real cleanup or prevent a client from continuing arbitrary work after admission. The child inheritance check covers a separately execed process that did not receive the lock descriptor; it does not exercise a holder spawning a child with explicit descriptor passing, a fork-only child, or every crash path. The pidfd observation does not simulate PID reuse or wire pidfd checks into admission. Socket authentication is only same-UID plus possession of the capability; same-UID adversaries and capability disclosure are outside this test. Replacement is detected on the next request, not atomically prevented, and the symlink test covers the final lock component only. The holder does not recheck the lock after admitting an operation or implement helper supervision, bounded writes, deadlines, durable recovery, or actual restore. The test does not scan process environments or logs for leakage beyond the described fixture design. A temporary script may disappear when `/tmp` is cleaned.

For integration, keep the holder alive and the lock held until all owned writers have stopped and actual restore and cleanup have been verified; enforce phase ordering and recheck ownership at sensitive boundaries. Helpers must monitor the live holder throughout bounded operations and stop safely on disconnect, with controller supervision and explicit failure handling. A pre-operation authorization alone cannot interrupt an already executing write. Use a private, verified directory, never replace the canonical lock inode, refuse unsafe paths, and use pidfd or PID plus kernel start time wherever process identity is required. Exercise these requirements against the real helper and its actual children before relying on them.

A crash releases flock even if a dummy operation is incomplete. Robust crash recovery therefore requires a durable transaction journal written before mutation, recording enough state to identify incomplete work, plus a recovery gate that refuses normal transactions until restoration is verified or a human performs recovery. The fixture does not establish crash-safe journaling or recovery. Advisory flock excludes cooperating participants only; it cannot exclude human or other uncooperative writers. Independent process guards and ownership procedures remain necessary.

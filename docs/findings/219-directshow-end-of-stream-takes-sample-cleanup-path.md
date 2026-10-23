# Finding 219 — DirectShow End of Stream Takes the Sample Cleanup Path

The configured-video sample callback accepts only `S_OK` from
`IStreamSample::Update`. Because it calls `Update(0, NULL, NULL, NULL)`, the
operation is synchronous; Microsoft's documented `MS_S_ENDOFSTREAM` result
therefore takes the callback's nonzero-result branch. That branch releases the
sample-related interfaces and returns false, which the configured-video
wrappers treat as the end of the playback invocation (findings 200–201).
Thus normal stream exhaustion and update errors share the same stop-and-cleanup
path in this binary.

**Evidence.** In shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
`FUN_004724a0` (`0x4724a0`) invokes the sample interface's vtable slot `+0x18`
with four zero arguments, then branches on `iVar2 == 0`. Only that branch blits
and returns the blit's zero-result test. Every other returned value reaches the
interface-release sequence and returns false. The official [Microsoft
`IStreamSample::Update` reference](https://learn.microsoft.com/en-us/previous-versions/ms786825%28v%3Dvs.85%29)
states that null event and APC parameters with no asynchronous flag perform a
synchronous update, and lists both `S_OK` and `MS_S_ENDOFSTREAM` as return
values; the latter means the stream ended and the sample was not updated.
Finding 201 traces the resulting false callback through wrapper cleanup.

This records status-to-control-flow behavior only. It does not establish that
the game mishandles stream completion; ending playback at end-of-stream is
consistent with the wrapper's loop.

# Finding 413 — Configured DirectShow video path

Configured boot and level videos use a DirectShow multimedia-stream graph and
DirectDraw samples. The graph is initialized from the configured path, its
primary video stream is adapted to an `IDirectDrawMediaStream`, and decoded
samples are copied to the game's shared secondary render surface. The normal
render step then presents that surface. This route is separate from the AVI
frame-retriever path used for credits movies (Finding 187).

## Assigned function map

| Address | Function | Observed role and relationships |
| --- | --- | --- |
| `0x004721e0` | `DirectShowSampleSetup` | Queries the primary stream for `IDirectDrawMediaStream`, creates a sample, obtains the sample's DirectDraw surface, queries it for `IDirectDrawSurface3`, and starts the stored media stream with state `1`. Each HRESULT is checked; a failure path releases acquired interfaces and returns false. See Findings 200 and 207. |
| `0x004722b0` | `ConfiguredVideoGraphSetup` | Creates the `AMMultiMediaStream` COM object, initializes it, adds the primary video stream, attempts to add primary audio, and opens the requested file. Video-stream addition failure exits setup; audio-stream addition failure is cleared and setup continues to `OpenFile` (Finding 208). |
| `0x00472390` | `ConfiguredVideoComRouteInit` | Initializes the wrapper's interface slots and stores the supplied path and configuration values, calls `CoInitialize`, copies the path into owned storage, builds the graph, then invokes sample setup when graph setup succeeds. It does not branch on the boolean result from sample setup; the effect of a sample-setup failure on the wrapper returned to callers remains unresolved. |
| `0x00472430` | `DirectShowInterfaceRelease` | Releases non-null graph, stream, sample, and surface interfaces held in the wrapper and frees its copied path. It does not call `SetState(STREAMSTATE_STOP)` before release (Finding 207); the graph's internal stop behavior on final release is not established. |
| `0x004724a0` | `ConfiguredVideoSampleUpdate` | Calls the sample's update slot with four zero arguments. On a zero HRESULT it blits the full sample surface to the stored destination using `DDBLT_WAIT`, returning whether the blit succeeded. A failed sample update releases several interfaces and returns false (Finding 200). The `Blt` rectangles are null, so the copy uses the full surfaces and does not apply the caller-computed rectangle (Finding 206). |
| `0x00472530` | `VideoDurationFrameBoundUpdate` | Calls `IMultiMediaStream::GetDuration`, ignores its HRESULT, and converts the 100-nanosecond duration to milliseconds by dividing by 10,000 (Finding 202). |
| `0x00472560` | `DirectShowRenderSurfaceCreate` | Resolves the configured resource path, gets the DirectShow/DirectDraw interfaces and the game's render surface, allocates a `0x38`-byte wrapper, and initializes its COM route. `MoviePlayNamed` and `MoviePlayLevelVideo` call it, pass the result into `MoviePlayWait`, and destroy it afterward (Findings 187 and 201). |
| `0x00472670` | `DirectDrawSurfaceDescriptionGet` | Queries the stored DirectDraw surface with a `0x6c`-byte surface-description record and writes returned description fields to caller outputs. `DirectShowVideoPlaybackLoop` converts those dimensions into a rectangle when its optional mode is active, but the sample callback still passes null rectangles to `Blt` (Findings 200 and 206). |
| `0x004726c0` | `VideoFrameBoundRefresh` | Thin forwarding wrapper around `VideoDurationFrameBoundUpdate`; `MoviePlayWait` uses the resulting duration bound during its render loop (Finding 202). |
| `0x004726d0` | `DirectShowSampleUpdateAndBlit` | Thin wrapper around the sample callback. It returns the callback's boolean result; the boot/level loops use false to leave the current playback invocation and then destroy the wrapper (Finding 201). |
| `0x004726f0` | `DirectShowWrapperDestroy` | If the wrapper is non-null, calls `DirectShowInterfaceRelease` and frees the wrapper allocation. Boot and level movie wrappers call this after playback returns (Finding 201). |

## Data and control flow

The resource/configuration callers resolve a video path and create the wrapper
through `DirectShowRenderSurfaceCreate`. `ConfiguredVideoComRouteInit` copies
the path, initializes COM, and asks `ConfiguredVideoGraphSetup` to create and
open the multimedia stream. Graph setup requires a primary video stream, while
primary audio is best effort. `DirectShowSampleSetup` then creates a DirectDraw
sample for the primary video stream and sets the graph running.

`MoviePlayWait` requests a sample, renders the game's current frame, increments
an iteration counter, compares it to the duration bound, checks skip/stop
input, and requests the next sample. That duration is a loose render-count
bound, not a frame clock; the synchronous sample update is the operation that
can wait for video data (Finding 202). A failed sample update or blit returns
false and ends the playback invocation without retrying that same stream
(Finding 201). Successful sample copies go to the shared secondary render
surface and are presented by the existing windowed blit or fullscreen flip
path (Finding 205).

After normal completion, skip, or update failure, callers release their
wrapper interfaces and free the wrapper. The game does not issue an explicit
`STREAMSTATE_STOP`; whether releasing the COM references stops the graph
immediately is unknown (Finding 207).

## Open question

`DirectShowSampleSetup` returns a boolean and releases partial interfaces when
it fails. `ConfiguredVideoComRouteInit` calls it but does not test that return
value. The decompiler also shows the route initializer's result flowing back
through `DirectShowRenderSurfaceCreate`, despite the initializer being rendered
as `void`. Confirming the constructor/return ABI and whether callers can enter
playback with a partially initialized wrapper would require instruction-level
validation and, if safe, a controlled setup-failure test. This static evidence
is not enough to claim a use-after-release or other runtime defect.

## Evidence and related findings

The checked-in Ghidra pseudocode for all 11 functions and callers is in
[`LEGORR.full.c`](../decompiles/full/LEGORR.full.c); their names and addresses
are also in [`manifest.csv`](../decompiles/full/manifest.csv). All 11 functions
were already assigned to `video-playback` in
`docs/coverage/function-areas.csv`, and their runtime-import/function records
were already present in the coverage ledger, so coverage indexes need no row
changes.

Related findings: [Finding 187](187-video-commands-use-two-playback-routes.md),
[Finding 189](189-configured-videos-add-primary-audio-stream.md),
[Finding 200](200-configured-video-polls-directshow-sample.md),
[Finding 201](201-configured-video-stops-after-update-failure.md),
[Finding 202](202-video-duration-only-bounds-render-count.md),
[Finding 205](205-directshow-video-uses-shared-render-presentation.md),
[Finding 206](206-configured-video-blit-fills-render-surface.md),
[Finding 207](207-configured-video-releases-without-explicit-stop.md), and
[Finding 208](208-video-setup-ignores-audio-stream-add-failure.md).

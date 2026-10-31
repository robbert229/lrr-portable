# Finding 406 — Audio Playback, Mixing, and Spatial Sound

This finding maps the 57 functions assigned to `audio-playback-mixing`. The runtime splits into DirectSound sample playback, attached 3D sources/listener state, finite WAV streams, and CD-DA music. Findings 14 and 62 plus the focused notes listed below contain the detailed binary-level traces; this inventory connects the assigned functions and their ownership/call paths without repeating those analyses.

## Device, mixer, and sound-bank setup

| Address | Function | Observed role |
| --- | --- | --- |
| `0x00478980` | `MusicMixerVolumeSet` | Calls the shared mixer helper in write mode to set the music mixer level. |
| `0x004789a0` | `MusicMixerVolumeGet` | Calls the shared mixer helper in read mode to return the current level. |
| `0x004789c0` | `WindowsMixerControlInit` | Enumerates Windows mixer devices and source controls for the CD-audio component; reads or writes the selected volume and normalizes requested values to 0–1 (Findings 159–160). |
| `0x0047a640` | `DirectSoundSystemInit` | Creates DirectSound, sets cooperative level and primary buffer, queries the 3D listener, applies listener defaults, and marks the sound system active on success (Findings 119 and 199). |
| `0x0047a800` | `DirectSoundObjectsShutdown` | When sound is active, restores the primary-buffer volume, stops/releases DirectSound objects, clears the device pointer, and disables the subsystem. |
| `0x0047a850` | `SoundVolumeValueValid` | Accepts integer DirectSound volume values from -10000 through 0. |
| `0x0047a870` | `SoundBankEntryLoad` | Claims a bank slot, forms a WAV path, loads either an in-memory sample or streamed-file metadata, and records volume/loop/stream flags. The table has 600 records of stride `0x134` (Finding 14). |
| `0x0047aad0` | `SoundBankFreeSlotFind` | Scans the 600-entry bank for the first record whose occupied flag is clear; returns its index or failure. |
| `0x0047c160` | `ConstantNegative10000Get` | Returns the DirectSound minimum-volume constant `-10000`. |

The sound bank is the junction between config/gameplay aliases and output buffers: in-memory WAVs flow through `InMemoryWaveSampleLoad`, while streamed entries retain file size and a format-rate probe for later playback. Findings 107, 120, 122, 133, and 215–218 trace alias selection, buffer duplication, data copying, and failure paths.

## Listener, source positioning, and tracked playback

| Address | Function | Observed role |
| --- | --- | --- |
| `0x0047ab80` | `DirectSoundListenerViewSet` | Stores the current view/frame provider while sound is enabled. |
| `0x0047aba0` | `DirectSoundListenerUpdateTick` | Calls the listener spatial update for the stored provider during the frame tick. |
| `0x0047abc0` | `DirectSoundListenerSpatialUpdate` | Gets scene-relative listener position and orientation, updates DirectSound listener fields when values change, and commits deferred settings (Findings 163, 196, and 203). |
| `0x0047ad10` | `SoundPositionSetFromVector` | Applies an explicit position to a selected sound buffer immediately (Finding 164). |
| `0x0047ad30` | `AttachedSoundStopByEntity` | Finds an attached sound association for the requested entity/provider and stops/removes its tracked sound handle. |
| `0x0047ad70` | `SoundBufferPlayWithPosition` | Resolves a bank entry, selects its single buffer or rotates among up to three duplicates, applies source position/volume, starts playback, and returns an encoded bank/buffer handle (Findings 133 and 168). |
| `0x0047b020` | `SoundBufferAssociationAdd` | Allocates a record for three supplied values and prepends it to the global association list; the second value is the buffer interface used by stop-all and liveness checks. |
| `0x0047b0f0` | `SoundHandleToBuffer` | Decodes a valid sound handle to its DirectSound buffer pointer; invalid handles return null. |
| `0x0047b130` | `SoundBufferStopHandle` | Resolves a handle to a buffer and stops that buffer when the subsystem and handle are valid. |
| `0x0047b160` | `SoundSubsystemStopAll` | Walks the global association list and calls `Stop` on each stored DirectSound buffer, then stops both streamed slots. |
| `0x0047b1a0` | `SoundInstanceTrackRegister` | Links an attached sound record into its provider's list and registers that provider in the bounded spatial-refresh table. |
| `0x0047b220` | `SoundInstanceTrackStopAndRemove` | Removes a tracked sound record and clears matching provider slots; when sounds share a provider, one stop can remove the shared refresh registration (Finding 204). |
| `0x0047b390` | `SoundSourceSpatialRefreshAll` | Refreshes registered providers, removes association records whose buffer liveness check fails, and advances both streamed-WAV ring buffers. |
| `0x0047b410` | `SoundSourceSpatialRefresh` | Queries a provider frame's scene-relative position, updates linked source buffers when it differs from cached state, and stores the new cache (Findings 162, 198, and 203). |
| `0x0047b4a0` | `SoundTrackedBuffersSetMinDistance` | Stores the minimum-distance setting and propagates it to every tracked 3D buffer (Finding 121). |
| `0x0047b4d0` | `SoundTrackedBuffersSetMaxDistance` | Stores the maximum-distance setting and propagates it to every tracked 3D buffer (Finding 121). |
| `0x0047c100` | `DirectSoundListenerRolloffSet` | Sets the listener rolloff factor when sound is active and returns the supplied factor. |

The listener and attached-source paths share Direct3DRM frame providers but have separate caches: the listener stores position/orientation globally, while each attached sound record stores a source position. Findings 162–164 and 196–204 cover the frame-query, cache, and removal behavior. The initialization path leaves DirectSound's distance factor at its default (Finding 199).

## Volumes, in-memory WAVs, and streamed playback

| Address | Function | Observed role |
| --- | --- | --- |
| `0x0047b500` | `SoundBufferVolumeSet` | Sets the primary DirectSound buffer volume if the requested value passes `SoundVolumeValueValid`; also caches the primary volume. |
| `0x0047b530` | `SoundVolumeIndexSet` | Maps the game's volume index through its lookup table and applies the resulting DirectSound volume (Findings 156 and 158). |
| `0x0047b550` | `SoundBufferVolumeSetMinus300` | Sets and caches the primary buffer volume at -300 hundredths of a decibel during initialization. |
| `0x0047b580` | `InMemoryWaveSampleLoad` | Opens/parses the WAV data chunk, allocates and fills a PCM payload, then creates/fills DirectSound buffers; some post-parse failure paths leave the MMIO handle open (Findings 122 and 215). |
| `0x0047b6c0` | `SoundRecordCreateBuffers` | Creates the primary DirectSound buffer from the parsed format/data and stores it in the bank record; requests 3D control for these buffers (Finding 121). |
| `0x0047b790` | `WaveSampleDuplicateBufferCreate` | Creates optional duplicate DirectSound buffers for a loaded sample; playback rotates through them when enabled (Findings 120 and 133). |
| `0x0047b8e0` | `StreamedWaveBufferStepRate` | Computes a stream-duration/rate value from stored file size and average bytes per second; the denominator is not guarded against zero (Finding 217). |
| `0x0047b970` | `StreamedWavePlaybackStart` | Replaces prior state for the selected stream slot, prepares its WAV/ring buffer, resets playback position, and starts looping DirectSound playback; position/play HRESULTs do not affect the success marker (Findings 136 and 153). |
| `0x0047ba20` | `StreamPlaybackStopAndRelease` | Clears the selected stream's active state, stops/releases its DirectSound buffer, and frees the format allocation and MMIO handle. |
| `0x0047baa0` | `StreamedWaveFormatAndBufferSetup` | Parses the WAV through MMIO, rejects non-PCM data, opens the data chunk, and creates an aligned four-segment DirectSound ring buffer of roughly three seconds (Findings 150–151). |
| `0x0047bc30` | `StreamedWavePlaybackDrainCheck` | Primes the selected ring buffer from the WAV source, pads a short initial read with silence, unlocks the buffer, and initializes queued-byte/cursor state. |
| `0x0047bdb0` | `StreamedWaveRingBufferTick` | Reads stream data into locked DirectSound ring regions as cursors advance, pads exhausted input with silence, and stops/releases after the queued data drains (Findings 152 and 154). |
| `0x0047f660` | `SoundPathFallbackFormat` | Formats the fallback path used when a named sound is not found at its initial resource path. |
| `0x0047fb60` | `SoundSampleFileOpenByName` | Opens a named sample through the game's file/resource path, including the sound-path fallback. |
| `0x00488c70` | `WaveFormatOpenAndRead` | Parses a RIFF/WAVE `fmt ` chunk from a memory buffer, allocates any format extension bytes, and returns an open MMIO memory handle (Findings 122 and 222). |
| `0x00488e70` | `WaveFileAverageBytesPerSecondRead` | Opens a standard path with MMIO, descends RIFF/WAVE and `fmt ` chunks, reads the average-bytes-per-second field, and closes the handle. |
| `0x00488f10` | `WaveFileOpenReadChunks` | Opens a standard path with MMIO and parses the RIFF/WAVE format chunk, returning its handle, format allocation, and chunk state. |
| `0x004890c0` | `WaveDataChunkOpen` | Seeks to and descends into a WAV `data` chunk under the supplied RIFF parent. |
| `0x00489100` | `WaveDataRead` | Copies the requested bytes from the MMIO read window, advancing the window and tracking remaining payload length. |
| `0x004891d0` | `MmioHandleAndGlobalBufferRelease` | Frees the format allocation and closes/clears the associated MMIO handle. |
| `0x0048f2b0` | `StreamedAudioFileLengthGet` | Opens a streamed audio path through the game file layer, returns its file length, and closes the handle. |

The two streamed slots use distinct state blocks. A frame update refills their looping buffers from the finite WAV source; the WAV does not repeat when its source reaches EOF. The registered-path metadata probe uses a separate standard-path MMIO open from the game-file-layer size query (Findings 154 and 217–218).

## CD-audio playback and lifecycle

| Address | Function | Observed role |
| --- | --- | --- |
| `0x00488b50` | `CDAudioDeviceStateUpdate` | Updates the CD-audio availability state and resets the current-track control flag from the supplied initialization result. |
| `0x00488b90` | `CdAudioPlaybackAvailable` | Returns true only when both CD-audio and the sound subsystem are enabled. |
| `0x00488bb0` | `CdAudioTrackStart` | Stores the track/callback state and opens/starts the selected MCI CD-audio track when available. |
| `0x00488bf0` | `CdAudioStopAndCloseIfActive` | Calls the CD stop/close routine only while CD audio is available. |
| `0x00488c00` | `CDAudioWatchdogTick` | Every four seconds, polls the current track and restarts it when needed, then calls the saved completion callback (Findings 14 and 180). |
| `0x00489210` | `CDAudioTrackPlayFrom` | Sends the MCI `play cdaudio from N` command, converting the zero-based game track to MCI's one-based track number. |
| `0x00489260` | `CDAudioErrorTextGet` | Converts the saved MCI error code into a human-readable error string. |
| `0x00489280` | `CDAudioTrackCompletionPoll` | Queries MCI mode/current-track status and reports whether playback is at or before the selected track; query-error handling is qualified in Finding 195. |
| `0x00489330` | `CDAudioDeviceOpen` | Opens the MCI CD-audio device, selects track/minute/second/frame time format, then starts playback; converts command failures to MCI error text. |
| `0x004893a0` | `CdAudioStopAndClose` | Sends MCI stop and close commands for the CD-audio device and reports command success. |

Music is CD-DA, not an archived music-file stream. The watchdog uses `timeGetTime` and a four-second poll interval; because playback is issued without an end-track argument, it can pass into a following track before the watchdog detects completion (Findings 14 and 180). The sound controls target the Windows CD-audio mixer source rather than the game's DirectSound primary buffer (Findings 159–160).

## Limits and related findings

- The winapi backend issues CD-DA operations through MCI and reads its device status/error text from MCI. Native SDL has no CD-DA device path, so these calls report unavailable instead of treating the game's PCM output stream as a disc player. `WindowsMixerControlInit` is represented by the active software-output gain; the CD-audio mixer source itself is not available through the native backend.
- The native audio surface has no DirectSound ring-buffer cursor or buffer-lock interface. Each streamed slot now owns a separate SDL queue, so stopping one stream clears only that voice; refill uses the backend's queued playback duration rather than a fixed frame-clock estimate. Direct3DRM-attached source refresh still needs a frame/provider adapter; native SDL has no matching COM position setter, so that path remains inactive until one is supplied.
- Static decompilation establishes control flow and state writes, not audibility or actual DirectSound/MCI success. Specific ignored HRESULTs, cache-on-failure behavior, and queue overflow paths are isolated in Findings 153, 168, 198, 203–204, and 217–218.
- The streamed metadata probe and file-layer size/read paths can diverge: Findings 217–218 describe the unchecked probe and standard-path MMIO open; Finding 11 documents the game file/WAD layer.
- Existing detail: Findings 14 (audio formats and subsystem map), 62 (distance controls), 107–108 (aliases/streamed WAV), 119–164 (DirectSound, source buffers, mixer volumes, stream ring), 168–171 and 176–180 (sound request/track behavior), 195–218 (failure paths and spatial/CD details), and 345 (sound-object duration).
- Focused follow-ups, if needed: establish runtime behavior of the 20-entry attached-source provider registry under shared providers; determine which installations expose streamed paths to the separate MMIO probe; exercise sample start and streaming failure paths on supported backends.

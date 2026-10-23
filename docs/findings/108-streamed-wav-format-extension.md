# 108 — Streamed WAV Parser Preserves Format Extensions Before a PCM Gate

The streamed-WAV parser `FUN_00488f10` accepts a `fmt ` chunk larger than the 16-byte base WAVEFORMAT fields. For format tag 1 (PCM), it sets the extension count to zero. For other tags, it reads `cbSize`, allocates space for the 18-byte WAVEFORMATEX header plus extension, and retains those extension bytes in the returned format descriptor.

The parser itself preserves non-PCM format metadata, but this does not mean streamed playback accepts it. `FUN_0047baa0` checks the returned format tag and releases the parsed format and returns failure unless the tag is `1` (PCM). The DirectSound buffer-creation call is reached only after that check, so non-PCM extensions do not reach the buffer-creation API through this path. Finding 150 traces this caller-side gate.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-streamed-wav-format.bundle`. Focused records: `ev_924b7b3de6c79a1a97b8e4deec48b1cf86361a7daecd5e411f6eece84233793d` (`FUN_00488f10`); `ev_7cc236e0d2189886aaec15f33fb0c43e84e0f9320de17e56d30b266a46aff417` (`FUN_0047baa0`); `ev_6486d5a9df603739f3e911a3ddbecf8de816bbeef37a6d5935882e522703faf8` (stream caller); `ev_9c898a0012ef190ca00dec848ecac16e70fddc5041194f9a169c9f3b63ff85a5` (parser caller inventory); and `ev_6801cc2525322f3dddfa4e4104dc98f1b2d7dd88486977162affce80d804e3ac` (buffer-creation caller).

**Limitations.** This is static shipped-binary evidence. It establishes the format-tag gate in the helper, not whether every malformed or unusual PCM file is accepted by DirectSound. Finding 14 documents the broader RIFF/WAVE path.

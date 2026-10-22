# 108 — Streamed WAV loading preserves non-PCM format extensions

The streamed-WAV parser `FUN_00488f10` accepts a `fmt ` chunk larger than the 16-byte base WAVEFORMAT fields. For format tag 1 (PCM), it sets the extension count to zero. For other tags, it reads `cbSize`, allocates space for the 18-byte WAVEFORMATEX header plus extension, and retains those extension bytes in the returned format descriptor.

`FUN_0047baa0` consumes that descriptor when it calls the DirectSound device's buffer-creation method (vtable offset `+0x0c`). Its caller `FUN_0047b970` connects the parser and buffer creation to the streamed playback path. The parser therefore carries non-PCM format metadata to the API boundary rather than normalizing every stream to PCM.

**Evidence.** REA/Ghidra 12.1.4, shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. Bundle: `docs/evidence/legorr-streamed-wav-format.bundle`. Focused records: `ev_924b7b3de6c79a1a97b8e4deec48b1cf86361a7daecd5e411f6eece84233793d` (`FUN_00488f10`); `ev_7cc236e0d2189886aaec15f33fb0c43e84e0f9320de17e56d30b266a46aff417` (`FUN_0047baa0`); `ev_6486d5a9df603739f3e911a3ddbecf8de816bbeef37a6d5935882e522703faf8` (stream caller); `ev_9c898a0012ef190ca00dec848ecac16e70fddc5041194f9a169c9f3b63ff85a5` (parser caller inventory); and `ev_6801cc2525322f3dddfa4e4104dc98f1b2d7dd88486977162affce80d804e3ac` (buffer-creation caller).

**Limitations.** This is static shipped-binary evidence. It does not establish that the installed DirectSound implementation accepts or plays a particular non-PCM format, or that such an asset exists in the shipped WAD. Finding 14 documents the broader RIFF/WAVE path; this finding adds the streamed parser's extension handling.

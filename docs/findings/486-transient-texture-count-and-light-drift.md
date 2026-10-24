# Finding 486 — Transient Texture Count and Light Drift

`0x00552130` stores the number of preloaded transient-effect texture handles. `TransientEffectTexturesPreload` assigns the requested count and fills the handle array beginning at `0x00552100`; `TransientEffectEmitterCreate` selects a handle using its effect step modulo this count. `SceneEffectRegistryReset` clears the count.

`0x004EADF8` stores the maximum random X-axis light-effect drift offset. `LightEffectsBlinkParametersApply` loads the configured amplitude here; `DriftOffsetStepApply` samples a signed value in the range `[-amplitude, +amplitude]` and adds it to the light-effect X offset. The adjacent Y and Z amplitudes are at `0x004EADFC` and `0x004EAE00`.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).

# Cross-compile the reconstruction to Windows with MinGW-w64

*Status: superseded by [ADR 0004](0004-miniwin-native-port.md) for the primary
target decision. Kept because the MinGW cross toolchain remains the
**reference build** mechanism (winapi adapter) for A/B against the shipped
binary.*

The `LegoRR.exe` reconstruction was first planned as a Windows binary built via
Meson + MinGW-w64. This remains true for the *reference* build: a native Linux
port was originally rejected because it meant re-implementing
DDraw/DInput/DSOUND/WinMM/AVIFIL32 backends from scratch — strictly bigger than
"recompile the decompiled game." ADR 0004 reverses the sequencing once miniwin
provided exactly those backends as existing, maintained code.
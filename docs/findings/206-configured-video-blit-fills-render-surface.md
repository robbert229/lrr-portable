# Finding 206 — Configured Video Blits Fill the Render Surface

The DirectShow frame blit passes null destination and source rectangles to `IDirectDrawSurface3::Blt`. DirectDraw therefore uses the full destination and source surfaces. The game does not compute a letterbox or aspect-preserving rectangle in this call; if the sample surface and render surface have different dimensions, DirectDraw scales the source to fill the destination, which can change its aspect ratio.

**Evidence.** In shipped `reference/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), `FUN_004724a0` (`0x4724a0`) calls the destination surface's `Blt` slot with destination rectangle `NULL`, the sample's full surface, source rectangle `NULL`, flag `DDBLT_WAIT`, and effects pointer `NULL` (finding 200). The destination is the game's secondary render surface (finding 205). Microsoft's [DirectDraw Blt reference](https://learn.microsoft.com/en-us/previous-versions/ms785070%28v%3Dvs.85%29) documents that null rectangles use the full surfaces and that Blt stretches or shrinks when source and destination rectangles differ in size.

This describes the DirectShow copy into the render surface. It does not establish the source dimensions or final window scaling for every configured video.

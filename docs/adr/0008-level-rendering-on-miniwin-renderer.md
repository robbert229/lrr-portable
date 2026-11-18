# The level renders through miniwin's renderer abstraction, extended upstream-first with immediate-mode drawing

**Status:** proposed (2026-11-16). Refines step 5 of
[ADR 0007](0007-port-on-develop-units-on-miniwin.md)'s order of work, "Add
D3DRM3 and D3DIM".

## Context

With #865 and #866, the native port reaches the first level but draws no 3D.
After the mission brief, the screen still shows the brief's backdrop, with the
interface panels drawn over it.

### What the units call

Most of the level's 3D drawing does not go through D3DRM's own meshes. The
units have two render paths:

- **Retained mode.** `container.c` builds a frame tree with lights, fog
  settings and `IDirect3DRMMesh` visuals: `Container_MakeMesh2` types 0 and 1,
  `.x` files through `IDirect3DRMMeshBuilder3::Load`, and `AnimationSet2`.
- **Immediate mode.** `mesh_unit.c` (gods98's `Mesh.c`) attaches each mesh to
  a frame as a user visual (`CreateUserVisual`). D3DRM calls the visual's
  callback during `Viewport::Render`, and the callback draws the mesh's groups
  itself on the `IDirect3DDevice3`:
  - It reads the frame's world matrix with `GetTransform(WORLD)`.
  - It sets about a dozen render states: z enable and z write, alpha blend
    with `SRCBLEND`/`DESTBLEND` (additive 2/2, 5/2, 5/6), cull mode, shade
    mode, dither, specular, the texture filters and `TEXTUREHANDLE`.
  - It sets texture stage states, `SetTexture(0, IDirect3DTexture2)` and
    `SetLightState(MATERIAL, handle)`.
  - It calls `DrawIndexedPrimitive(TRIANGLELIST, D3DFVF_VERTEX, …)`. That is
    the only vertex type: position, normal and uv, with no vertex colour.
  - Alpha-blended groups go on a post-render list. `viewport_render` draws
    that list *after* `Render` returns, and before the 2D interface is blitted
    over the frame.

  The immediate-mode meshes include the terrain (`terrain_map.c` creates it with
  `Container_MakeMesh2` types 2 and 3), and every LightWave model and scene:
  `.lwo` and `.lws` files go through `mesh_load` and `lws.c`.

A run of the first level, made for this ADR, confirms this split. The build
merged #865 and #866, used the software renderer, ran under a private Xvfb,
and counted every miniwin entry point with a temporary counter. In about 53k
`Viewport::Render` calls the game made these calls:

| Calls | What |
| --- | --- |
| 10,118 | `CreateUserVisual` and `AddVisual` |
| 0 | `CreateMesh`, `CreateMeshBuilder`, `CreateAnimationSet` |
| 17,654 | frames created; per frame, about 3,000 `GetWorldTransform` calls from `GetPosition`/`GetTransform` |
| 8 | lights created |
| 2–3 | `SetRange`, `SetUmbra` and `SetPenumbra` (spot lights) |
| 52,371 | `Light::SetColor` |
| 5,371 | `Light::SetEnableFrame` |
| 2 | viewports: the level view and the radar's sub-rectangle |
| 53,088 | `IDirect3DViewport3::Clear2` with a rectangle and target-plus-z flags |
| 18.1M | `Blt` onto the back buffer, about 220 per frame |
| 1,644 | `Lock` of the back buffer after 3D |
| 2,540 | `FindPreferredTextureFormat` |

These calls are not implemented: `Clear2`, `Viewport::GetX`/`GetY`, `Light`
`SetColor`, `SetRange`, `SetUmbra`, `SetPenumbra` and `SetEnableFrame`,
`Material` `SetEmissive`/`SetSpecular`, and `FindPreferredTextureFormat`.

The 10,118 user visuals are what the level draws, and miniwin never calls
them: `Viewport::Render` collects only `IDirect3DRMMesh` visuals. Every
`IDirect3DDevice3` drawing method in the fork is a stub. Nothing clears the
level area either, which is why the brief's backdrop stays on screen.

### What miniwin has

The survey covers upstream isle-portable at `8ce9570` (master, 2026-11-15).
Our fork is that commit plus 14 DX6 commits.

- **One renderer abstraction, ten backends.** `Direct3DRMRenderer`
  (`internal/d3drmrenderer.h`) has these backends: software (RGBA), palette
  software, SDL3 GPU, OpenGL 1.x, OpenGL ES 2 and 3, DirectX 9, Citro3D, GXM
  and Glide.
  - The viewport walks the frame tree, collects lights and calls
    `SubmitDraw(meshId, matrices, Appearance)` for each cached mesh group.
  - The back buffer *is* the renderer's target. `Blt` onto it becomes
    `Draw2DImage`, `Lock` becomes `Download`, and `Unlock` blits the result
    back. 2D-over-3D compositing and locking the back buffer after 3D
    therefore already work, and the menus use both today.
- **Driven only the way LEGO Island needs.** LEGO Island renders only
  through Tgl's retained-mode `IDirect3DRMMesh`. Nothing upstream draws user
  visuals, execute buffers or `DrawPrimitive`. Upstream's D3DRM also has:
  - no fog;
  - no spot cone, light range or attenuation (a spot light is a point light
    with a direction);
  - no light enable frame;
  - no sub-rectangle viewports (`CreateViewport` ignores x and y);
  - no rectangle clears;
  - no vertex colour (miniwin's `D3DRMVERTEX` is 32 bytes);
  - no mesh builder, animation set or user-visual rendering.

  Transparency works, by group alpha, sorted back to front. Colour keys
  become alpha 0 when a texture is converted.
- **The software backend is pure CPU.** It has a float z-buffer and
  perspective-correct texturing, so it runs under Xvfb without a GPU. It is
  the only 3D backend the port builds today, with `MINIWIN_SOFTWARE_HAL`
  (a fork-only define) and an RGB565 display. Its `BeginFrame` clears the
  z-buffer on every `Render`.
- **Upstream receptiveness.** The owner has had more than ten PRs merged
  upstream, including miniwin ones (#838, #855, #866). Feature PRs merge
  readily when they serve LEGO Island or make D3DRM more correct (lighting
  models #823 and #920, backends #799 and #800). No DX6 interface version
  has been proposed upstream yet. `CONTRIBUTING.md` asks for small PRs and
  says nothing about other games.

## Decision

1. **Extend miniwin's renderer abstraction, upstream-first.** The level is
   drawn by miniwin's `Direct3DRMRenderer` and its backends. No new renderer
   is written beside it. The units keep both of their paths unchanged: they
   still call `CreateUserVisual` and `DrawIndexedPrimitive`.
2. **Immediate mode is implemented on the renderer.** The renderer already
   *is* the `IDirect3DDevice3` the units hold.
   - `Viewport::Render` calls each user visual's callback with `CANSEE` and
     then `RENDER`, in frame-walk order. Before each call it sets the device's
     world, view and projection transforms and its current viewport.
   - The device keeps a render-state block: the states listed under
     *Context*, the texture and texture stage states, and the material
     handle.
   - `DrawIndexedPrimitive` with `D3DFVF_VERTEX` triangle lists goes to one
     new renderer entry point, `SubmitImmediate`. It takes transient vertices
     and indices, the transforms and the state block, and caches nothing.
   - Drawing stays possible between `Render` and `Flip`, so the post-render
     list composites correctly. The z-buffer is cleared only by a clear that
     asks for it (`Clear2` with `D3DCLEAR_ZBUFFER`), not by every
     `BeginFrame`.
3. **Fill in what D3DRM itself is missing, in renderer data that every
   backend can read.**
   - Viewport sub-rectangles and rectangle clears.
   - Light range, attenuation, spot umbra and penumbra, and enable-frame
     scoping, added to `SceneLight`.
   - Fog: colour, mode, start, end, density, and the vertex or table method.
   - Material emissive and specular.
   - Texture decal transparency and `FindPreferredTextureFormat`.

   These are D3DRM 1/2 features (`IDirect3DRMLight::SetRange`,
   `IDirect3DRMFrame2::SetSceneFog*`, `IDirect3DRMViewport` with x and y), so
   they reach upstream without any DX6 interface.
4. **Each upstream patch is expressed in the interface versions upstream
   already has.** The core goes upstream as `IDirect3DDevice2` (DX5)
   `DrawIndexedPrimitive` with `D3DVT_VERTEX`, which has the same layout as
   `D3DFVF_VERTEX`, and as D3DRM v1 user visuals. The fork's DX6 declarations
   (`Device3`, `Viewport2`, `Frame3`) stay thin forwarders onto that core.
   Each new renderer virtual has a default that reports the feature as
   unsupported, so backends LegoRR does not use keep building unchanged.
   This follows ADR 0007's rule: the fork carries a patch only while its
   upstream PR is in review.
5. **Software first, then SDL3 GPU.** The software backend is the reference
   implementation. It is what CI tests and what the port ships with today.
   SDL3 GPU follows, for speed and higher resolutions. Other backends
   implement the new entry points only if someone wants them.

## Considered options

- **(b) A fork-only D3DRM3 layer**, written beside upstream's d3drm with its
  own renderer. Rejected. It would be a permanent fork of the largest part of
  miniwin, the owner does not carry changes upstream would not take, and it
  would duplicate the frame walk, lights, picking and the 2D compositing that
  already work.
- **(c) Implement the 3D directly on SDL3 GPU.** Rejected as the first
  target. CI and Xvfb have no GPU, so nothing could be tested there, and a
  single-backend feature bypasses the abstraction upstream maintains. SDL3
  GPU is instead the second backend under (a).
- **Lower `DrawIndexedPrimitive` onto the existing cached `SubmitDraw`**, by
  building a temporary `IDirect3DRMMesh` for each draw. This needs no
  interface change, so every backend would draw something at once. Rejected.
  `Appearance` cannot carry blend factors, z write or cull mode, so the work
  would have to be redone. It would also churn the mesh-ID cache with
  terrain that changes every frame.
- **Rewrite `mesh_unit.c` to build retained-mode meshes instead of user
  visuals.** Rejected. It breaks the units' original shape (ADR 0007), loses
  the post-render ordering and per-group render states, and the units' draw
  calls would no longer line up with the shipped binary's in the seam log.
- **A `// [library:render]` substitution in the units.** Rejected. Direct3D
  is inside miniwin's scope under ADR 0007, and 3D drawing is what miniwin
  is for.

## Consequences

- The renderer interface grows `SubmitImmediate`, a state block, a viewport
  rectangle, and richer lights and fog. Upstream review of the first PR
  decides whether this shape holds.
- **If upstream declines immediate mode or user visuals in principle**, this
  ADR is reopened before the SDL3 GPU work starts. The fork would then be
  carrying a patch with no upstream future, which ADR 0007 does not allow.
- Retained-mode features that the first level does not reach are deferred
  until a scenario reaches them:
  - vertex colour in `D3DRMVERTEX`, for `container.c`'s 36-byte vertices;
  - `MeshBuilder3::Load` of `.x` files;
  - `AnimationSet2`.
- Performance comes after correctness. Two costs showed up in the trace: the
  frame walks (`GetWorldTransform` about 3,000 times a frame), and a texture
  lookup on every one of the roughly 220 2D blits a frame.

## Order of work

Each step is one PR in this repo, bumping the submodule. Each miniwin commit
goes on `lrr/dx6-interfaces` and, at the same time, into an upstream PR to
isle-portable.

Verification uses four tools:

- **ctest**: seam golden tests on synthetic data. They render with the
  software backend into a small target and read pixels back through `Lock`.
- **Xvfb**: screenshots of the first level, from a scratch run directory.
- **seamrec**: `tools/seamrec` logs the shipped `LegoRR.exe` under Wine with
  Microsoft's `D3DRM.DLL`. Its log nests the game's calls inside each
  user-visual callback, so callback order, render states and
  `DrawIndexedPrimitive` arguments can be compared frame by frame.
- **The trace**: the per-entry-point counts above, rerun.

Steps 1, 2 and 3 touch different miniwin files and can run in parallel.
Step 0 can start at once. Steps 4 and 5 can run in parallel after step 1.

0. **Record a first-level scenario from the shipped binary** with seamrec,
   from boot to about 30 s into level 1. No port code is needed. The result
   is the oracle for steps 1–5: user-visual callbacks, the order and
   arguments of `DrawIndexedPrimitive`, render states, `Clear2` rectangles,
   and light and fog parameters.
1. **Immediate-mode meshes draw (software).** Decision items 2 and 4:
   - user-visual callbacks in `Viewport::Render`;
   - the device's transforms, render states, light state and texture stage
     states;
   - `DrawIndexedPrimitive` through `SubmitImmediate`;
   - drawing between `Render` and `Flip`, and z clears only on request.

   Untextured draws use the material colour. *Verify:* ctest draws, through a
   user visual, a triangle into a 64×64 target and checks the coverage, the
   z test against a second triangle, additive and `SRCALPHA` blending, and
   back-face culling. In the Xvfb screenshot, the terrain and the Rock
   Raiders appear as shaded geometry. The number of `RENDER` callbacks per
   frame is within a small margin of step 0's.
2. **Viewports and clears.**
   - `CreateViewport` x and y become a sub-rectangle: scissor, and a
     projection centred on the sub-rectangle.
   - `Viewport::GetX`/`GetY`.
   - `IDirect3DViewport3::Clear2` and `Viewport::Clear(flags)` with
     rectangles, the target, z, and the background colour.

   *Verify:* ctest uses two viewports on one target and checks the clear
   colours and that nothing draws outside a rectangle. In the Xvfb
   screenshot, the brief backdrop is gone and the radar draws in its own
   rectangle. The `Clear2` rectangles match step 0's.
3. **Textures.**
   - `FindPreferredTextureFormat`: 16-bit, and 8-bit paletted when asked.
   - A DirectDraw surface's `QueryInterface` to `IDirect3DTexture2`, and its
     handle.
   - Colour-keyed and paletted texture surfaces become alpha-0 texels.
   - `Texture::SetDecalTransparency` and `SetDecalTransparentColor`.

   *Verify:* ctest renders a colour-keyed 8-bit texture onto a quad and
   checks that the key colour leaves the background visible. In the Xvfb
   screenshot, the terrain is textured. After steps 1–3, the first level is
   visibly rendering: commit the screenshot.
4. **Lights.** `SceneLight` gains range, attenuation, umbra, penumbra and
   enable-frame scoping. `Light::SetColor`, `SetRange`, `SetUmbra`,
   `SetPenumbra` and `SetEnableFrame`. Material emissive and specular for
   both the D3DRM and the D3D material. *Verify:* ctest compares the lit
   colours of the vertices of a quad under a spot light (inside the umbra, in
   the penumbra, beyond the range) with the D3D lighting formulas. Light
   parameters match step 0's log. A screenshot is compared with the shipped
   binary's at the same scenario frame, with a tolerance.
5. **Fog.** `SetSceneFog*` reaches the renderer: vertex fog and table fog,
   linear and exponential. *Verify:* ctest checks fogged pixel values at
   known depths. A screenshot is compared with step 0's.
6. **SDL3 GPU backend.** Steps 1–5 on SDL3 GPU, built behind an option and
   selected when a GPU device exists. *Verify:* run locally on a GPU; CI only
   builds it. Screenshots are compared with the software backend's.
7. **The retained-mode remainder**, as scenarios reach it: `D3DRMVERTEX`
   colour, `MeshBuilder3::Load`, `AnimationSet2`. Then the frame-walk and
   2D-blit performance work.

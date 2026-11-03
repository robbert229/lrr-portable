# Win32 scene-height contract audit

Source inspection only, against the current50cc source. The runtime checkpoint
shows tall textured walls with only partial figures, but this audit does not
establish their exact runtime cause. No geometry/camera/flags/source was changed.
The inspected renderer is compiled into the32-bit candidate; this is not work on
the excluded native64 terrain bridge.

Two producers currently write different units to the same scene cache:

1. terrain_grid_vertex_build produces geometry height `-height_byte *
   vertical_scale` (terrain_grid.cpp:2758). TerrainCreate sets vertical_scale
   from its fourth argument, distinct from block spacing. The level-owned CPU
   vertex cache stores that value as Y (level.cpp:805).
2. Initial map setup copies those vertices but explicitly replaces Y with the
   raw physical-cell height byte before scene_render_terrain_cell_vertices_update
   (level.cpp:2638). That updater copies Y directly into s_scene_vertex_heights
   (scene_render.cpp:201), without conversion.
3. terrain_cell_mutation_progress_level copies the level CPU vertices, modifies
   their geometry-space Y, stores them back, and passes those world-space Y
   values directly to that same scene updater (level.cpp:6593–6674). Its fallback
   branch, in contrast, starts from raw bytes.
4. render_terrain always interprets the scene cache as raw height and converts
   it with `-cached_height * (block * 0.10)` (scene_render.cpp:771). Actor origin
   and initial camera focus use the raw physical height byte with the same
   visual scale (scene_render.cpp:700), so their floor convention differs from
   the mutated quad's cached world-space value.

Thus initial and mutation/cache paths have a source-verified unit/sign contract
mismatch. Whether the problematic mutation path ran for the occluding cells in
this exact Wine run remains unverified. A source defect is not a captured
occlusion cause; do not hide terrain, flatten heights, force camera parameters,
modify authored data, or weaken depth tests to make actors visible.

Needed decisive evidence: actual actor world/grid coordinate and rendered origin,
terrain raw byte, vertical_scale/block spacing, corresponding CPU quad Y and
scene cached heights/valid/orientation at the actor and nearby occluding cells,
plus camera/projection/depth. Resolve current exact PDB, never reuse50cc addresses
after another build. No injected calls/state writes or invisible clicks.

A bounded repair should establish one explicit scene-height convention and audit
all producers, consumers, mutation and interpolation paths. Preserve retail CPU
geometry and mapped function behavior; translate at the real presentation seam.
Meaningful PE32 verification should compare production height/vertex/render
owners before and after an actual loaded-cell update. No native64 test or
Python duplicate implementation can establish candidate runtime correctness.

Reachability refinement: MapLoadPipeline calls cell_init_all after loading its
voxel layers; cell_init_pass calls terrain_cell_mutation_progress when the loaded
UseRoof field is nonzero, and that helper additionally requires cell flags8/0x10.
This is a concrete production path, not proof of the actual fields/cells in the
historical run. Archived config uses UseRoofYES (recognized by block_get_bool);
current exact runtime roof/cell/cache values remain the decisive missing evidence.
The completed worker delivery is external
/var/tmp/wine-sceneheight-contract-20261102T234357Z/; root requested independent
source review before integration. No geometry patch is applied to the candidate.

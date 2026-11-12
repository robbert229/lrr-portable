# Issue #607 experiments: MSVC 5.0 `/Od` frame layout

Finding 587 (`docs/findings/587-camera-unit-original-shape.md`). The original
compiled the camera unit (GAME-022) without optimisation, and MSVC 5.0 then
places a block's locals in the frame by a hash of their names, not by their
declaration order:

- lowest address first, in descending order of a 16-way bucket of the name;
- locals in the same bucket keep their declaration order (earlier is lower);
- each nested block is laid out after its enclosing block, and sibling blocks
  do not share slots.

The bucket function is not identified (for one-character names it is
`ord(c) % 16`); `frame_buckets.py` measures it instead.

```sh
python3 tools/experiments/607/frame_buckets.py height camDist destPos   # bucket per name
python3 tools/experiments/607/frame_buckets.py --order pos,dir,dot       # frame order of one block
```

To match a function, take the original's slot order for each block and pick
names whose buckets do not increase along it (ties declared in slot order).
Both forms compile through `../486/msvc.py` (wine, or the `developer`
distrobox) with the C-unit flags plus `/Od`.

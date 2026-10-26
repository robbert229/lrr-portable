# LegoRR call graph height

This graph covers the 2,564 local functions in the checked-in Ghidra corpus.
The graph edges are resolved calls in the Ghidra pseudocode, with edges to
functions outside the executable omitted.

`functions-by-height.csv` sorts all functions from greatest to least height.
Height is the longest caller-to-callee path to a leaf after strongly connected
components are collapsed. A leaf has height 0; every function in a recursive
component receives the component's height. `scc_size` and
`recursive_component` identify recursive groups. `edges.csv` contains the
directed caller-to-callee edges used for the calculation.

The graph contains 6,783 resolved edges and reaches a maximum height of 41.
The highest-ranked functions are:

| Rank | Address | Height | Function |
| ---: | --- | ---: | --- |
| 1 | `0x004250c0` | 41 | `MainSimStep` |
| 2 | `0x004589c0` | 40 | `QuitPoll` |
| 3 | `0x0046f550` | 40 | `FUN_0046f550` |
| 4 | `0x00415220` | 39 | `FrontEndStartLevel` |
| 5 | `0x0041fb50` | 39 | `GameConfigInit` |
| 6 | `0x00435940` | 39 | `KeepRunning` |
| 7 | `0x0043f7d0` | 39 | `EntityEnergyLowTargetRouteCommandTick` |
| 8 | `0x0046f3b0` | 39 | `RayHitScanAndEffectApply` |
| 9 | `0x00471fc0` | 39 | `EntityVelocitySlotDispatch` |
| 10 | `0x0040de40` | 38 | `EntityFenceSparkTerrainInteraction` |

This is the static call graph visible in the decompiled corpus. Calls through
unresolved function pointers and other dynamic dispatch are not included.
Regenerate it with:

```sh
python3 tools/ghidra/call_graph_height.py
```

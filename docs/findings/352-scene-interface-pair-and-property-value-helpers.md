# Finding 352 — Scene Interface Pair and Property Value Helpers

This batch names an interface-pair update and a scene-property record value accessor. The exact interface and field semantics remain unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00476220` | `RenderInterfacePairUpdate` | Invokes methods on a pair of interface objects, releases an optional returned object, then applies a setter using the second interface; exact interface semantics are unresolved. |
| `0x00476310` | `ScenePropertyValueAt08Get` | Calls the object vtable slot at +0x1c and returns record offset +8, or zero when no record is returned. |

Direct pseudocode evidence: `legorr-advisor-animation.bundle` (`ev_0ec3059957c05673a84acc4ee06dafe179301f1e8d5023e3abf1cc599f9e7085`) and `legorr-ddscene.bundle` (`ev_b48bc1ac415a9c42443d39bd256e831a480875406242a6977c04b51b2667919f`) for the pair update; `legorr-ddscene.bundle` (`ev_9bf356e5959d1ae68070946cba715f45f79bf160b320dfde1b950cdcc7d5f8f7`) for the record accessor.

The complete checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Field and interface meanings remain unspecified wherever callers do not establish them.

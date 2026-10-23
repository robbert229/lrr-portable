# Finding 363 — Geometry, String Lookup, and Entity State Helpers

This pass names rectangle and distance predicates, case-insensitive table lookups, request-record flag operations, and structural entity-state helpers. Names preserve literal flags and table offsets where semantics remain unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0046b4d0` | `CaseInsensitiveNameTableLookup` | Scans a fixed pointer table with case-insensitive comparison, writes the one-based matching index, and returns whether it found a match. |
| `0x00463c00` | `RectContainsPointByConvertedCoords` | Converts the supplied point coordinates to integers and tests them against four float rectangle bounds. |
| `0x00463c90` | `RectContainsPointFromObjectBounds` | Converts the supplied point coordinates and tests them against the rectangle stored at object offset +0x14. |
| `0x0046c0b0` | `CopyFourFieldsAndConvertSecond` | Copies four dwords, then replaces destination field 1 with the result of a conversion helper. |
| `0x0043c200` | `EntityStatePairPredicate` | Evaluates two state pairs, conditionally adjusts a local float through a callback, and returns a boolean from the resulting sum. |
| `0x00442480` | `VectorDirectionAngleRadiansGet` | Gets a vector pair, computes its angle, and adjusts the result into the observed 0-to-2π range. |
| `0x00443670` | `TryRandomEntityActionFromCodeRange` | When its guard passes, tries up to five randomized codes in the observed range, dispatches the first accepted action, and marks a success flag. |
| `0x00443710` | `AdjacentPositionCandidateFind` | Calls the position callbacks and helpers, then copies two generated fields into the output when a candidate succeeds. |
| `0x004440a0` | `LinkedEntityFlagAndCommandUpdate` | When a linked object passes the type check, sets bit 0x40 on both records and queues command code 0x1c. |
| `0x004471b0` | `TryCreateLinkedEntityAtPosition` | When its flag gate passes, attempts creation near the supplied position and links the resulting objects on success. |
| `0x004502c0` | `CoordinatePairWithinBounds` | Returns whether both converted coordinate values are below their respective float bounds. |
| `0x0045a370` | `CaseInsensitiveStringTableLookup` | Scans a fixed string table case-insensitively and writes the zero-based matching index. |
| `0x0045a410` | `CaseInsensitiveIndexedStringLookup` | Scans the selected fixed-stride string table case-insensitively and writes the zero-based matching slot. |
| `0x0045b8a0` | `PointDistanceWithinRadius` | Computes Euclidean distance between two integer coordinate pairs and tests whether it is less than the supplied radius. |
| `0x0045ab30` | `RequestRecordSetFlag4ClearFlag8` | Sets bit 4 and clears bit 8 in the selected request record flags. |
| `0x0045ab60` | `RequestRecordSetFlag2ClearFlag8` | Sets bit 2 and clears bit 8 in the selected request record flags. |
| `0x0045ab90` | `RequestRecordFlag8IsSet` | Returns whether bit 8 is set in the selected request record flags. |
| `0x00451fd0` | `CaseInsensitiveNameTableLookupAtOffset8` | Scans a resource table by the string at entry offset +8 and returns its zero-based match index, or -1 when absent. |

Direct pseudocode evidence for `0x0043c200` is in `legorr-energy-low-target-routing.bundle` (`ev_5a52a3fd5529b9b47b20edd10e55c3dd92e757e7af32bc2da7f06275d2beb1fe`); caller evidence is `ev_4dc7a5cfdc8ad698961ecdb04c1339fef9cf7580d89a8a6e42e02f04f8101a9a`. Other entries are supported by checked-in pseudocode and caller context; no matching direct evidence records were found.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Field, flag, and callback semantics remain at the operation level where callers do not establish more.

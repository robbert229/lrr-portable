# Finding 268 — CRT Conversion, Scene Hierarchy, and Math Helpers

Four CRT routines, one hierarchy-registration routine, and one CRT math core
have names grounded in direct behavior and available callers. These names
describe mechanics rather than asserting exact CRT export identities.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x496190 | CrtEnvironmentBlockGet | Chooses a wide or ANSI Win32 environment-block API, converts wide data to ANSI when needed, frees the OS block, and returns a CRT-heap copy. The startup entry stores the result as the environment global. |
| 0x49abe0 | `wcstombs` | Exact VC5 `LIBC.LIB` `wcstombs.obj` entry; the MSVC build calls the runtime directly, which handles both the C locale and active code-page conversion paths. Portable builds retain the local compatibility implementation. The observed caller converts time-zone names. |
| 0x4957d0 | CxxCatchObjectBindOrCopy | Prepares a C++ catch parameter: adjusts for a base-class subobject, binds the source pointer or copies bytes using `CrtMemoryMove` or a supplied copy function, then continues to the catch handler. |
| 0x499b30 | CrtDaylightSavingTimeCheck | Recomputes transition day-of-year boundaries for a `struct tm`-like year and compares the date/time against cached start/end values. Reached from local-time-to-epoch conversion when DST is unspecified. |
| 0x4887c0 | SceneHierarchyInterfaceRegisterRecursive | Recursively walks child nodes, assigns table indices, writes generated sanitized names through an interface slot, and initializes an identity matrix for flag-4 nodes. Caller use in LWS loading and shared-object cloning supports a hierarchy role; exact vtable-slot semantics remain unknown. |
| 0x48e0ad | CrtAcosCore | Handles x87 edge/error cases and computes `fpatan(sqrt((1-x)*(1+x)), x)` for the interior range, producing acos values from 0 to π. A wrapper calls it from geometry/angle paths. |

**Evidence.** CRT observations are in
[legorr-crt-third-pass-2026-10-23.bundle](../evidence/legorr-crt-third-pass-2026-10-23.bundle):
`ev_c10b505360de86b17433c5e9f399f162379f880d82d0f712c826844c85a67a16`
(environment block),
`ev_c4aa309dee7b512acd88e2d5eaa9371dd34db0b592566f0ad7d1e16e84eee16d`
(wide conversion),
`ev_36334223f7669850e141b6b9f51c0aac28f193cb85b661447986065e96ccd126`
(catch object), and
`ev_2470181f69b36f65492ae219aa6cd3bf1abecf2f7e1302a0e63c56a44738bad3`
(DST check). Caller chains and supporting helpers are in the same bundle.

The scene hierarchy observations are in
[legorr-ui-render-third-pass-2026-10-23.bundle](../evidence/legorr-ui-render-third-pass-2026-10-23.bundle):
`ev_12fbf6ea79f5ce915959077f5361b36fe47e3dc7d284cdaafe40c58f83268708`,
with caller records `ev_8da9fc512857ccb8b71544dadf2d362b31b0aa7c756a226f99ba6628869eb8e0`
and `ev_628ecc454b36f428417fb81266e576713b4e2321367acf4ae8e3b31dc1c9f61a`.
The acos core is `ev_4619c3a7ee8d58e0f009d81ac39bb2cfb9c8cb70355c815cae3e0c16eab49d47`;
its wrapper evidence is `ev_767971267f63cf3ca68b647c6a0ed30ad4322b9e17c66984b32a5b5fe596fb6d`.
All observations use static analysis of the shipped `reference/LegoRR.exe`
(SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).

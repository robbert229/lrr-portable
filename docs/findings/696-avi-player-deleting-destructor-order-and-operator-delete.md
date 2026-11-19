# 696. avi_player.cpp: the scalar deleting destructor's place, and the global operator delete

Part of the identity work in PR #848. This follows finding 604, which rebuilt
G98-012 (`src/gods98/avi_player.cpp`) in the original's shape. A layout scan
of the rebuilt image found two differences from the original's function
list. The figures below come from scratch compiles of the unit, a scratch
relink of the matching build's objects, and `make reccmp-check`.

## The differences

| Original | Matching build, before |
| --- | --- |
| `0047e460` ``AviSurface::`scalar deleting destructor'`` (0x20) | `??1AviSurface` (0x40) |
| `0047e480` `AviSurface::~AviSurface` (0x40) | `??_G`/`??_EAviSurface` (0x20) |
| … | … |
| `0047e800` `AviMovie::AviMovie` (0x180) | `??0AviMovie` (0x180) |
| — | `??3AviSurface@@SAXPAX@Z` (0x10), **extra** |
| `0047e980` ``AviMovie::`scalar deleting destructor'`` | `??_G`/`??_EAviMovie` |
| … | … |
| `0047ec90` `avi_load` (0x60) | `_avi_load` (0x60) |
| — | `??3AviMovie@@SAXPAX@Z` (0x10), **extra** |
| `0047ecf0` `avi_update` | `_avi_update` |

The unit was 0x950 bytes, against the original's 0x930.

## 1. Where MSVC 5.0 puts a scalar deleting destructor

MSVC 5.0 emits a class's vtable when a function first stores the vtable
pointer, which a constructor or a destructor does. The scalar deleting
destructor that the vtable names, and any other compiler-generated function,
comes immediately after that function. `AviMovie` shows the rule: its
constructor (`0047e800`) is its first such function, and its `??_G` follows
the constructor. `??3AviSurface` comes between them, because the
constructor's EH funclet refers to it.

`AviSurface`'s first such function was `~AviSurface`, so its `??_G`
followed the destructor. In the original, `??_G` comes first. No
annotated function in the unit precedes it, so the function that stored the
pointer first left the image. It was an unreferenced function that
`/OPT:REF` removed.

Scratch compiles of the unit:

| Variant | COMDAT order |
| --- | --- |
| as before | `~AviSurface`, `??_G`, `AviSurface(5 args)` |
| destructor declared after `Width`/`Height` | unchanged |
| destructor `inline` | the constructor, then `??_G` with the destructor inlined (0x50); no `~AviSurface` |
| constructor defined first | `AviSurface(5 args)`, `??_G`, `~AviSurface` |
| an unused `AviSurface::AviSurface()` before the destructor | `AviSurface()`, `??_G`, `~AviSurface`, `AviSurface(5 args)` |

The last variant is the original's order once `/OPT:REF` removes the unused
constructor. avi_player.cpp now defines that constructor, marked
`PORT-ONLY`, as movie_unit.cpp keeps the original's removed position setter
(finding 667). The original's name for the removed function is not known.
Any constructor or destructor of the class defined there would do.

`~AviSurface` stays at 94.74%. The original stores the vtable pointer before
it loads `m_surf`. Our build loads `m_surf` first. The unused constructor
does not change that, and neither do `m_surf != NULL`, a local copy,
`this->` or an inline helper.

## 2. The class-scope operator delete

Finding 604 gave both classes inline class-scope `operator new` and
`operator delete` (`PORT-ONLY`), which forward to the annotated functions.
The scalar deleting destructors inline `operator delete` and call
`0045daf0` directly. Each new-expression's EH funclet (`.text$x`), which
frees the memory when the constructor throws, does not: MSVC 5.0 does not
inline into funclets. Those funclets are in `??0AviMovie` (`new AviSurface`)
and `avi_load` (`new AviMovie`), and each one calls an out-of-line
`??3AviSurface` or `??3AviMovie`.

In the original, the funclets call `0045daf0` directly. It has eight
callers:
- `0042f24c`, in lego_unit.c's `0042f0c0`, a C function;
- `00472701`, in Movie.cpp;
- `0047e470` and `0047e990`, the two scalar deleting destructors;
- four funclets at `0049dd97`–`0049dde4`.

`0045daf0` is therefore the global `operator delete` (`??3@YAXPAX@Z`) and
also a plain C function, `ui_render_record_free` in the port (radarmap.c).
The original's link folded no identical functions (finding 695), and C code
cannot name `??3@YAXPAX@Z`. In the matching build, the same function needs
both names.

LINK 5.0 has no `/ALTERNATENAME` (finding 560), but it resolves COFF weak
externals: its error text includes "cannot find resolution for weak
extern". `tools/coff_weak_external.py` writes a section-less object whose
only weak external, `??3@YAXPAX@Z`, defaults to `_ui_render_record_free`.
It uses `IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY`, so LINK never pulls LIBC's
`delete.obj`. The matching build links it as `weak_operator_delete.obj`. It
has no sections and adds no bytes. avi_player.cpp drops the class-scope
`operator delete` under `_MSC_VER`. The funclets and the scalar deleting
destructors then call the global one, and LINK resolves it to
`ui_render_record_free`. The native port and MinGW keep the class-scope
operator.

## The symbol counter

Either change alone moves `AviSurface::Init` (`0047e510`) out of the
original's phase of MSVC 5.0's symbol counter (finding 538 rule B). Its
bytes change. The unused constructor alone also changes
`AviMovie::FrameToSurface`'s bytes. The two removed inline operators and the
added constructor balance each other. With both changes, every function in
the unit compiles to the same bytes as before (relocations masked), apart
from the two removed `??3` COMDATs and the added constructor, which
`/OPT:REF` removes. The changes therefore land in one commit.

## Result

The unit's function COMDATs are now in the original's order and at its
sizes, and it is 0x930 bytes. The table below is the rebuilt map after
`make reccmp-check`. The unit starts at `0047e440` because the units before
it are 0x20 short, which this change does not address.

| Original | Function | Size | Rebuilt | Offset in unit |
| --- | --- | ---: | --- | ---: |
| `0047e460` | ``AviSurface::`scalar deleting destructor'`` | 0x20 | `0047e440` | 0x000 |
| `0047e480` | `AviSurface::~AviSurface` | 0x40 | `0047e460` | 0x020 |
| `0047e4c0` | `AviSurface::AviSurface` | 0x50 | `0047e4a0` | 0x060 |
| `0047e510` | `AviSurface::Init` | 0x200 | `0047e4f0` | 0x0b0 |
| `0047e710` | `AviSurface::InitDesc` | 0x30 | `0047e6f0` | 0x2b0 |
| `0047e740` | `AviSurface::Width` | 0x10 | `0047e720` | 0x2e0 |
| `0047e750` | `AviSurface::Height` | 0x10 | `0047e730` | 0x2f0 |
| `0047e760` | `AviSurface::Lock` | 0x50 | `0047e740` | 0x300 |
| `0047e7b0` | `AviSurface::Unlock` | 0x30 | `0047e790` | 0x350 |
| `0047e7e0` | `AviSurface::CountMaskBits` | 0x20 | `0047e7c0` | 0x380 |
| `0047e800` | `AviMovie::AviMovie` | 0x180 | `0047e7e0` | 0x3a0 |
| `0047e980` | ``AviMovie::`scalar deleting destructor'`` | 0x20 | `0047e960` | 0x520 |
| `0047e9a0` | `AviMovie::~AviMovie` | 0x40 | `0047e980` | 0x540 |
| `0047e9e0` | `AviMovie::FrameToSurface` | 0x140 | `0047e9c0` | 0x580 |
| `0047eb20` | `AviMovie::Update` | 0xc0 | `0047eb00` | 0x6c0 |
| `0047ebe0` | `AviMovie::GetLength` | 0x10 | `0047ebc0` | 0x780 |
| `0047ebf0` | `AviMovie::SetTime` | 0x70 | `0047ebd0` | 0x790 |
| `0047ec60` | `AviMovie::GetTime` | 0x20 | `0047ec40` | 0x800 |
| `0047ec80` | `avi_initialise` | 0x10 | `0047ec60` | 0x820 |
| `0047ec90` | `avi_load` | 0x60 | `0047ec70` | 0x830 |
| `0047ecf0` | `avi_update` | 0x10 | `0047ecd0` | 0x890 |
| `0047ed00` | `avi_blit_to_back_buffer` | 0x40 | `0047ece0` | 0x8a0 |
| `0047ed40` | `avi_set_time` | 0x10 | `0047ed20` | 0x8e0 |
| `0047ed50` | `avi_get_time` | 0x10 | `0047ed30` | 0x8f0 |
| `0047ed60` | `avi_get_length` | 0x10 | `0047ed40` | 0x900 |
| `0047ed70` | `avi_remove` | 0x10 | `0047ed50` | 0x910 |
| `0047ed80` | `avi_is_ok` | 0x10 | `0047ed60` | 0x920 |

Every function is at the original's offset in the unit, at the original's
size. The rebuilt `.text` is 32 bytes shorter (642,533 bytes against
642,565). No function's score changed (3,022 compared, before against
after); `~AviSurface` stays at 94.74%, `FrameToSurface` at 66.36% and
`Update` at 96.49%. The scalar deleting destructors and both funclets now
call `ui_render_record_free` directly.

## Not done here

`movie_unit.cpp`'s `G98CMovie` has the same class-scope `operator delete`,
and its `??3G98CMovie@@SAXPAX@Z` (0x10, at `00472640` in the rebuild) is
extra in the same way. The same `#if !defined(_MSC_VER)` would remove it,
but its symbol-counter effects have not been checked.

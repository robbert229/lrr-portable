// SPDX-License-Identifier: LGPL-3.0-only
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(LEGORRR_BACKEND_WINAPI)
#include <windows.h>
#endif

#include "crt.hpp"
#include "heap.hpp"
#include "string.hpp"

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define CRT_U64(value) value##ui64
#else
#define CRT_U64(value) value##ULL
#endif

/* CRT inverse-trigonometric entries. The original uses x87 register inputs
 * and computes the acos core with fpatan(sqrt((1-x)(1+x)), x); these portable
 * double interfaces preserve the mathematical result for the engine's
 * ordinary finite inputs. */
// FUNCTION: LEGORR 0x0048e090
double crt_asin(double value)
{
    return asin(value);
}

// FUNCTION: LEGORR 0x0048e0ad
double crt_acos_core(double value)
{
    return atan2(sqrt((1.0 - value) * (1.0 + value)), value);
}

/* The 96-bit extended-integer helpers operate on three little-endian DWORDs.
 * Keeping the carries explicit also works with the VC5 compiler used by the
 * original target. */
// FUNCTION: LEGORR 0x0049a050
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void extended_float96_bit_shift_left_one(uint32_t* value)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        push edi
        mov esi, dword ptr [eax]
        mov edi, dword ptr [eax + 4]
        mov ecx, esi
        add esi, esi
        mov dword ptr [eax], esi
        lea esi, [edi + edi]
        shr ecx, 31
        or esi, ecx
        mov ecx, dword ptr [eax + 8]
        mov edx, edi
        mov dword ptr [eax + 4], esi
        shr edx, 31
        shl ecx, 1
        or ecx, edx
        mov dword ptr [eax + 8], ecx
        pop edi
        pop esi
        ret
    }
}
#else
void extended_float96_bit_shift_left_one(uint32_t* value)
{
    uint32_t low;
    uint32_t middle;
    low = value[0];
    middle = value[1];
    value[0] = low << 1;
    value[1] = (middle << 1) | (low >> 31);
    value[2] = (value[2] << 1) | (middle >> 31);
}
#endif

// FUNCTION: LEGORR 0x0049a080
void extended_float96_bit_shift_right_one(uint32_t* value)
{
    uint32_t middle;
    middle = value[1];
    value[1] = (middle >> 1) | (value[2] << 31);
    value[2] >>= 1;
    value[0] = (value[0] >> 1) | (middle << 31);
}

/* FUN_00498000: the corpus's memmove with word loops and byte tails —
 * byte-equivalent to the host memmove (findings 260/394). */
// FUNCTION: LEGORR 0x00498000
void* crt_memory_move(void* dest, const void* src, size_t count)
{
    return memmove(dest, src, count);
}

/* FUN_00417430: __ftol under the truncate-toward-zero control word the
 * compiler switches to — a C++ cast (findings 381/393). Overflow values
 * are outside the corpus's caller set (deviation in crt.hpp). */
// FUNCTION: LEGORR 0x00417430
int float_to_long(float value)
{
    return (int)value;
}

/* FUN_0041f8e0: empty body; the corpus's callable no-op (finding 381). */
// FUNCTION: LEGORR 0x0041f8e0
void noop_callback(void)
{
}

/* FUN_0047d3f0: popcount via the clear-lowest-set-bit loop. */
// FUNCTION: LEGORR 0x0047d3f0
int bit_count32(uint32_t value)
{
    int n = 0;
    while (value != 0u) {
        value &= value - 1u;
        ++n;
    }
    return n;
}

/* FUN_0047d700: popcount via the per-position test loop. */
// FUNCTION: LEGORR 0x0047d700
int pop_count32(uint32_t value)
{
    int n = 0;
    for (unsigned i = 0; i < 32u; ++i) {
        if ((value & (1u << (i & 31u))) != 0u) {
            ++n;
        }
    }
    return n;
}

/* FUN_0047d720: index of the lowest set bit, 0xffffffff for zero. */
// FUNCTION: LEGORR 0x0047d720
uint32_t find_first_set_bit32(uint32_t value)
{
    for (uint32_t i = 0; i < 32u; ++i) {
        if ((value & (1u << (i & 31u))) != 0u) {
            return i;
        }
    }
    return 0xffffffffu;
}

/* FUN_0047e7e0: the corpus's second identical position-loop body. */
// FUNCTION: LEGORR 0x0047e7e0
int bit_count32_by_position(uint32_t value)
{
    return pop_count32(value);
}

/* FUN_00497ab0: zero over exactly 3 dwords. */
// FUNCTION: LEGORR 0x00497ab0
int uint96_is_zero(const uint32_t limbs[3])
{
    int i = 0;
    do {
        if (limbs[i] != 0u) {
            return 0;
        }
        ++i;
    } while (i < 3);
    return 1;
}

/* FUN_0048e870: the double's exponent field not all-ones. */
// FUNCTION: LEGORR 0x0048e870
int double_is_finite(double value)
{
    uint64_t bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return ((bits >> 52u) & 0x7ffu) != 0x7ffu;
}

/* FUN_0045b900: the corpus casts the int pair and the divisor to float
 * before both divides (the results land in a caller float pair). */
// FUNCTION: LEGORR 0x0045b900
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* out_x, float* out_y)
{
    *out_x = (float)x / (float)divisor;
    *out_y = (float)y / (float)divisor;
}

/* FUN_0045b930: the compiler's two-shot __ftol — each float truncates
 * toward zero into its int output (same deviation as float_to_long). */
// FUNCTION: LEGORR 0x0045b930
void float_pair_to_integer_pair(float x, float y, int* out_x, int* out_y)
{
    *out_x = (int)x;
    *out_y = (int)y;
}

/* FUN_00499fb0: the corpus's carry test — the sum below either operand
 * means the addition carried out of the 32-bit result. */
// FUNCTION: LEGORR 0x00499fb0
int crt_unsigned_add_with_carry(uint32_t a, uint32_t b, uint32_t* sum)
{
    uint32_t s;
    int carry = 0;
    s = b + a;
    if (s < a || s < b) {
        carry = 1;
    }
    *sum = s;
    return carry;
}

/* FUN_00493a80: the exponent/quiet-bit pattern classifier over the two
 * bit words, corpus masks verbatim (finding 399). The corpus loads both
 * parameters up front and tests the infinities first, then reads the
 * exponent+quiet field as a raw dword at esp+0xa — six bytes past the
 * low parameter's address, the high half of the hi word plus two bytes
 * past the last parameter, which the 0x7ff8 mask discards — and compares
 * it as a WORD. */
// FUNCTION: LEGORR 0x00493a80
int double_classify(uint32_t lo, uint32_t hi)
{
    if (hi == 0x7ff00000u && lo == 0u) {
        return 1;                                   /* +infinity */
    }
    if (hi == 0xfff00000u && lo == 0u) {
        return 2;                                   /* -infinity */
    }
    /* The corpus reads the exponent+quiet field as a raw dword six bytes
     * past &lo — esp+0xa, the high half of the hi word plus two bytes
     * past the parameter, which the 0x7ff8 mask discards — and compares
     * it as a WORD; mirror the read shape through the parameter bytes. */
    const uint16_t exp_quiet = (uint16_t)(
        *(const uint32_t*)((const char*)&lo + 6) & 0x7ff8u);
    if (exp_quiet == 0x7ff8u) {
        return 3;                                   /* quiet-NaN-shaped */
    }
    if (exp_quiet == 0x7ff0u && ((hi & 0x7ffffu) != 0u || lo != 0u)) {
        return 4;                                   /* signaling-NaN-shaped */
    }
    return 0;                                       /* finite */
}

/* FUN_00499710: the 16-bit strlen — code units counted up to the NUL. */
// FUNCTION: LEGORR 0x00499710
int wide_string_length(const uint16_t* text)
{
    const uint16_t* p = text;
    uint16_t ch = *p;
    for (;;) {
        ++p;
        if (ch == 0) {
            break;
        }
        ch = *p;
    }
    return (int)(p - text) - 1;
}

/* FUN_0049add0: bounded scan — the terminator index plus one when found
 * within `bound` (count including the NUL), the bound otherwise; a zero
 * bound returns the bound without reading. */
// FUNCTION: LEGORR 0x0049add0
int bounded_wide_string_length_including_null(const uint16_t* text,
                                              int bound)
{
    if (bound != 0) {
        const uint16_t* p = text;
        int left = bound;
        while (*p != 0) {
            ++p;
            --left;
            if (left == 0) {
                break;
            }
        }
        if (left != 0 && *p == 0) {
            return (int)(p - text) + 1;
        }
    }
    return bound;
}

/* DoubleExponentBitsGetOrSpecialValue (FUN_00492068, findings 386/399):
 * exponent field for ordinary values; the untouched high word when the
 * exponent is all ones (Inf/NaN) so the caller sees the full pattern.
 * The corpus callee takes a cdecl double: the low word at [esp+4] is
 * unused, the high word at [esp+8] is live — two uint32_t params so
 * MSVC5 reads the high word at [esp+8]. */
// FUNCTION: LEGORR 0x00492068
uint32_t double_exponent_bits_or_special(uint32_t low_word, uint32_t high_word)
{
    (void)low_word; /* the corpus reads only [esp+8] */
    if ((high_word & 0x7ff00000u) != 0x7ff00000u) {
        return high_word & 0x7ff00000u;
    }
    return high_word;
}

/* FormatArgumentReadDword (FUN_00491430, finding 395): read the dword
 * at the cursor, then advance it by four. */
// FUNCTION: LEGORR 0x00491430
uint32_t format_argument_read_dword(uint8_t** cursor)
{
    uint32_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor = *cursor + 4;
    return value;
}

/* FormatArgumentReadWord (FUN_00491470, finding 395): read the word, advance
 * the cursor by four bytes, and return the word in the low half with the
 * advanced cursor's high 16 bits in the high half, as in CONCAT22. */
// FUNCTION: LEGORR 0x00491470
uint32_t format_argument_read_word(uint8_t** cursor)
{
    uint16_t value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor = *cursor + 4;
    return value;
}

/* FormatArgumentReadDouble (FUN_00491450, finding 395): read eight
 * bytes, advance by eight. */
// FUNCTION: LEGORR 0x00491450
double format_argument_read_double(uint8_t** cursor)
{
    double value;
    memcpy(&value, *cursor, sizeof(value));
    *cursor = *cursor + 8;
    return value;
}

/* X87RoundToIntegral (FUN_00493150, finding 379): FRNDINT under the
 * default control word — nearest-even integral, still floating.
 * rint() matches it exactly (and keeps -0, per the x87 answer). */
// FUNCTION: LEGORR 0x00493150
double x87_round_to_integral(double value)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    /* MSVC 5.0 has no rint() declaration. Keep this leaf on the same x87
     * instruction as the original CRT implementation. */
    double result;
    __asm {
        fld qword ptr [value]
        frndint
        fstp qword ptr [result]
    }
    return result;
#else
    return rint(value);
#endif
}

/* CrtMathDispatchTableInitialize (FUN_0048d940): the six consecutive
 * callback slots contain the CRT fatal handler until startup replaces them
 * with the formatting and decimal-point routines below. Keep the image
 * addresses as DWORDs; they are not callable host pointers. */
// GLOBAL: LEGORR 0x004aff80
static volatile uint32_t s_math_dispatch_table[6] = {
    0x00498340u, 0x00498340u, 0x00498340u,
    0x00498340u, 0x00498340u, 0x00498340u
};

// FUNCTION: LEGORR 0x0048d940
void crt_math_dispatch_table_initialize(void)
{
    s_math_dispatch_table[1] = 0x00490310u;
    s_math_dispatch_table[0] = 0x00490750u;
    s_math_dispatch_table[2] = 0x004903a0u;
    s_math_dispatch_table[3] = 0x004902b0u;
    s_math_dispatch_table[4] = 0x00490380u;
    s_math_dispatch_table[5] = 0x00490750u;
}

/* CrtMathIntrinsicDispatch (FUN_0048e1aa) calls __cintrindisp2. That
 * compiler-runtime dispatcher consumes an x87 result plus EDX/EBP state
 * owned by its original call frame; a normal C ABI entry has no faithful
 * way to recover that state. Preserve the one explicit global side effect
 * identified in __cintrindisp2 while leaving the context-sensitive work
 * as a seam until its caller/runtime frame is ported. */
// GLOBAL: LEGORR 0x00549168
static volatile unsigned char s_math_intrinsic_dispatch_seen;

// FUNCTION: LEGORR 0x0048e1aa
void crt_math_intrinsic_dispatch(void)
{
    s_math_intrinsic_dispatch_seen = 1;
}

/* CrtRoundToIntegralWithMathErrorHandling (FUN_0048e280): for finite
 * values the runtime uses FRNDINT, returning that x87 result. The full CRT
 * routes an unmasked precision exception and special values through
 * 0x493170/0x4931c0; those handlers depend on the not-yet-ported matherr
 * and FPU-control-word paths. FRNDINT itself preserves the target rounding
 * mode and raises the hardware precision flag. */
// FUNCTION: LEGORR 0x0048e280
double crt_round_to_integral_with_math_error_handling(double value)
{
    return x87_round_to_integral(value);
}

/* CrtRoundToIntegralWithMathErrorHandlingVariant (FUN_0048e8e0) has the
 * same FRNDINT finite path as FUN_0048e280. Its distinct math exception
 * operation code is 0x0c (the first routine uses 0x0b); dispatching that
 * error path requires the unported 0x493170/0x4931c0 handler chain. */
// FUNCTION: LEGORR 0x0048e8e0
double crt_round_to_integral_with_math_error_handling_variant(double value)
{
    return x87_round_to_integral(value);
}

/* CrtDoubleAbsoluteValue (FUN_0048e680): clearing only the sign bit
 * reproduces the ordinary finite and infinity result without introducing
 * a host libm NaN policy. The CRT's quiet/signaling-NaN branches call its
 * exception handlers, which are not yet available in this port. */
// FUNCTION: LEGORR 0x0048e680
double crt_double_absolute_value(double value)
{
    uint32_t high_word;
    memcpy(&high_word, ((char*)&value) + 4, sizeof(high_word));
    high_word &= 0x7fffffffu;
    memcpy(((char*)&value) + 4, &high_word, sizeof(high_word));
    return value;
}

/* CrtRunCallbackArray (FUN_0048e5a0): invoke each nonnull callback in the
 * corpus's forward, half-open pointer range. */
// FUNCTION: LEGORR 0x0048e5a0
void crt_run_callback_array(CrtCallback* start, CrtCallback* end)
{
    while (start < end) {
        if (*start != NULL) {
            (*start)();
        }
        ++start;
    }
}

/* DoubleBuildWithExponent (FUN_00493a40, findings 383/399): the corpus
 * builds the floating value from two significand words and the
 * exponent-bias input; Ghidra renders the recipe as CONCAT chains,
 * which decompose exactly to: top word = ((high >> 16) & 0x800f) |
 * ((exponent + 0x3fe) << 4), and the 16-bit halves of `high` plus `low`
 * under it. The sign bit and the fraction nibble of `high` survive the
 * mask, the 16-bit ops truncate like the corpus's, and the assembled
 * bits are bitcast to the port's double (x87 deviation, as documented
 * across this band). What the built value MEANS stays with the caller
 * (the frexp band); this leaf is the faithful bit construction. */
// FUNCTION: LEGORR 0x00493a40
double double_build_with_exponent(uint32_t significand_low,
                                  uint32_t significand_high,
                                  int16_t exponent)
{
    union {
        double value;
        struct {
            uint32_t low;
            uint32_t high;
        } words;
    } result;
    const uint32_t exponent_bits =
        (uint32_t)((int32_t)exponent + 0x3fe) << 4;
    const uint32_t high_word =
        ((significand_high >> 16) & 0x800fu) | exponent_bits;
    result.words.low = significand_low;
    result.words.high = (significand_high & 0xffffu) | (high_word << 16);
    return result.value;
}


/* ------------------------------------------------------------------ *
 *  Constant and DAT-global band (see crt.hpp). */

/* The corpus's .rdata +0.0f (file bytes verified 00 00 00 00): the
 * leaf flds a DWORD data operand (the s_global_float_00556f00 fld
 * width), so the zero rides a file-scope float carrier — the
 * float-to-double widening is free on the x87 return stack. */
// GLOBAL: LEGORR 0x0049e084
static const float  s_zero_float     = 0.0f; /* DAT_0049e084 */

// FUNCTION: LEGORR 0x00408930
double constant_zero_float_get(void)
{
    return (double)s_zero_float; /* corpus: fld DWORD ds:0049e084 / ret */
}

// FUNCTION: LEGORR 0x0047c160
int32_t constant_negative_10000_get(void)
{
    return -10000;
}

// FUNCTION: LEGORR 0x00490230
uint32_t processor_feature_fallback_false(void)
{
    return 0u;
}

/* The original resolves this optional Kernel32 export at runtime because
 * older Windows releases may not provide it. The native miniwin backend
 * has no equivalent processor-feature query, so it follows the original
 * missing-export fallback. */
// FUNCTION: LEGORR 0x00490280
uint32_t crt_floating_point_feature_probe(void)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    typedef BOOL (WINAPI *ProcessorFeatureProbe)(DWORD);
    HMODULE module = GetModuleHandleA("KERNEL32");
    FARPROC address;
    ProcessorFeatureProbe probe;
    if (module == NULL) {
        return processor_feature_fallback_false();
    }
    address = GetProcAddress(module, "IsProcessorFeaturePresent");
    if (address == NULL) {
        return processor_feature_fallback_false();
    }
    probe = (ProcessorFeatureProbe)address;
    return probe(0) ? 1u : 0u;
#else
    return processor_feature_fallback_false();
#endif
}

// FUNCTION: LEGORR 0x00456210
uint32_t flh_magic_uu_noop(void)
{
    return 0u;
}

/* The two DAT globals, static-backed until the corpus's writers (all
 * still unported) land — message.cpp's s_system_state precedent. The
 * float width is the corpus's own: 0x4230b0 flds a DWORD. */
// GLOBAL: LEGORR 0x00556f00
static float    s_global_float_00556f00;
// GLOBAL: LEGORR 0x004ff9bc
static uint32_t s_global_dword_004ff9bc;

// FUNCTION: LEGORR 0x004230b0
float global_float_field_00556f00_get(void)
{
    return s_global_float_00556f00;
}

// FUNCTION: LEGORR 0x004567c0
uint32_t global_value_004ff9bc_store_and_return(const void* value)
{
    if (value == NULL) {
        return 0u; /* deviation: corpus dereferences blindly */
    }
    uint32_t v = *(const uint32_t*)value;
    s_global_dword_004ff9bc = v;
    return v;
}

/* --- unowned DAT-global counter/accumulator band (see crt.hpp) -----------
 * None of these globals has a ported owner or feeder, so each lands as a
 * zero-initialized module static (the s_system_state precedent). */
// GLOBAL: LEGORR 0x004ff990
static uint32_t s_counter_004ff990 = 0u;
static float    s_float_00554adc   = 0.0f;
static float    s_float_00554efc   = 0.0f;
// GLOBAL: LEGORR 0x00555324
static uint32_t s_counter_00555324 = 0u;
// GLOBAL: LEGORR 0x00555328
static uint32_t s_counter_00555328 = 0u;
// GLOBAL: LEGORR 0x0055532c
static uint32_t s_counter_0055532c = 0u;
// GLOBAL: LEGORR 0x00555330
static float    s_float_00555330   = 0.0f;
// GLOBAL: LEGORR 0x00555120
static float    s_float_00555120   = 0.0f;

// FUNCTION: LEGORR 0x00454880
void global_counter_004ff990_increment(void)
{
    s_counter_004ff990 = s_counter_004ff990 + 1u;
}

// FUNCTION: LEGORR 0x00454890
void global_counter_004ff990_reset(void)
{
    s_counter_004ff990 = 0u;
}

// FUNCTION: LEGORR 0x00462830
void global_float_at_00554adc_add_one(void)
{
    s_float_00554adc = s_float_00554adc + 1.0f; /* corpus: x - -1.0 */
}

// FUNCTION: LEGORR 0x00462850
void global_float_at_00554efc_add_one(void)
{
    s_float_00554efc = s_float_00554efc + 1.0f;
}

// FUNCTION: LEGORR 0x00462870
void global_counter_at_00555324_increment(void)
{
    s_counter_00555324 = s_counter_00555324 + 1u;
}

// FUNCTION: LEGORR 0x00462880
void global_counter_at_00555328_increment(void)
{
    s_counter_00555328 = s_counter_00555328 + 1u;
}

// FUNCTION: LEGORR 0x00462890
void global_counter_at_0055532c_increment(void)
{
    s_counter_0055532c = s_counter_0055532c + 1u;
}

// FUNCTION: LEGORR 0x004628a0
void global_float_accumulator_00555330_add(float value)
{
    s_float_00555330 = s_float_00555330 + value;
}

// FUNCTION: LEGORR 0x004628c0
void global_float_accumulate_00555120(float value)
{
    s_float_00555120 = s_float_00555120 + value;
}

uint32_t crt_counter_004ff990_peek(void) { return s_counter_004ff990; }
float    crt_float_00554adc_peek(void)   { return s_float_00554adc; }
float    crt_float_00554efc_peek(void)   { return s_float_00554efc; }
uint32_t crt_counter_00555324_peek(void) { return s_counter_00555324; }
uint32_t crt_counter_00555328_peek(void) { return s_counter_00555328; }
uint32_t crt_counter_0055532c_peek(void) { return s_counter_0055532c; }
float    crt_float_00555330_peek(void)   { return s_float_00555330; }
float    crt_float_00555120_peek(void)   { return s_float_00555120; }

/* --- DirectDraw display-mode records + filter tuples -------------------- */
typedef struct FilterTuple {
    int32_t w, h, bpp;
} FilterTuple;

#define FILTER_TUPLE_ENTRIES 16
#define DISPLAY_MODE_ENTRIES CRT_DISPLAY_MODE_ENTRIES
#define DISPLAY_MODE_KINDS 2
static CrtDisplayModeEntry
    s_display_modes[DISPLAY_MODE_KINDS][CRT_DISPLAY_MODE_ENTRIES];
static int s_display_mode_counts[DISPLAY_MODE_KINDS];
static int s_active_display_mode;
static CrtDisplayResolutionEntry
    s_display_resolutions[CRT_DISPLAY_RESOLUTION_ENTRIES];
static int s_display_resolution_count;
static int s_display_resolution_enumeration_succeeded;
static FilterTuple s_filter_tuples[FILTER_TUPLE_ENTRIES];
static int         s_filter_tuple_count;

/* Caller-side DirectDraw enumeration replacement. This record occupies
 * 0x110 bytes, with `name` at the same +0x0c offset as the native list. */
int display_mode_entry_append(int mode, int width, int height, int bpp,
                              const char* name)
{
    const int kind = mode != 0 ? 1 : 0;
    int* count = &s_display_mode_counts[kind];
    if (name == NULL || *count >= DISPLAY_MODE_ENTRIES) {
        return 0;
    }
    CrtDisplayModeEntry* entry = &s_display_modes[kind][*count];
    entry->width = width;
    entry->height = height;
    entry->bpp = bpp;
    size_t length = strlen(name);
    if (length >= sizeof(entry->name)) {
        length = sizeof(entry->name) - 1;
    }
    memcpy(entry->name, name, length);
    entry->name[length] = '\0';
    ++*count;
    return 1;
}

int display_mode_entry_count(void)
{
    return s_display_mode_counts[s_active_display_mode];
}

int display_mode_entry_get(int index, CrtDisplayModeEntry* out)
{
    if (out == NULL || index < 0 ||
        index >= s_display_mode_counts[s_active_display_mode]) {
        return 0;
    }
    *out = s_display_modes[s_active_display_mode][index];
    return 1;
}

/* DisplayModeNameIndexFind (FUN_0049d950) compares mode-name bytes exactly
 * and returns the first matching row's zero-based index. */
// FUNCTION: LEGORR 0x0049D950
int display_mode_name_index_find(const char* name, uint32_t* out_index)
{
    const int count = s_display_mode_counts[s_active_display_mode];
    if (name == NULL || count <= 0) {
        return 0;
    }
    for (int index = 0; index < count; ++index) {
        if (strcmp(name, s_display_modes[s_active_display_mode][index].name) == 0) {
            if (out_index != NULL) {
                *out_index = (uint32_t)index;
            }
            return 1;
        }
    }
    return 0;
}

/* DisplayModeListPopulate (FUN_0049d630): form the exact supported-mode
 * index list for a dialog. The native combo box is represented by an
 * ordered array; the first match is selected, and an empty list disables
 * the dialog's default action. */
// FUNCTION: LEGORR 0x0049D630
void display_mode_list_populate(int mode, int desktop_bpp,
                                CrtDisplayModeList* out)
{
    if (out == NULL) return;
    s_active_display_mode = mode != 0 ? 1 : 0;
    memset(out, 0, sizeof(*out));
    out->desktop_bpp = desktop_bpp;
    out->selected_mode_index = 0xffffffffu;
    out->empty_message_kind = mode == 0 ? 1 : 2;
    const int mode_count = s_display_mode_counts[s_active_display_mode];
    for (int index = 0; index < mode_count; ++index) {
        if (!display_mode_matches_filter_tuple(index)) continue;
        if (out->count >= CRT_DISPLAY_MODE_ENTRIES) break;
        out->mode_indices[out->count++] = (uint32_t)index;
    }
    out->default_action_enabled = out->count != 0;
    out->selected_index = 0;
    if (out->count != 0) {
        out->selected_mode_index = out->mode_indices[0];
        out->empty_message_kind = 0;
    }
}

/* DisplayModeDialogSetModeAndRefreshChoices (FUN_0049d5e0): store the
 * windowed/fullscreen selector, repopulate its list, and select the
 * matching localized label resource. */
// FUNCTION: LEGORR 0x0049D5E0
void display_mode_dialog_set_mode_and_refresh_choices(
    CrtDisplayModeDialog* dialog, int mode, int desktop_bpp)
{
    if (dialog == NULL) return;
    dialog->mode = mode;
    dialog->mode_label_resource = mode != 0 ? 0x004b2d44u : 0x004b2d38u;
    display_mode_list_populate(mode, desktop_bpp, &dialog->modes);
}

/* DisplayRefreshRateControlsSync (FUN_0049d9d0): adapter flag 0x20 controls
 * whether the refresh-rate selector is available. When unavailable, force
 * the dialog to its non-refresh mode and remember the previous mode for
 * restoration if a later adapter supports refresh selection. */
// FUNCTION: LEGORR 0x0049D9D0
void display_refresh_rate_controls_sync(CrtDisplayModeDialog* dialog,
                                        uint32_t adapter_flags,
                                        int desktop_bpp)
{
    if (dialog == NULL) return;
    if ((adapter_flags & 0x20u) == 0u) {
        dialog->previous_mode = dialog->mode;
        display_mode_dialog_set_mode_and_refresh_choices(dialog, 1,
                                                         desktop_bpp);
        dialog->refresh_rate_control_enabled = 0;
        dialog->checkbox_3ed_checked = 0;
        dialog->checkbox_3ec_checked = 1;
        return;
    }

    dialog->refresh_rate_control_enabled = 1;
    if (dialog->previous_mode == 0) {
        display_mode_dialog_set_mode_and_refresh_choices(dialog, 0,
                                                         desktop_bpp);
        dialog->checkbox_3ec_checked = 0;
        dialog->checkbox_3ed_checked = 1;
    }
}

/* Platform enumeration handoff for DisplayResolutionComboPopulate. Reset
 * starts a new DirectDraw enumeration result; records retain the names and
 * 0x2000 preferred-mode bit that the native combo builder consumes. */
void display_resolution_entries_reset(int enumeration_succeeded)
{
    s_display_resolution_count = 0;
    s_display_resolution_enumeration_succeeded =
        enumeration_succeeded != 0;
}

int display_resolution_entry_append(const char* name, uint32_t flags)
{
    if (!s_display_resolution_enumeration_succeeded || name == NULL ||
        s_display_resolution_count >= CRT_DISPLAY_RESOLUTION_ENTRIES) {
        return 0;
    }
    CrtDisplayResolutionEntry* entry =
        &s_display_resolutions[s_display_resolution_count];
    size_t length = strlen(name);
    if (length >= sizeof(entry->name)) {
        length = sizeof(entry->name) - 1;
    }
    memcpy(entry->name, name, length);
    entry->name[length] = '\0';
    entry->flags = flags;
    ++s_display_resolution_count;
    return 1;
}

int display_resolution_entry_count(void)
{
    return s_display_resolution_count;
}

int display_resolution_entry_get(int index,
                                 CrtDisplayResolutionEntry* out)
{
    if (out == NULL || index < 0 || index >= s_display_resolution_count) {
        return 0;
    }
    *out = s_display_resolutions[index];
    return 1;
}

/* DisplayResolutionComboPopulate (FUN_0049d780): include every enumerated
 * display resolution, selecting the last row marked with 0x2000. Failure
 * contributes the DirectX error resource as one placeholder row. */
// FUNCTION: LEGORR 0x0049D780
void display_resolution_combo_populate(CrtDisplayResolutionList* out)
{
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    if (!s_display_resolution_enumeration_succeeded) {
        out->count = 1;
        out->error_resource = 0x004b2d90u;
        return;
    }
    out->count = (uint32_t)s_display_resolution_count;
    for (int index = 0; index < s_display_resolution_count; ++index) {
        if ((s_display_resolutions[index].flags & 0x2000u) != 0u) {
            out->selected_index = (uint32_t)index;
        }
    }
}

// FUNCTION: LEGORR 0x0049d870
void display_resolution_filter_tuple_append(int width, int height, int bpp)
{
    if (s_filter_tuple_count < FILTER_TUPLE_ENTRIES) {
        FilterTuple* t = &s_filter_tuples[s_filter_tuple_count];
        t->w = width;
        t->h = height;
        t->bpp = bpp;
    }
    s_filter_tuple_count = s_filter_tuple_count + 1;
}

int display_resolution_filter_tuple_count(void)
{
    return s_filter_tuple_count;
}

void display_resolution_filter_tuple_get(int index, int* w, int* h, int* bpp)
{
    const int n = (index >= 0 && index < s_filter_tuple_count
                   && index < FILTER_TUPLE_ENTRIES)
        ? index : -1;
    if (w != NULL) {
        *w = (n >= 0) ? s_filter_tuples[n].w : 0;
    }
    if (h != NULL) {
        *h = (n >= 0) ? s_filter_tuples[n].h : 0;
    }
    if (bpp != NULL) {
        *bpp = (n >= 0) ? s_filter_tuples[n].bpp : 0;
    }
}

/* DisplayModeMatchesFilterTuple (FUN_0049d8d0): any registered tuple may
 * match; zero tuple fields are wildcards, as in the DirectDraw path. */
// FUNCTION: LEGORR 0x0049D8D0
int display_mode_matches_filter_tuple(int mode_index)
{
    if (s_filter_tuple_count <= 0) {
        return 1;
    }
    if (mode_index < 0 ||
        mode_index >= s_display_mode_counts[s_active_display_mode]) {
        return 0;
    }
    const CrtDisplayModeEntry* mode =
        &s_display_modes[s_active_display_mode][mode_index];
    int count = s_filter_tuple_count;
    if (count > FILTER_TUPLE_ENTRIES) {
        count = FILTER_TUPLE_ENTRIES;
    }
    for (int i = 0; i < count; ++i) {
        const FilterTuple* tuple = &s_filter_tuples[i];
        if ((tuple->w == 0 || tuple->w == mode->width) &&
            (tuple->h == 0 || tuple->h == mode->height) &&
            (tuple->bpp == 0 || tuple->bpp == mode->bpp)) {
            return 1;
        }
    }
    return 0;
}

/* --- the remaining unowned DAT one-liners (see crt.hpp) ---------------------
 * Zero-initialized module statics per the s_system_state precedent — the
 * globals' meanings are unresolved (finding 383) and none has a ported
 * feeder. */
// GLOBAL: LEGORR 0x004ff978
static uint32_t s_flag_004ff978 = 0u;
// GLOBAL: LEGORR 0x004ffe48
static uint32_t s_value_004ffe48 = 0u;

// FUNCTION: LEGORR 0x00456640
uint32_t global_flag_004ff978_get(void)
{
    return s_flag_004ff978;
}

// FUNCTION: LEGORR 0x00458660
void global_value_004ffe48_set(uint32_t value)
{
    s_value_004ffe48 = value;
}

/* --- the unowned DAT word-store band (see crt.hpp) -------------------------- */
// GLOBAL: LEGORR 0x004b31bc
static uint32_t s_advisor_state_word = 0u; /* DAT_004b31bc */
// GLOBAL: LEGORR 0x00557860
static uint32_t s_task_refresh_flag = 0u;  /* DAT_00557860 */
// GLOBAL: LEGORR 0x004b8a54
static uint32_t s_level_stored_word = 0u;  /* DAT_004b8a54 */
// GLOBAL: LEGORR 0x004c7178
static uint32_t s_mode_zero_flag = 0u;     /* DAT_004c7178 */
// GLOBAL: LEGORR 0x004c79a4
static uint32_t s_rockfall_base = 0u;      /* DAT_004c79a4 */
// GLOBAL: LEGORR 0x004c7ea8
static uint32_t s_ui_flag_word = 0u;       /* DAT_004c7ea8 */
static uint32_t s_config_word = 0u;        /* DAT_004a1ee4 */

// FUNCTION: LEGORR 0x00401c20
uint32_t global_flag_bit0_is_set(void)
{
    return s_advisor_state_word & 1u;
}

// FUNCTION: LEGORR 0x00407340
void state_flag_toggle_when_zero(void)
{
    s_task_refresh_flag = (s_task_refresh_flag == 0u) ? 1u : 0u;
}

// FUNCTION: LEGORR 0x00407360
uint32_t global_task_refresh_flag_get(void)
{
    return s_task_refresh_flag;
}

// FUNCTION: LEGORR 0x00409d40
void level_state_stored_value_set(uint32_t v)
{
    s_level_stored_word = v;
}

// FUNCTION: LEGORR 0x0040ab10
void global_mode_zero_input_set(int value)
{
    s_mode_zero_flag = (value == 0) ? 1u : 0u;
}

uint32_t global_mode_zero_input_peek(void)
{
    return s_mode_zero_flag;
}

// FUNCTION: LEGORR 0x0040bfc0
void rockfall_style_table_base_set(uint32_t v)
{
    s_rockfall_base = v;
}

// FUNCTION: LEGORR 0x0040e7e0
void global_flag_bit0_clear(void)
{
    /* corpus: mov eax,ds:004c7ea8 / and al,0FEh / mov ds:004c7ea8,eax.
     * The plain load-op-store spelling (not the &= compound) keeps the
     * /Od register round-trip instead of folding to and mem,imm; the
     * corpus's byte-sized and is MSVC's own narrowing of the mask. */
    s_ui_flag_word = s_ui_flag_word & ~1u;
}

// FUNCTION: LEGORR 0x0040f5e0
void global_config_value_set(uint32_t v)
{
    s_config_word = v;
}

uint32_t advisor_state_word_peek(void)  { return s_advisor_state_word; }
void     advisor_state_word_set(uint32_t v) { s_advisor_state_word = v; }
uint32_t level_stored_word_peek(void)   { return s_level_stored_word; }
uint32_t mode_zero_input_flag_peek(void){ return s_mode_zero_flag; }
uint32_t rockfall_table_base_peek(void) { return s_rockfall_base; }
uint32_t ui_flag_word_peek(void)        { return s_ui_flag_word; }
uint32_t global_config_word_peek(void)  { return s_config_word; }

/* --- display dimensions + request-record predicates (see crt.hpp) ---------- */
// GLOBAL: LEGORR 0x00505924
static int s_display_w;   /* DAT_00505924 */
// GLOBAL: LEGORR 0x00505928
static int s_display_h;   /* DAT_00505928 */

// FUNCTION: LEGORR 0x00401c30
int display_width_get(void)  { return s_display_w; }
// FUNCTION: LEGORR 0x00401c40
int display_height_get(void) { return s_display_h; }

void display_dimensions_set(int w, int h)
{
    s_display_w = w;
    s_display_h = h;
}

/* DAT_0050013c record block: 18 records of 0x30 bytes (the 0x50013c…
 * 0x50037c span the panel reset walks), flag dword at record+0x2c — one
 * array at the corpus's DAT_00500110 base, stride 0x30 (the lea
 * [eax+eax*2]/shl 4 = 3*16 walks, no bounds tests), all-zero image bytes
 * filled at runtime. Only the flag words are modeled — the record
 * payloads belong to the unported panel layer. */
#define REQUEST_RECORD_COUNT 18

typedef struct RequestRecord {
    uint8_t  pad[0x2c];
    uint32_t flags;   /* +0x2c */
} RequestRecord;

// GLOBAL: LEGORR 0x00500110
static RequestRecord s_request_records[REQUEST_RECORD_COUNT];

// FUNCTION: LEGORR 0x0045ab30
int request_record_ready_flag4(int index)
{
    if ((unsigned)index >= REQUEST_RECORD_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    const uint32_t f = s_request_records[index].flags;
    return (f & 4u) != 0u && (f & 8u) == 0u ? 1 : 0;
}

// FUNCTION: LEGORR 0x0045ab60
int request_record_ready_flag2(int index)
{
    const uint32_t f = s_request_records[index].flags;
    return (f & 2u) != 0u && (f & 8u) == 0u ? 1 : 0;
}

// FUNCTION: LEGORR 0x0045ab90
int request_record_flag8_is_set(int index)
{
    if ((unsigned)index >= REQUEST_RECORD_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    return (int)(s_request_records[index].flags & 8u);
}

void request_record_flags_set(int index, uint32_t flags)
{
    if ((unsigned)index < REQUEST_RECORD_COUNT) {
        s_request_records[index].flags = flags;
    }
}

uint32_t request_record_flags_peek(int index)
{
    if ((unsigned)index >= REQUEST_RECORD_COUNT) {
        return 0u;
    }
    return s_request_records[index].flags;
}

/* --- the remaining DAT one-liner stores/reads (see crt.hpp) ----------------- */
// GLOBAL: LEGORR 0x0050080c
static uint32_t s_value_0050080c = 0u;
static uint32_t s_font_words[16];   /* DAT_00500a9c window */
static uint32_t s_cursor_slot = 0u; /* DAT_00500a98 */

// FUNCTION: LEGORR 0x0045c4a0
void global_value_0050080c_set(uint32_t value)
{
    s_value_0050080c = value;
}

uint32_t global_value_0050080c_peek(void)
{
    return s_value_0050080c;
}

// FUNCTION: LEGORR 0x0045cc10
uint32_t indexed_global_value_at_get(int index)
{
    if (index < 0 || index >= 16) {
        return 0u; /* deviation: the corpus strides blindly */
    }
    return s_font_words[index];
}

// FUNCTION: LEGORR 0x0045cc60
uint32_t global_cursor_word_get(void)
{
    return s_cursor_slot;
}

void font_cursor_word_set(int index, uint32_t v)
{
    if (index == -1) {
        s_cursor_slot = v; /* the DAT_00500a98 word itself */
        return;
    }
    if (index >= 0 && index < 16) {
        s_font_words[index] = v;
    }
}

/* --- pure-return stubs + advisor word clear (see crt.hpp) -------------------- */
// FUNCTION: LEGORR 0x00491ff5
void crt_no_op_stub(void)
{
    /* FUN_00491ff5: pure return — faithful. */
}

/* CrtFpuPrecisionModeCodeGet (FUN_0049200c): this helper consumes the
 * caller's EAX value directly and returns 7 when bit 0x80000 is set, else
 * 1. Preserve that register contract on the original MSVC5 x86 target. */
#if defined(_MSC_VER) && defined(_M_IX86)
#define CRT_FPU_PRECISION_NAKED __declspec(naked)
#else
#define CRT_FPU_PRECISION_NAKED
#endif
// FUNCTION: LEGORR 0x0049200c
CRT_FPU_PRECISION_NAKED uint32_t crt_fpu_precision_mode_code_get(void)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        test eax, 80000h
        jz precision_default
        mov eax, 7
        ret
    precision_default:
        mov eax, 1
        ret
    }
#else
    /* Portable callers do not have the original implicit EAX input. */
    return 1u;
#endif
}
#undef CRT_FPU_PRECISION_NAKED

// FUNCTION: LEGORR 0x00497870
void crt_no_op_stub_variant(void)
{
    /* FUN_00497870: pure return — faithful. */
}

// FUNCTION: LEGORR 0x00401b20
void advisor_clear_active_state(void)
{
    s_advisor_state_word &= ~2u;
}

// FUNCTION: LEGORR 0x0048e030
void byte_range_swap(void* a, void* b, size_t n)
{
    if (a == NULL || b == NULL) {
        return; /* deviation: the corpus reads blindly */
    }
    if (a == b) {
        return; /* the corpus's same-range shortcut */
    }
    unsigned char* pa = (unsigned char*)a;
    unsigned char* pb = (unsigned char*)b;
    for (size_t i = 0; i < n; ++i) {
        const unsigned char t = pa[i];
        pa[i] = pb[i];
        pb[i] = t;
    }
}

/* --- the DAT window band (see crt.hpp) ---------------------------------------
 * All tables are 18-entry zero-initialized statics (the corpus's .data
 * tables have no ported owner; the corpus strides/blind-derefs — guarded
 * here, the unowned-global deviation). */
#define DAT_WINDOW_COUNT 18

static uint32_t s_pair_a[DAT_WINDOW_COUNT]; /* DAT_00500128 entries */
static uint32_t s_pair_b[DAT_WINDOW_COUNT]; /* DAT_0050012c entries */
static float    s_fpair_a[DAT_WINDOW_COUNT]; /* DAT_00552144 entries */
static float    s_fpair_b[DAT_WINDOW_COUNT]; /* DAT_00552148 entries */
static void*    s_metric_records[DAT_WINDOW_COUNT]; /* DAT_00552154 */
static uint32_t s_region_xy[2];  /* DAT_005013a8/3ac */
static float    s_region_wh[2];  /* DAT_005013b0/b4 */
static void*    s_panel_records[DAT_WINDOW_COUNT]; /* DAT_00500134 */


// FUNCTION: LEGORR 0x0045a6e0
void indexed_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides blindly */
    }
    s_pair_a[index] = x;
    s_pair_b[index] = y;
}

// FUNCTION: LEGORR 0x0045a700
void indexed_pair_get(int index, uint32_t* out_x, uint32_t* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT
        || out_x == NULL || out_y == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    *out_x = s_pair_a[index];
    *out_y = s_pair_b[index];
}

// FUNCTION: LEGORR 0x00463730
void indexed_float_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides blindly */
    }
    s_fpair_a[index] = (float)x; /* the corpus's uint->float cast */
    s_fpair_b[index] = (float)y;
}

void indexed_float_pair_peek(int index, float* out_x, float* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT || out_x == NULL
        || out_y == NULL) {
        return;
    }
    *out_x = s_fpair_a[index];
    *out_y = s_fpair_b[index];
}

// FUNCTION: LEGORR 0x00463ed0
void indexed_text_metrics_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return;
    }
    char* rec = (char*)s_metric_records[index];
    if (rec == NULL) {
        return; /* deviation: the corpus dereferences its slot blindly */
    }
    __builtin_memcpy(rec + 0x1c, &x, sizeof(x));
    __builtin_memcpy(rec + 0x20, &y, sizeof(y));
    const uint32_t one = 1u; /* the corpus's dirty flag */
    __builtin_memcpy(rec + 0x24, &one, sizeof(one));
}

// FUNCTION: LEGORR 0x00463ea0
void indexed_text_metrics_pair_get(int index, uint32_t* out_x,
                                   uint32_t* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT
        || out_x == NULL || out_y == NULL) {
        return;
    }
    const char* rec = (const char*)s_metric_records[index];
    if (rec == NULL) {
        return;
    }
    __builtin_memcpy(out_x, rec + 0x1c, sizeof(*out_x));
    __builtin_memcpy(out_y, rec + 0x20, sizeof(*out_y));
}

void indexed_record_pointer_set(int index, void* record)
{
    if (index >= 0 && index < DAT_WINDOW_COUNT) {
        s_metric_records[index] = record;
    }
}

void* indexed_record_pointer_peek(int index)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    return s_metric_records[index];
}

// FUNCTION: LEGORR 0x0045d370
void region_bounds_set(const uint32_t* xy_pair, uint32_t w, uint32_t h)
{
    if (xy_pair == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    s_region_xy[0] = xy_pair[0];
    s_region_xy[1] = xy_pair[1];
    s_region_wh[0] = (float)w; /* the corpus's uint->float casts */
    s_region_wh[1] = (float)h;
}

void region_bounds_peek(uint32_t* out_xy, float* out_wh)
{
    if (out_xy != NULL) {
        out_xy[0] = s_region_xy[0];
        out_xy[1] = s_region_xy[1];
    }
    if (out_wh != NULL) {
        out_wh[0] = s_region_wh[0];
        out_wh[1] = s_region_wh[1];
    }
}

void panel_draw_records_set(int index, void* records)
{
    if (index >= 0 && index < DAT_WINDOW_COUNT) {
        s_panel_records[index] = records;
    }
}

void* panel_draw_records_peek(int index)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    return s_panel_records[index];
}

static uint32_t* panel_item_word(int panel, int item)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT) {
        return NULL;
    }
    char* block = (char*)s_panel_records[panel];
    if (block == NULL || item < 0) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return (uint32_t*)(block + 0x28 + (size_t)item * 0x2c);
}

// FUNCTION: LEGORR 0x0045aac0
void panel_item_flag16_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x10u : *w & ~0x10u;
}

// FUNCTION: LEGORR 0x0045abb0
void panel_item_flag32_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x20u : *w & ~0x20u;
}

// FUNCTION: LEGORR 0x0045ab00
void panel_state_flags_toggle(int index)
{
    if ((unsigned)index >= DAT_WINDOW_COUNT) {
        return;
    }
    const uint32_t f = s_request_records[index].flags;
    if ((f & 2u) != 0u) {
        s_request_records[index].flags = f & ~2u | 0xcu;
    } else {
        s_request_records[index].flags = f & ~4u | 0xau;
    }
}

static float    s_reward_score;  /* DAT_00555738 */
static float    s_reward_count;  /* DAT_0055573c */

// FUNCTION: LEGORR 0x00462800
void reward_score_add(float value)
{
    s_reward_count = s_reward_count - -1.0f; /* corpus shape */
    s_reward_score = s_reward_score + value;
}

float reward_score_total_peek(void) { return s_reward_score; }
uint32_t reward_score_count_peek(void)
{
    return (uint32_t)s_reward_count;
}

/* --- the panel/view blit band (see crt.hpp) --------------------------------- */
static uint32_t s_view_handle[DAT_WINDOW_COUNT]; /* DAT_00552140 */
static void*    s_view_rect_a[DAT_WINDOW_COUNT]; /* DAT_0055214c */
static void*    s_view_rect_b[DAT_WINDOW_COUNT]; /* DAT_00552150 */
static int      s_view_region[DAT_WINDOW_COUNT]; /* DAT_00552158 */
static int      s_panel_count[DAT_WINDOW_COUNT]; /* DAT_00500138 */

#define CRT_SURFACE_CALLS 256
static uint32_t s_call_handle[CRT_SURFACE_CALLS];
static float    s_call_x[CRT_SURFACE_CALLS];
static float    s_call_y[CRT_SURFACE_CALLS];
static int      s_call_n;
/* Optional rect witnesses: rect1 is the corpus's source rect (4 floats)
 * and rect3 the size pair (2 floats); the plain record leaves both
 * absent. */
static float    s_call_rect1[CRT_SURFACE_CALLS][4];
static float    s_call_rect3[CRT_SURFACE_CALLS][2];
static uint8_t  s_call_rect1_set[CRT_SURFACE_CALLS];
static uint8_t  s_call_rect3_set[CRT_SURFACE_CALLS];

static void surface_call_store(uint32_t handle, float x, float y,
                               const float* rect1, const float* rect3)
{
    if (s_call_n >= CRT_SURFACE_CALLS) {
        return;
    }
    s_call_handle[s_call_n] = handle;
    s_call_x[s_call_n] = x;
    s_call_y[s_call_n] = y;
    s_call_rect1_set[s_call_n] = 0u;
    s_call_rect3_set[s_call_n] = 0u;
    if (rect1 != NULL) {
        __builtin_memcpy(s_call_rect1[s_call_n], rect1,
                         sizeof s_call_rect1[s_call_n]);
        s_call_rect1_set[s_call_n] = 1u;
    }
    if (rect3 != NULL) {
        __builtin_memcpy(s_call_rect3[s_call_n], rect3,
                         sizeof s_call_rect3[s_call_n]);
        s_call_rect3_set[s_call_n] = 1u;
    }
    ++s_call_n;
}

uint32_t crt_surface_handle_next(void)
{
    static uint32_t s_handle_counter = 0u;
    ++s_handle_counter;
    return s_handle_counter;
}

void crt_surface_blit_record(uint32_t handle, float x, float y)
{
    surface_call_store(handle, x, y, NULL, NULL);
}

void crt_surface_blit_record_rect(uint32_t handle, float x, float y,
                                  const float* rect1, const float* rect3)
{
    surface_call_store(handle, x, y, rect1, rect3);
}

int crt_surface_calls_count(void) { return s_call_n; }

int crt_surface_call_get(int index, uint32_t* handle, float* x, float* y)
{
    if (index < 0 || index >= s_call_n) {
        return 0;
    }
    if (handle != NULL) {
        *handle = s_call_handle[index];
    }
    if (x != NULL) {
        *x = s_call_x[index];
    }
    if (y != NULL) {
        *y = s_call_y[index];
    }
    return 1;
}

int crt_surface_call_get_rect(int index, float* rect1, int* rect1_set,
                              float* rect3, int* rect3_set)
{
    if (index < 0 || index >= s_call_n) {
        return 0;
    }
    if (rect1_set != NULL) {
        *rect1_set = s_call_rect1_set[index] != 0u;
    }
    if (rect3_set != NULL) {
        *rect3_set = s_call_rect3_set[index] != 0u;
    }
    if (s_call_rect1_set[index] != 0u && rect1 != NULL) {
        __builtin_memcpy(rect1, s_call_rect1[index],
                         4 * sizeof(float));
    }
    if (s_call_rect3_set[index] != 0u && rect3 != NULL) {
        __builtin_memcpy(rect3, s_call_rect3[index], 2 * sizeof(float));
    }
    return 1;
}

void crt_surface_calls_clear(void) { s_call_n = 0; }

float crt_float_bits(uint32_t bits)
{
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

static uint32_t float_bits(float f)
{
    uint32_t b = 0u;
    __builtin_memcpy(&b, &f, sizeof(b));
    return b;
}

void view_blit_window_set(int view, uint32_t handle, void* rect_a,
                          void* rect_b, int region_index)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    s_view_handle[view] = handle;
    s_view_rect_a[view] = rect_a;
    s_view_rect_b[view] = rect_b;
    s_view_region[view] = region_index;
}

void view_blit_window_peek(int view, uint32_t* handle, void** rect_a,
                           void** rect_b, int* region_index)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    if (handle != NULL) {
        *handle = s_view_handle[view];
    }
    if (rect_a != NULL) {
        *rect_a = s_view_rect_a[view];
    }
    if (rect_b != NULL) {
        *rect_b = s_view_rect_b[view];
    }
    if (region_index != NULL) {
        *region_index = s_view_region[view];
    }
}

void panel_draw_count_set(int panel, int count)
{
    if (panel >= 0 && panel < DAT_WINDOW_COUNT) {
        s_panel_count[panel] = count;
    }
}

int panel_draw_count_peek(int panel)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT) {
        return 0;
    }
    return s_panel_count[panel];
}

/* PanelItemFlag8SetIfEnabled (FUN_0045abf0) — see crt.hpp. Bit 8 of the
 * stride-0x2c item's word at +0x28 only follows the flag while bit 32
 * (the enable gate) is set; otherwise the write is skipped. */
// FUNCTION: LEGORR 0x0045abf0
void panel_item_flag8_set_if_enabled(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    if ((*w & 0x20u) == 0u) {
        return;
    }
    *w = value != 0 ? *w | 8u : *w & ~8u;
}

/* ViewBlitRectPositionsApply (FUN_00463d20) — see crt.hpp. Rect records:
 * +0x00/+0x04 position, +0x10/+0x14 the size pair (bit-for-bit blit
 * handles), +0x18 the flag word. The corpus strides its own block and
 * dereferences every window blindly — the guards here are the documented
 * deviation. */
// FUNCTION: LEGORR 0x00463d20
void view_blit_rect_positions_apply(int view)
{
    if (view < 0 || view >= DAT_WINDOW_COUNT) {
        return;
    }
    const int k = s_view_region[view];
    if (k < 0 || k >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides its own block blindly */
    }
    const float ox = s_fpair_a[view] + (float)(int)s_pair_a[k];
    const float oy = s_fpair_b[view] + (float)(int)s_pair_b[k];
    crt_surface_blit_record(s_view_handle[view], ox, oy);

    for (int which = 0; which <= 1; ++which) {
        const float* pf = (const float*)(which == 0 ? s_view_rect_a[view]
                                                   : s_view_rect_b[view]);
        if (pf == NULL) {
            continue; /* deviation: the corpus reads blindly */
        }
        uint32_t flags = 0u;
        __builtin_memcpy(&flags, pf + 6, sizeof(flags));
        uint32_t handle = 0u;
        if (((flags & 2u) == 0u) || (pf[4] == 0.0f)) {
            if ((flags & 4u) != 0u && pf[5] != 0.0f) {
                handle = float_bits(pf[5]);
            } else {
                continue;
            }
        } else {
            handle = float_bits(pf[4]);
        }
        crt_surface_blit_record(handle, pf[0] + ox, pf[1] + oy);
    }

    const float* pf = (const float*)s_metric_records[view];
    if (pf == NULL) {
        return; /* deviation: the corpus dereferences its slot blindly */
    }
    crt_surface_blit_record(float_bits(pf[6]),
                            pf[0] + pf[4] + ox, pf[1] + pf[5] + oy);
}

/* PanelTextureSlotsAllocate (FUN_0045ac30) — see crt.hpp. Draw-record
 * fields: +0x00/+0x04 position, +0x08/+0x0c the size sums, +0x10/+0x14/
 * +0x18 the texture-slot handles, +0x20 the id word, +0x28 the flag word.
 * The corpus stores the new block over the panel's previous one without
 * freeing it (faithful); a null allocator answer skips the fill loop
 * (deviation: the corpus strides on through the failure). */
// FUNCTION: LEGORR 0x0045ac30
void panel_texture_slots_allocate(int panel, int count, const int* slot_index,
                                  const float* stream, const int* tex1,
                                  const int* tex2, const int* tex3,
                                  const int* ids)
{
    if (panel < 0 || panel >= DAT_WINDOW_COUNT || count <= 0) {
        return;
    }
    float* block = (float*)engine_heap_alloc((long)count * 0x2c);
    panel_draw_records_set(panel, block);
    panel_draw_count_set(panel, count);
    if (block == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const int slot = (slot_index != NULL) ? slot_index[i] : i;
        if (slot < 0 || slot >= count) {
            continue; /* deviation: the corpus strides blindly */
        }
        float* rec = block + (size_t)slot * 11;
        const float* s = stream + (size_t)i * 4;
        rec[0] = s[0];
        rec[1] = s[1];
        rec[2] = s[2] + s[0];
        rec[3] = s[3] + s[1];
        rec[4] = 0.0f;
        rec[5] = 0.0f;
        rec[6] = 0.0f;
        rec[10] = 0.0f;
        if (ids != NULL) {
            rec[8] = crt_float_bits((uint32_t)ids[i]);
        }
        const int* tex[3] = { tex1, tex2, tex3 };
        const int field[3] = { 6, 4, 5 }; /* corpus slot order: +0x18,
                                            +0x10, +0x14 */
        for (int t = 0; t < 3; ++t) {
            if (tex[t] != NULL && tex[t][i] != 0) {
                const uint32_t h = crt_surface_handle_next();
                rec[field[t]] = crt_float_bits(h); /* load seam: a handle */
                crt_surface_blit_record(h, 0.0f, 0.0f); /* transform call */
            }
        }
    }
}

/* --- the small global primitives band (see crt.hpp) -------------------------- */

// FUNCTION: LEGORR 0x00497aa0
void uint96_zero(uint32_t limbs[3])
{
    limbs[0] = 0u;
    limbs[1] = 0u;
    limbs[2] = 0u;
}

/* GlobalValueClearIfEqual (FUN_0040e910) — the screen-effect handle word
 * (the corpus's DAT_004c7ea4, unowned) clears only on a matching release. */
// GLOBAL: LEGORR 0x004c7ea4
static int s_global_value_word;

// FUNCTION: LEGORR 0x0040e910
void global_value_clear_if_equal(int value)
{
    if (s_global_value_word == value) {
        s_global_value_word = 0;
    }
}

void global_value_word_set(int value) { s_global_value_word = value; }
int  global_value_word_peek(void) { return s_global_value_word; }

/* The two global-list pushes (FUN_0046a470/FUN_004792c0) — the corpus's
 * head words have no ported owner, so the heads land here; the link field
 * at record +0x28 is a port-side pointer-sized slot (the corpus's 32-bit
 * dword, widened with the host — deviation documented). */
/* Datacmp note: the corpus words at 0x4aa450/0x4aa458 are file-backed
 * zeros while the port's zero-init statics land in the exe's zero-fill
 * tail; reccmp-datacmp reads only file bytes per side and flags the pair
 * (uninitialized) — both sides are genuinely zero (image-verified). */
// GLOBAL: LEGORR 0x004aa458
static void* s_linked_list_head = NULL;  /* DAT_004aa458 role */
static void* s_record_list_head;  /* DAT_00506518 role */

static void record_list_push(void** head, void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    *(void**)((char*)record + 0x1c) = *head;
    *head = record;
}

// FUNCTION: LEGORR 0x0046a470
void linked_list_push_at_global_head(void* record)
{
    record_list_push(&s_linked_list_head, record);
}

void* linked_list_global_head_peek(void) { return s_linked_list_head; }

void linked_list_global_head_set(void* record)
{
    s_linked_list_head = record;
}

// FUNCTION: LEGORR 0x004792c0
void global_record_list_push_head(void* record)
{
    record_list_push(&s_record_list_head, record);
}

void* global_record_list_head_peek(void) { return s_record_list_head; }

/* SceneEffectRegistryReset (FUN_00465380) — the transient-effect registry
 * words (head/tail/shared-texture, unowned) clear and the reset answers 1
 * exactly as the corpus does. The corpus addresses the trio individually at
 * 0x00552128/0x0055212c/0x00552130, so each word is its own annotated
 * static (the s_indexed_window precedent); all three are zero-filled
 * virtual tail, matching the zero-initialized statics here. */
// GLOBAL: LEGORR 0x00552128
static uint32_t s_scene_effect_head;     /* DAT_00552128 role: transient-effect head */
// GLOBAL: LEGORR 0x0055212c
static uint32_t s_scene_effect_tail;     /* DAT_0055212c role: transient-effect tail */
// GLOBAL: LEGORR 0x00552130
static uint32_t s_scene_effect_shared;   /* DAT_00552130 role: shared texture */

// FUNCTION: LEGORR 0x00465380
int scene_effect_registry_reset(void)
{
    s_scene_effect_head = 0u;    /* corpus clears in address order */
    s_scene_effect_tail = 0u;
    s_scene_effect_shared = 0u;
    return 1;
}

void scene_effect_registry_words_peek(uint32_t out_words[3])
{
    if (out_words == NULL) {
        return;
    }
    out_words[0] = s_scene_effect_head;
    out_words[1] = s_scene_effect_tail;
    out_words[2] = s_scene_effect_shared;
}

/* CrtFatalMessageExit (FUN_00498340) — the corpus's exhausted-memory
 * abort. The port aborts with the host runtime, or routes through the
 * observer hook when installed so the path is testable (deviation). */
static void (*s_fatal_exit_observer)(void);

// FUNCTION: LEGORR 0x00498340
void crt_fatal_message_exit(void)
{
    if (s_fatal_exit_observer != NULL) {
        s_fatal_exit_observer();
        return;
    }
    abort();
}

void crt_fatal_exit_observer_set(void (*observer)(void))
{
    s_fatal_exit_observer = observer;
}

/* CrtNoOpStubVariant2 (FUN_00493c40) — see crt.hpp. */
// FUNCTION: LEGORR 0x00493c40
void crt_no_op_stub_variant2(void)
{
    return;
}

/* LinkedRecordChainFree / GlobalLinkedRecordChainClear — see crt.hpp. */
// GLOBAL: LEGORR 0x004aa450
static void* s_linked_list_aux = NULL; /* DAT_004aa450 role: unowned, zero-init */

// FUNCTION: LEGORR 0x0046a370
void linked_record_chain_free(void* record)
{
    if (record == NULL) {
        return; /* the corpus's gate */
    }
    void* next = NULL;
    __builtin_memcpy(&next, (char*)record + 0x1c, sizeof(next));
    linked_record_chain_free(next);
    engine_heap_free(record);
}

// FUNCTION: LEGORR 0x0046a390
void global_linked_record_chain_clear(void)
{
    void* head = s_linked_list_head; /* the corpus reads the head first */
    s_linked_list_aux = NULL;
    linked_record_chain_free(head);
    s_linked_list_head = NULL;
}

void* linked_list_global_aux_peek(void) { return s_linked_list_aux; }
void  linked_list_global_aux_set(void* word) { s_linked_list_aux = word; }

/* ReleaseLinkedObjectList (FUN_00479240) — see crt.hpp. */
// FUNCTION: LEGORR 0x00479240
void linked_object_list_release(void* list)
{
    if (list == NULL) {
        return; /* deviation: the corpus strides the head blindly */
    }
    void* owner = *(void* const*)list;
    engine_heap_free(owner);
    for (void* node = list; node != NULL;) {
        void* next = *(void* const*)((char*)node + 0x14);
        global_record_list_push_head(node);
        node = next;
    }
}

/* ModelRenderListClear (FUN_00482d30) — see crt.hpp. */
// GLOBAL: LEGORR 0x005343c4
static void* s_model_render_list_head; /* DAT_005343c4 role: unowned */

// FUNCTION: LEGORR 0x00482d30
void model_render_list_clear(void)
{
    void* node = s_model_render_list_head;
    while (node != NULL) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)node + 0x44, sizeof next);
        engine_heap_free(node);
        node = next;
    }
    s_model_render_list_head = NULL;
}

void model_render_list_head_set(void* node) { s_model_render_list_head = node; }
void* model_render_list_head_peek(void) { return s_model_render_list_head; }

/* RendererCommandNodePrepend (FUN_00482ce0) — see crt.hpp. */
// FUNCTION: LEGORR 0x00482ce0
void renderer_command_node_prepend(uint32_t kind, const uint32_t* payload)
{
    /* Deviation: the port lays the corpus's dword node window out with
     * the widened host-word window (the CrtOwnedStreamBufferRelease
     * convention) while the next link rides as a widened void* at the
     * corpus's byte slot +0x44 (the ModelRenderListClear precedent). */
    char* node = (char*)engine_heap_alloc(18 * (long)sizeof(void*));
    if (node == NULL) {
        return; /* deviation: the corpus strides its alloc blindly */
    }
    memset(node, 0, 18 * (size_t)sizeof(void*));
    const uint32_t zero = 0u;
    __builtin_memcpy(node + 0x00, &kind, sizeof kind);
    for (int i = 0; i < 16; ++i) {
        const uint32_t v = (payload != NULL) ? payload[i] : zero;
        __builtin_memcpy(node + 0x04 + (size_t)i * sizeof v, &v, sizeof v);
    }
    void* head = model_render_list_head_peek();
    __builtin_memcpy(node + 0x44, &head, sizeof head);
    model_render_list_head_set(node);
}

/* CrtOwnedStreamBufferRelease (FUN_00494df0) — see crt.hpp. */
// FUNCTION: LEGORR 0x00494df0
void crt_owned_stream_buffer_release(char* record)
{
    /* Fields 0-2 are host pointers; field 3 is the following flags dword. */
    void** fields = (void**)record;
    uint32_t* flags = (uint32_t*)(fields + 3);
    if ((*flags & 0x83u) != 0u && (*flags & 0x8u) != 0u) {
        engine_heap_free(fields[2]);
        *flags &= 0xfffffbf7u; /* corpus mask: bits 3 and 11 */
        fields[0] = NULL;
        fields[2] = NULL;
        fields[1] = NULL;
    }
}

/* --- the shared texture directory word (see crt.hpp) ------------------------ */
// GLOBAL: LEGORR 0x0076cd88
static char* s_shared_texture_dir; /* DAT_0076cd88 role: unowned */

// FUNCTION: LEGORR 0x004728c0
void shared_texture_directory_set(const char* path)
{
    s_shared_texture_dir = string_duplicate(path);
}

char* shared_texture_directory_peek(void) { return s_shared_texture_dir; }
void shared_texture_directory_clear(void)
{
    engine_heap_free(s_shared_texture_dir);
    s_shared_texture_dir = NULL;
}

/* --- the renderer flag word + scene hook pair band (see crt.hpp) ----------- */
// GLOBAL: LEGORR 0x0076cd94
static uint32_t s_renderer_flag_word; /* DAT_0076cd94 role: unowned */
// GLOBAL: LEGORR 0x0076cd80
static void*    s_scene_end_hook;     /* DAT_0076cd80 role: unowned */
// GLOBAL: LEGORR 0x0076cd84
static void*    s_scene_end_value;    /* DAT_0076cd84 role: unowned */
// GLOBAL: LEGORR 0x0076cd78
static void*    s_scene_sample_hook;  /* DAT_0076cd78 role: unowned */
// GLOBAL: LEGORR 0x0076cd7c
static void*    s_scene_sample_value; /* DAT_0076cd7c role: unowned */

// FUNCTION: LEGORR 0x004728e0
void renderer_flag_bit6_set(int value)
{
    s_renderer_flag_word = (value != 0) ? (s_renderer_flag_word | 0x40u)
                                        : (s_renderer_flag_word & ~0x40u);
}

uint32_t renderer_flag_word_peek(void) { return s_renderer_flag_word; }

// FUNCTION: LEGORR 0x00472900
void scene_end_hook_register(void* hook, void* value)
{
    s_scene_end_hook = hook;
    s_scene_end_value = value;
}

void* scene_end_hook_peek(void) { return s_scene_end_hook; }
void* scene_end_hook_value_peek(void) { return s_scene_end_value; }

// FUNCTION: LEGORR 0x00472920
void scene_sample_hook_register(void* hook, void* value)
{
    s_scene_sample_hook = hook;
    s_scene_sample_value = value;
    s_renderer_flag_word |= 0x40u;
}

void* scene_sample_hook_peek(void) { return s_scene_sample_hook; }
void* scene_sample_hook_value_peek(void) { return s_scene_sample_value; }

/* --- the string registry (see crt.hpp) ------------------------------------- */
// GLOBAL: LEGORR 0x004db8cc
static void* s_string_registry_head; /* DAT_004db8cc role: unowned */

/* The corpus's locale-following compare, modeled on the port's fold pattern
 * (see resource.cpp's helper). */
static int s_strcasecmp(const char* a, const char* b)
{
    for (;; a++, b++) {
        int ca = (unsigned char)*a;
        int cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return ca - cb;
        if (ca == 0) return 0;
    }
}

// FUNCTION: LEGORR 0x00410450
void* string_keyed_cache_entry_find(const char* key)
{
    /* Corpus walk: the head gates once, the name rides strcasecmp_lite
     * untested (intern only ever links an entry whose duplicated name
     * landed), and the +0x0c link advances through a bottom-tested
     * test/jne. */
    void* entry = s_string_registry_head;
    if (entry == NULL) {
        return NULL;
    }
    for (;;) {
        if (strcasecmp_lite(*(char**)entry, key) == 0) {
            return entry;
        }
        entry = *(void**)((char*)entry + 0xc); /* corpus mov esi,[esi+0xc] */
        if (entry == NULL) {
            return NULL;
        }
    }
}

// FUNCTION: LEGORR 0x00410490
void* string_registry_intern(const char* key)
{
    if (key == NULL || *key == '\0') {
        return NULL; /* deviation: the corpus strides a null string blindly */
    }
    void* entry = string_keyed_cache_entry_find(key);
    if (entry != NULL) {
        return entry;
    }
    /* Corpus strdup idiom: both allocations are unchecked (the corpus
     * stores blind — a NULL allocator answer writes through it), the
     * length rides the inlined strlen intrinsic into the alloc push, and
     * the copy pointer lands in field 0 before the inlined strcpy runs. */
    void* record = engine_heap_alloc(0x10);
    char** slots = (char**)record;
    char* copy = (char*)engine_heap_alloc((long)strlen(key) + 1);
    slots[0] = copy; /* the corpus stores the copy pointer before the copy */
    strcpy(copy, key);
    void* head = s_string_registry_head; /* the head is loaded once, here */
    slots[1] = NULL;
    slots[3] = (char*)head;
    slots[2] = NULL;
    s_string_registry_head = record;
    return record;
}

void string_registry_reset(void)
{
    const long field = (long)sizeof(void*);
    void* entry = s_string_registry_head;
    while (entry != NULL) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)entry + 3 * field, sizeof next);
        char* key = NULL;
        __builtin_memcpy(&key, (char*)entry + 0 * field, sizeof key);
        engine_heap_free(key);
        engine_heap_free(entry);
        entry = next;
    }
    s_string_registry_head = NULL;
}

static void* (*s_bitmap_surface_loader)(const char*);
static void* (*s_font_resource_loader)(const char*);

void string_cache_bitmap_loader_set(void* (*hook)(const char*))
{
    s_bitmap_surface_loader = hook;
}

void string_cache_font_loader_set(void* (*hook)(const char*))
{
    s_font_resource_loader = hook;
}

/* Shared cache-slot fold: load once on a zero slot, return the word. */
static void* s_entry_cache_slot_get(const char* key, long slot,
                                    void* (*loader)(const char*))
{
    if (key == NULL || *key == '\0') {
        return NULL;
    }
    void* entry = string_registry_intern(key);
    if (entry == NULL) {
        return NULL; /* deviation: the corpus strides the intern failure */
    }
    void* value = NULL;
    __builtin_memcpy(&value, (char*)entry + slot * (long)sizeof(void*),
                     sizeof value);
    if (value == NULL) {
        value = (loader != NULL) ? loader(key)
                                 : NULL; /* seam: answers NULL uninstalled */
        if (value != NULL) {
            __builtin_memcpy((char*)entry + slot * (long)sizeof(void*),
                             &value, sizeof value);
        }
    }
    return value;
}

// FUNCTION: LEGORR 0x00410520
void* string_cache_lookup_or_load(const char* key)
{
    return s_entry_cache_slot_get(key, 1, s_bitmap_surface_loader);
}

// FUNCTION: LEGORR 0x00410560
void* resource_name_cache_get_or_load(const char* key)
{
    return s_entry_cache_slot_get(key, 2, s_font_resource_loader);
}

/* --- the selection marker table (see crt.hpp) ------------------------------
 * DAT_004c7e00..e7c is 32 pointer slots plus the count word at e80; the
 * descriptor sits at DAT_004c7df8. All unowned: zero-init statics here. */
// GLOBAL: LEGORR 0x004c7e00
static void*    s_marker_table[32];
// GLOBAL: LEGORR 0x004c7e80
static uint32_t s_marker_count;
// GLOBAL: LEGORR 0x004c7df8
static void*    s_marker_descriptor;

// FUNCTION: LEGORR 0x0040ce30
void selection_marker_resources_release(void)
{
    /* Corpus walk: an unsigned add-4 pointer stride bounded by the table
     * end. Deviation: each freed slot is cleared in place (the corpus
     * dangles, which a second release could double-free), so the window
     * stays idempotent. */
    void** p = s_marker_table;
    while (p < s_marker_table + 32) {
        void* slot = *p;
        if (slot != NULL) {
            engine_heap_free(slot);
            *p = NULL;
        }
        ++p;
    }
    s_marker_count = 0u;
    if (s_marker_descriptor != NULL) {
        engine_heap_free(s_marker_descriptor);
        s_marker_descriptor = NULL;
    }
}

void selection_marker_slot_set(int index, void* ptr)
{
    if (index >= 0 && index < 32) {
        s_marker_table[index] = ptr;
    }
}
void* selection_marker_slot_peek(int index)
{
    return (index >= 0 && index < 32) ? s_marker_table[index] : NULL;
}
void  selection_marker_descriptor_set(void* desc) { s_marker_descriptor = desc; }
void* selection_marker_descriptor_peek(void) { return s_marker_descriptor; }
void     selection_marker_count_set(uint32_t count) { s_marker_count = count; }
uint32_t selection_marker_count_peek(void) { return s_marker_count; }

/* OwnedResourceTreeRelease (FUN_0048c180) — see crt.hpp. */
// FUNCTION: LEGORR 0x0048c180
void owned_resource_tree_release(void** node, int own_node)
{
    if (node == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    if (node[0] != NULL) {
        engine_heap_free(node[0]);
    }
    if (node[2] != NULL) {
        engine_heap_free(node[2]);
    }
    if (node[1] != NULL) {
        owned_resource_tree_release((void**)node[1], 1);
    }
    if (own_node != 0) {
        engine_heap_free((void*)node);
    }
}

/* --- the string + stream allocation leaves (see crt.hpp) ------------------- */

/* StringPrefixReserve (FUN_004907c0): the scan is a strlen; the shift is an
 * overlapping move of the whole string (NUL included) `count` bytes right. */
// FUNCTION: LEGORR 0x004907c0
void string_prefix_reserve(char* s, int count)
{
    if (count != 0) {
        const size_t bytes = strlen(s) + 1u;
        crt_memory_move(s + count, s, bytes);
    }
}

/* CrtStreamBufferAllocate (FUN_00498ae0) — see crt.hpp. DAT_005493e4 is
 * unowned: zero-init hooked static. */
// GLOBAL: LEGORR 0x005493e4
static long s_stream_alloc_count;

long stream_allocation_count_peek(void) { return s_stream_alloc_count; }
void stream_allocation_count_reset(void) { s_stream_alloc_count = 0; }

// FUNCTION: LEGORR 0x00498ae0
void crt_stream_buffer_allocate(char* record)
{
#if defined(_WIN64) || defined(__x86_64__) || defined(__aarch64__)
    const long field = (long)sizeof(void*);
    void* buffer = engine_heap_alloc(0x1000);
    void* zero = NULL;
    uint32_t flags;
    ++s_stream_alloc_count;
    __builtin_memcpy(record + 2 * field, &buffer, sizeof buffer);
    __builtin_memcpy(&flags, record + 3 * field, sizeof flags);
    if (buffer != NULL) {
        flags |= 8u;
        {
            const uint32_t cap = 0x1000u;
            __builtin_memcpy(record + 6 * field, &cap, sizeof cap);
        }
    } else {
        flags |= 4u;
        {
            const uint32_t cap = 2u;
            __builtin_memcpy(record + 6 * field, &cap, sizeof cap);
        }
        buffer = record + 5 * field;
        __builtin_memcpy(record + 2 * field, &buffer, sizeof buffer);
    }
    __builtin_memcpy(record + 3 * field, &flags, sizeof flags);
    __builtin_memcpy(record + 1 * field, &zero, sizeof zero);
    __builtin_memcpy(record + 0 * field, &buffer, sizeof buffer);
#else
    void* buffer;
    uint32_t* fields = (uint32_t*)record;
    ++s_stream_alloc_count;
    buffer = engine_heap_alloc(0x1000);
    fields[2] = (uint32_t)buffer;
    if (buffer != NULL) {
        fields[3] |= 8u;
        fields[6] = 0x1000;
        fields[0] = (uint32_t)buffer;
        fields[1] = 0;
    } else {
        fields[6] = 2;
        fields[3] |= 4u;
        fields[2] = (uint32_t)(record + 20);
        fields[0] = fields[2];
        fields[1] = 0;
    }
#endif
}

// FUNCTION: LEGORR 0x00498f50
uint32_t crt_stream_unget_byte(uint32_t value, char* record)
{
    int* fields = (int*)record;
    int current;
    uint32_t flags;
    if (value != 0xffffffffu &&
        (((uint32_t)fields[3] & 1u) != 0u ||
         (((uint32_t)fields[3] & 0x80u) != 0u &&
          ((uint32_t)fields[3] & 2u) == 0u))) {
        if (fields[2] == 0) {
            crt_stream_buffer_allocate(record);
        }
        if (fields[0] == fields[2]) {
            if (fields[1] != 0) {
                return 0xffffffffu;
            }
            fields[0] = fields[0] + 1;
        }
        if (((uint32_t)fields[3] & 0x40u) == 0u) {
            current = fields[0];
            fields[0] = current - 1;
            *(char*)(current - 1) = (char)value;
        } else {
            current = fields[0];
            fields[0] = current - 1;
            if (*(char*)(current - 1) != (char)value) {
                fields[0] = current;
                return 0xffffffffu;
            }
        }
        fields[1] = fields[1] + 1;
        fields[3] = (fields[3] & 0xffffffefu) | 1;
        return value & 0xffu;
    }
    return 0xffffffffu;
}

/* UInt96Add (FUN_00499fe0) — see crt.hpp. Every carry-add writes its sum
 * whether or not it carries; only the bump is gated. */
// FUNCTION: LEGORR 0x00499fe0
void crt_uint96_add(uint32_t* dst, const uint32_t* src)
{
    int carry = crt_unsigned_add_with_carry(dst[0], src[0], &dst[0]);
    if (carry != 0) {
        carry = crt_unsigned_add_with_carry(dst[1], 1u, &dst[1]);
        if (carry != 0) {
            dst[2] = dst[2] + 1u;
        }
    }
    carry = crt_unsigned_add_with_carry(dst[1], src[1], &dst[1]);
    if (carry != 0) {
        dst[2] = dst[2] + 1u;
    }
    crt_unsigned_add_with_carry(dst[2], src[2], &dst[2]);
}

// FUNCTION: LEGORR 0x0049a0b0
void crt_decimal_digit_sequence_to_extended96(const char* digits, int count,
                                              void* out_extended96)
{
    uint32_t* limbs = (uint32_t*)out_extended96;
    int16_t exponent = 0x404e;
    int i;

    limbs[0] = 0;
    limbs[1] = 0;
    limbs[2] = 0;
    for (i = 0; i < count; ++i) {
        uint32_t original[3];
        uint32_t digit[3];

        original[0] = limbs[0];
        original[1] = limbs[1];
        original[2] = limbs[2];
        extended_float96_bit_shift_left_one(limbs);
        extended_float96_bit_shift_left_one(limbs);
        crt_uint96_add(limbs, original);
        extended_float96_bit_shift_left_one(limbs);

        digit[0] = (uint8_t)digits[i];
        digit[1] = 0;
        digit[2] = 0;
        crt_uint96_add(limbs, digit);
    }

    while (limbs[2] == 0) {
        exponent = (int16_t)(exponent - 16);
        limbs[2] = limbs[1] >> 16;
        limbs[1] = (limbs[0] >> 16) | (limbs[1] << 16);
        limbs[0] <<= 16;
    }
    while ((limbs[2] & 0x8000u) == 0) {
        extended_float96_bit_shift_left_one(limbs);
        exponent = (int16_t)(exponent - 1);
    }
    *(int16_t*)((uint8_t*)out_extended96 + 10) = exponent;
}

// FUNCTION: LEGORR 0x00497f40
void crt_double_to_extended80(const void* source_double, void* out_extended80)
{
    const uint32_t* source = (const uint32_t*)source_double;
    uint32_t* destination = (uint32_t*)out_extended80;
    uint16_t source_high = *(const uint16_t*)((const uint8_t*)source_double + 6);
    uint32_t exponent = (source_high & 0x7ff0u) >> 4;
    uint32_t sign_bit = 0x80000000u;
    uint32_t significand_high;
    uint32_t significand_low;
    uint32_t extended_exponent;

    significand_low = source[0];
    if (exponent == 0u) {
        sign_bit = 0u;
        if (((source[1] & 0x000fffffu) == 0u) && (significand_low == 0u)) {
            destination[0] = 0u;
            destination[1] = 0u;
            *(uint16_t*)(destination + 2) = 0u;
            return;
        }
        extended_exponent = 0x3c01u;
    }
    else if (exponent == 0x7ffu) {
        extended_exponent = 0x7fffu;
    }
    else {
        extended_exponent = exponent + 0x3c00u;
    }

    significand_high = (significand_low >> 21) |
                       ((source[1] & 0x000fffffu) << 11) | sign_bit;
    destination[1] = significand_high;
    destination[0] = significand_low << 11;
    while ((significand_high & 0x80000000u) == 0u) {
        significand_high = (destination[0] >> 31) |
                           (significand_high << 1);
        extended_exponent = (extended_exponent - 1u) & 0xffffu;
        destination[1] = significand_high;
        destination[0] <<= 1;
    }
    *(uint16_t*)(destination + 2) =
        (uint16_t)(extended_exponent | (source_high & 0x8000u));
}

/* UInt96IncrementAtBit (FUN_00497970) — see crt.hpp. The carry walks DOWN
 * the limb indices (toward limb 0), the opposite direction from the
 * uint96-add chain; the corpus's bit-mask reduction rides the byte-trick
 * expression, which simplifies to bit % 32 for the non-negative inputs. */
static uint32_t uint96_increment_at_bit_with_carry(uint32_t limbs[3], int bit)
{
    if (limbs == NULL) {
        return 0u; /* deviation: the corpus strides the limbs blindly */
    }
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    if (limb < 0 || limb > 2) {
        return 0u; /* deviation: the corpus strides the limbs blindly */
    }
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t mask = 1u << (31u - (uint32_t)idx);
    uint32_t sum = 0;
    int carry = crt_unsigned_add_with_carry(limbs[limb], mask, &sum);
    limbs[limb] = sum;
    for (int i = limb - 1; i >= 0; --i) {
        if (carry == 0) {
            return 0u;
        }
        carry = crt_unsigned_add_with_carry(limbs[i], 1u, &sum);
        limbs[i] = sum;
    }
    return (uint32_t)carry;
}

// FUNCTION: LEGORR 0x00497970
void crt_uint96_increment_at_bit(uint32_t limbs[3], int bit)
{
    if (limbs == NULL) {
        return; /* deviation: the corpus strides the limbs blindly */
    }
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    if (limb < 0 || limb > 2) {
        return; /* deviation: the corpus strides the limbs blindly */
    }
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t mask = 1u << (31u - (uint32_t)idx);
    uint32_t sum = 0;
    int carry = crt_unsigned_add_with_carry(limbs[limb], mask, &sum);
    limbs[limb] = sum;
    for (int i = limb - 1; i >= 0; --i) {
        if (carry == 0) {
            return;
        }
        carry = crt_unsigned_add_with_carry(limbs[i], 1u, &sum);
        limbs[i] = sum;
    }
}

/* UInt96HasNoBitsAtOrAbove (FUN_00497900) — see crt.hpp. The bit-index
 * numbering is the corpus's MSB-first one (bit n lives in limb n / 32 at
 * word position 31 - n % 32), and the in-limb mask
 * ~(-1 << (31 - bit % 32)) covers the positions STRICTLY below the named
 * bit, i.e. the bits above it; equal-index bits don't count. */
// FUNCTION: LEGORR 0x00497900
uint32_t crt_uint96_has_no_bits_at_or_above(const uint32_t limbs[3], int bit)
{
    if (limbs == NULL) {
        return 1u; /* deviation: the corpus strides the limbs blindly */
    }
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    if (limb < 0 || limb > 2) {
        return 1u; /* deviation: the corpus strides the limbs blindly */
    }
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t mask = ~((uint32_t)0xffffffffu << (31u - (uint32_t)idx));
    if ((limbs[limb] & mask) != 0u) {
        return 0u;
    }
    for (int i = limb + 1; i < 3; ++i) {
        if (limbs[i] != 0u) {
            return 0u;
        }
    }
    return 1u;
}

/* UInt96ClearFromBit (FUN_004979e0) — see crt.hpp. The increment branch
 * runs only while "bit is set AND something lives at or above bit + 1";
 * its final carry is returned because the extended-float converter uses it
 * to adjust the exponent after rounding. */
// FUNCTION: LEGORR 0x004979e0
uint32_t crt_uint96_clear_from_bit(uint32_t limbs[3], int bit)
{
    if (limbs == NULL) {
        return 0u; /* deviation: the corpus strides the limbs blindly */
    }
    const int limb = (bit + (int)((bit >> 31) & 0x1fu)) >> 5;
    if (limb < 0 || limb > 2) {
        return 0u; /* deviation: the corpus strides the limbs blindly */
    }
    const unsigned char b = (unsigned char)(bit >> 31);
    int idx = ((((unsigned char)bit ^ b) - b) & 0x1f) ^ b;
    idx = (idx - b) & 0x1f;
    const uint32_t shift = 31u - (uint32_t)idx;
    uint32_t result = 0u;
    if ((limbs[limb] & (1u << shift)) != 0u &&
        crt_uint96_has_no_bits_at_or_above(limbs, bit + 1) == 0u) {
        result = uint96_increment_at_bit_with_carry(limbs, bit - 1);
    }
    limbs[limb] &= ((uint32_t)0xffffffffu) << shift;
    for (int i = limb + 1; i < 3; ++i) {
        limbs[i] = 0u;
    }
    return result;
}

/* --- the pair-table window pair (see crt.hpp) ------------------------------
 * The find scan keeps the corpus's masked walk: the compare index is the
 * low 16 bits of the hop counter and the bound test reads that mask.
 * Deviations: the port guards the table and the key; the corpus strides
 * them (and its append blindly stores past the caller's window). */
// FUNCTION: LEGORR 0x00488730
void* pair_table_find(const void* table, uint32_t count, const char* key)
{
    if (table == NULL || count == 0u || key == NULL) {
        return NULL; /* deviation: the corpus strides the table blindly */
    }
    const long field = (long)sizeof(void*);
    uint32_t hops = 0;
    for (;;) {
        const uint32_t idx = hops & 0xffffu; /* pinned: corpus walk mask */
        char* name = NULL;
        __builtin_memcpy(&name, (const char*)table + (long)idx * 2 * field,
                         sizeof name);
        void* value = NULL;
        __builtin_memcpy(&value,
                         (const char*)table + ((long)idx * 2 + 1) * field,
                         sizeof value);
        if (name != NULL && s_strcasecmp(key, name) == 0) {
            return value;
        }
        ++hops;
        if ((hops & 0xffffu) >= count) {
            return NULL;
        }
    }
}

// FUNCTION: LEGORR 0x00488790
void pair_table_append(void* table, int* count, const char* key,
                       void* value)
{
    if (table == NULL || count == NULL) {
        return; /* deviation: the corpus strides both blindly */
    }
    const long field = (long)sizeof(void*);
    char* copy = string_duplicate(key); /* FUN_00477550 */
    char* entry = (char*)table + (long)*count * 2 * field;
    __builtin_memcpy(entry, &copy, sizeof copy);
    __builtin_memcpy(entry + field, &value, sizeof value);
    *count += 1;
}

/* --- the renderer-word band (2026-10-25) ------------------------------------
 * The corpus's DAT_0076ab24/b28/b3c + DAT_0076ac84/ac8c + DAT_0050608c band
 * has no ported writer; each word lands as a zero-init static and the six
 * getters answer their installed half (the unowned-global deviation, per the
 * DAT-window convention). The install hooks are port-side. */
// GLOBAL: LEGORR 0x0076ab24
static uint32_t s_renderer_word_a;      /* DAT_0076ab24 */
// GLOBAL: LEGORR 0x0076ab28
static uint32_t s_renderer_word_b;      /* DAT_0076ab28 */
// GLOBAL: LEGORR 0x0076ab3c
uint32_t g_renderer_word_c;             /* DAT_0076ab3c */
// GLOBAL: LEGORR 0x0076ac84
static uint32_t s_renderer_global_dword; /* DAT_0076ac84 */
// GLOBAL: LEGORR 0x0076ac8c
static uint32_t s_render_surface_secondary; /* DAT_0076ac8c */
// GLOBAL: LEGORR 0x0050608c
static uint32_t s_render_feature_flags; /* DAT_0050608c */

// FUNCTION: LEGORR 0x00410b30
uint32_t renderer_word_a_get(void) { return s_renderer_word_a; } /* FUN_00410b30 */
// FUNCTION: LEGORR 0x00410b40
uint32_t renderer_word_b_get(void) { return s_renderer_word_b; } /* FUN_00410b40 */
// FUNCTION: LEGORR 0x00410b50
uint32_t renderer_word_c_get(void) { return g_renderer_word_c; } /* FUN_00410b50 */
void renderer_words_set(uint32_t a, uint32_t b, uint32_t c)
{
    s_renderer_word_a = a;
    s_renderer_word_b = b;
    g_renderer_word_c = c;
}

// FUNCTION: LEGORR 0x004065d0
uint32_t renderer_global_dword_get(void) { return s_renderer_global_dword; }
void renderer_global_dword_set(uint32_t value) { s_renderer_global_dword = value; }

// FUNCTION: LEGORR 0x004065e0
uint32_t render_surface_secondary_get(void) { return s_render_surface_secondary; }
void render_surface_secondary_set(uint32_t value) { s_render_surface_secondary = value; }

// FUNCTION: LEGORR 0x00477f70
uint32_t render_feature_flags_get(void) { return s_render_feature_flags; }
void render_feature_flags_set(uint32_t value) { s_render_feature_flags = value; }

/* SetLowTextureFlag (2026-10-25): the corpus's DAT_00505f84 engine word has
 * a second half-model (EngineContext::feature in engine.hpp). This band's
 * store carries no context, so the port models it as a module static with
 * install/peek hooks — the two half-models do not interoperate
 * (unowned-global deviation). */
// GLOBAL: LEGORR 0x00505f84
static uint32_t s_engine_feature_word; /* DAT_00505f84 */

uint32_t engine_feature_word_peek(void)
{
    return s_engine_feature_word;
}

void engine_feature_word_install(uint32_t value) /* port hook */
{
    s_engine_feature_word = value;
}

// FUNCTION: LEGORR 0x00477bd0
void low_texture_flag_set(void) /* FUN_00477bd0 */
{
    if ((s_engine_feature_word & 0x40000u) == 0u) {
        s_engine_feature_word |= 0x20u;
    }
}

/* --- integer/string helper band (2026-10-25) -------------------------------- */

/* UInt96Copy (FUN_00497a80) — see crt.hpp. The corpus computes dst − src as
 * a byte distance and walks three dwords forward through the *source* side,
 * reading each word at the walked pointer and writing it back through the
 * delta, so each source word is read only after earlier writes land; the
 * ordered copy reproduces the overlap. */
// FUNCTION: LEGORR 0x00497a80
void uint96_copy(uint32_t dst[3], const uint32_t src[3])
{
    const uint32_t* p = src;
    const ptrdiff_t delta = (char*)dst - (const char*)src;
    int n = 3;
    do {
        *(uint32_t*)((char*)p + delta) = *p;
        p += 1;
        --n;
    } while (n != 0);
}

/* UInt96ShiftRight (FUN_00497ad0): shift the three big-endian DWORD limbs.
 * The conversion callers use nonnegative shifts, including bit and whole-word
 * components; preserve the signed quotient/remainder decomposition of the
 * original for callers outside that range as well. */
// FUNCTION: LEGORR 0x00497ad0
void uint96_shift_right(uint32_t limbs[3], int count)
{
    int word_shift;
    unsigned int bit_shift;
    uint32_t carry = 0u;
    int i;

    if (limbs == NULL) {
        return;
    }
    word_shift = (count + (int)((count >> 31) & 0x1fu)) >> 5;
    bit_shift = (unsigned int)count & 0x1fu;
    for (i = 0; i < 3; ++i) {
        const uint32_t current = limbs[i];
        limbs[i] = (current >> bit_shift) | carry;
        carry = (bit_shift == 0u) ? 0u :
            ((current & ((1u << bit_shift) - 1u)) << (32u - bit_shift));
    }
    for (i = 2; i >= 0; --i) {
        const int source = i - word_shift;
        if (source < 0 || source > 2) {
            limbs[i] = 0u;
        }
        else if (source != i) {
            limbs[i] = limbs[source];
        }
    }
}

// GLOBAL: LEGORR 0x004b28a8
static const int32_t s_extended_float_format_a[6] = {
    1024, -1023, 53, 11, 64, 1023
};
// GLOBAL: LEGORR 0x004b28c0
static const int32_t s_extended_float_format_b[6] = {
    128, -127, 24, 8, 32, 127
};

/* CrtExtended80ToBinaryFloat (FUN_00497b90): convert the CRT's packed
 * 80-bit significand/exponent form into a 32- or 64-bit IEEE layout. */
// FUNCTION: LEGORR 0x00497b90
uint32_t crt_extended80_to_binary_float(const uint16_t* source,
                                        uint32_t* destination,
                                        const int32_t* format)
{
    uint16_t sign_and_exponent = source[5];
    uint32_t limbs[3];
    uint32_t original_limbs[3];
    uint32_t low_word;
    uint32_t high_word;
    uint32_t exponent = sign_and_exponent & 0x7fffu;
    int binary_exponent = (int)exponent - 0x3fff;
    uint32_t status = 0u;

    memcpy(&limbs[1], source + 1, sizeof(uint32_t));
    memcpy(&limbs[0], source + 3, sizeof(uint32_t));
    limbs[2] = (uint32_t)source[0] << 16;

    if (binary_exponent == -0x3fff) {
        binary_exponent = 0;
        if (!uint96_is_zero(limbs)) {
            uint96_zero(limbs);
            status = 2u;
        }
    }
    else {
        uint96_copy(original_limbs, limbs);
        if (crt_uint96_clear_from_bit(limbs, format[2]) != 0u) {
            binary_exponent = (int)exponent - 0x3ffe;
        }
        if (binary_exponent < format[1] - format[2]) {
            uint96_zero(limbs);
            binary_exponent = 0;
            status = 2u;
        }
        else if (binary_exponent <= format[1]) {
            uint96_copy(limbs, original_limbs);
            uint96_shift_right(limbs, format[1] - binary_exponent);
            crt_uint96_clear_from_bit(limbs, format[2]);
            uint96_shift_right(limbs, format[3] + 1);
            binary_exponent = 0;
            status = 2u;
        }
        else if (binary_exponent >= format[0]) {
            uint96_zero(limbs);
            limbs[0] |= 0x80000000u;
            uint96_shift_right(limbs, format[3]);
            binary_exponent = format[5] + format[0];
            status = 1u;
        }
        else {
            binary_exponent += format[5];
            limbs[0] &= 0x7fffffffu;
            uint96_shift_right(limbs, format[3]);
        }
    }

    high_word = ((uint32_t)binary_exponent <<
                (0x1fu - (uint32_t)format[3] & 0x1fu)) |
                ((sign_and_exponent & 0x8000u) ? 0x80000000u : 0u) |
                limbs[0];
    low_word = limbs[1];
    if (format[4] == 0x40) {
        destination[0] = low_word;
        destination[1] = high_word;
    }
    else if (format[4] == 0x20) {
        destination[0] = high_word;
    }
    return status;
}

/* The two format wrappers return the converter's status even though the
 * decompiler's prototype loses the EAX result; callers branch on 1/2. */
// FUNCTION: LEGORR 0x00497d60
uint32_t crt_extended_float_convert_format_a(const uint16_t* source,
                                             uint32_t* destination)
{
    return crt_extended80_to_binary_float(source, destination,
                                          s_extended_float_format_a);
}

// FUNCTION: LEGORR 0x00497d80
uint32_t crt_extended_float_convert_format_b(const uint16_t* source,
                                             uint32_t* destination)
{
    return crt_extended80_to_binary_float(source, destination,
                                          s_extended_float_format_b);
}

/* CrtStringDuplicate (FUN_0049b660) — see crt.hpp. The corpus's length walk
 * counts bytes through the NUL, so the copy is strlen + 1 bytes. */
// FUNCTION: LEGORR 0x0049b660
char* crt_string_duplicate(const char* text)
{
    if (text == NULL) {
        return NULL; /* deviation: the corpus strides the string blindly */
    }
    const size_t n = strlen(text) + 1u;
    char* copy = (char*)engine_heap_alloc((long)n);
    if (copy != NULL) {
        memcpy(copy, text, n);
    }
    return copy; /* NULL allocation answers NULL, like the corpus */
}

/* CrtUnsignedIntegerFormatRadix (FUN_0049dba0) — see crt.hpp. The corpus's
 * digit fold is `digit < 10 ? digit + '0' : digit + 'W'`, which lands the
 * letters lowercase; the port pins that expression shape. */
// FUNCTION: LEGORR 0x0049dba0
void crt_unsigned_format_radix(uint32_t value, char* out, int radix,
                               int signed_value)
{
    if (out == NULL) {
        return; /* deviation: the corpus strides the buffer blindly */
    }
    if (radix < 2) {
        out[0] = '0';
        out[1] = '\0';
        return; /* deviation: the corpus's divide loop never terminates */
    }
    char* head = out;
    char* end;
    uint32_t v = value;
    if (signed_value != 0) {
        *out = '-';
        ++out;
        v = (uint32_t)0u - v;
        head = out;
    }
    end = out;
    for (;;) {
        const uint32_t digit = v % (uint32_t)radix;
        v /= (uint32_t)radix;
        *end++ = (char)(digit < 10u ? digit + (uint32_t)'0'
                                    : digit + (uint32_t)'W');
        if (v == 0u) {
            break;
        }
    }
    *end = '\0';
    /* Reverse in place: the corpus swaps the last digit with the buffer
     * head, then walks the ends toward each other until they cross. */
    char* tail = end - 1;
    while (head < tail) {
        const char t = *head;
        *head = *tail;
        *tail = t;
        ++head;
        --tail;
    }
}

/* SignedDecimalToStringConvert (FUN_0049db60) — see crt.hpp. */
// FUNCTION: LEGORR 0x0049db60
char* crt_signed_to_string(int value, char* out, int radix)
{
    if (radix == 10 && value < 0) {
        crt_unsigned_format_radix((uint32_t)value, out, 10, 1);
    } else {
        crt_unsigned_format_radix((uint32_t)value, out, radix, 0);
    }
    return out;
}

/* StringToUpper / StringToLowercase (FUN_0049dcd0 / FUN_0049dc10) — see
 * crt.hpp. The C-locale fast half folds ASCII letters in place; the
 * DAT_00549180 locale half rides the recorded locale-map chain. */
// GLOBAL: LEGORR 0x00549180
static uint32_t s_crt_locale_handle; /* DAT_00549180: zero-init locale handle */

// FUNCTION: LEGORR 0x0049dcd0
char* crt_string_to_upper(char* text)
{
    if (s_crt_locale_handle == 0u) {
        char* original = text;
        if (*text != '\0') {
            do {
                char* next = text + 1;
                if (*text > '`' && *text < '{') {
                    *text = (char)(*text - 0x20);
                }
                text = next;
            } while (*text != '\0');
            return original;
        }
    }
    return text;
}

// FUNCTION: LEGORR 0x0049dc10
char* crt_string_to_lower(char* text)
{
    if (s_crt_locale_handle == 0u) {
        char* original = text;
        if (*text != '\0') {
            do {
                char* next = text + 1;
                if (*text > '@' && *text < '[') {
                    *text = (char)(*text + 0x20);
                }
                text = next;
            } while (*text != '\0');
            return original;
        }
    }
    return text;
}

/* --- math error band (2026-10-25) -------------------------------------------
 * The corpus's _DAT_005490d0 math-error slot lands as guarded own storage
 * (the fd/codec readers ride the unported CRT runtime band — deviation). */
// GLOBAL: LEGORR 0x005490d0
static uint32_t s_math_error_code; /* _DAT_005490d0 role */
// GLOBAL: LEGORR 0x005490d4
static uint32_t s_win32_error_code; /* DAT_005490d4 role */

// FUNCTION: LEGORR 0x004939a0
void crt_math_error_code_set(int param)
{
    if (param != 1) {
        if (param <= 1 || param > 3) {
            return;
        }
        s_math_error_code = 0x22u;
        return;
    }
    s_math_error_code = 0x21u;
    /* other parameters: the slot stays (faithful) */
}

uint32_t crt_math_error_code_peek(void) { return s_math_error_code; }

// FUNCTION: LEGORR 0x00493be0
int fpu_status_word_get_variant_a(void)
{
    unsigned short status = 0;
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        fnstsw ax
        mov status, ax
    }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fnstsw %0" : "=a"(status));
#endif
    return (int)(short)status;
}

// FUNCTION: LEGORR 0x00493bf0
int fpu_status_word_get_variant_b(void)
{
    unsigned short status = 0;
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        fnstsw ax
        mov status, ax
    }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fnstsw %0" : "=a"(status));
#endif
    return (int)(short)status;
}

// FUNCTION: LEGORR 0x00493c10
int fpu_control_word_update_masked(uint32_t value, uint32_t mask)
{
    unsigned short old_word = 0;
    unsigned short new_word;
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm { fnstcw old_word }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fnstcw %0" : "=m"(old_word));
#endif
    new_word = (unsigned short)(((unsigned short)value & (unsigned short)mask) |
                               (old_word & (unsigned short)~mask));
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm { fldcw new_word }
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("fldcw %0" : : "m"(new_word));
#endif
    return (int)(short)old_word;
}

/* CrtMathExceptionCodeFromStatus (FUN_00493a00) — see crt.hpp. The priority
 * arms follow the decompile; the final raw low-byte return follows the
 * shipped instruction (`and eax,0xff`). */
// FUNCTION: LEGORR 0x00493a00
int crt_math_exception_code_from_status(uint32_t status)
{
    if ((status & 0x20u) != 0u) {
        return 5;
    }
    if ((status & 8u) != 0u) {
        return 1;
    }
    if ((status & 4u) != 0u) {
        return 2;
    }
    if ((status & 1u) != 0u) {
        return 3;
    }
    return (int)(status & 0xffu);
}

void global_flag_004ff978_install(uint32_t value)
{
    s_flag_004ff978 = value;
}

/* --- math handler table + 64-bit multiply thunk (2026-10-25) --------------- */

/* CrtMathExceptionHandlerGet (FUN_004939d0) — see crt.hpp. The corpus scans
 * the DAT_004b2170 table (27 stride-8 records, id dword then handler dword)
 * with an add-8 pointer walk bounded by 0x4b2248 and loads a hit through the
 * scaled index; the table is no longer zeroed own storage, the seed below
 * reproduces its image bytes. The install hook still stands in for the
 * unported CRT startup band (deviation). */
#define CRT_MATH_HANDLER_ENTRIES 27

/* The corpus's stride-8 handler table (id dword, handler dword), image-
 * populated with the CRT startup band's twenty-seven stub handlers
 * (0x49ec70..0x49ed20); seeded so the DAT's initial bytes match. */
struct CrtMathHandlerRecord {
    int      id;
    uint32_t handler;
};

// GLOBAL: LEGORR 0x004b2170
static CrtMathHandlerRecord s_math_handlers[CRT_MATH_HANDLER_ENTRIES] = {
    { 0x14, 0x49ed20u },
    { 0x1d, 0x49ed1cu },
    { 0x1a, 0x49ed18u },
    { 0x1b, 0x49ed10u },
    { 0x1f, 0x49ed08u },
    { 0x13, 0x49ed00u },
    { 0x21, 0x49ecf8u },
    { 0xe,  0x49ecf0u },
    { 0xd,  0x49ece8u },
    { 0xf,  0x49ece0u },
    { 0x10, 0x49ecd8u },
    { 5,    0x49ecd0u },
    { 0x1e, 0x49ecccu },
    { 0x12, 0x49ecc8u },
    { 0x20, 0x49ecc4u },
    { 0xc,  0x49ecbcu },
    { 0xb,  0x49ecb4u },
    { 0x15, 0x49ecacu },
    { 0x1c, 0x49eca4u },
    { 0x19, 0x49ec9cu },
    { 0x11, 0x49ec94u },
    { 0x18, 0x49ec8cu },
    { 0x16, 0x49ec84u },
    { 0x17, 0x49ec7cu },
    { 0x22, 0x49ec78u },
    { 0x23, 0x49ec74u },
    { 0x24, 0x49ec70u }
};

// FUNCTION: LEGORR 0x004939d0
uint32_t crt_math_exception_handler_get(int id)
{
    /* Corpus walk shape: the counter clears before the walk seeds, the
     * add-8 pointer bounds the scan against the table end through a
     * SIGNED (long) compare so MSVC emits jl, and the hit loads back
     * through the hop counter's scaled index. */
    int i = 0;
    const CrtMathHandlerRecord* p = s_math_handlers;
    while ((long)p < (long)(s_math_handlers + CRT_MATH_HANDLER_ENTRIES)) {
        if (p->id == id) {
            return s_math_handlers[i].handler;
        }
        ++p;
        ++i;
    }
    return 0u;
}

void crt_math_exception_handler_install(int index, int id, uint32_t handler)
{
    if (index >= 0 && index < CRT_MATH_HANDLER_ENTRIES) {
        s_math_handlers[index].id = id;
        s_math_handlers[index].handler = handler;
    }
}

/* CrtInteger64Multiply (FUN_0048ea90, __allmul) — see crt.hpp. The two
 * cross-word products contribute only their low 32 bits (the bit pattern
 * is sign-agnostic), so the fold lands as plain wrapping 32-bit adds.
 * The winapi branch is a byte-faithful naked reproduction of the corpus's
 * unsigned mul sequence, including the fast-path skip: when both high
 * words are zero the cross terms are never computed (the `or`/`jne`
 * test at the top); the portable branch keeps the same three-way fold
 * in C. */
// FUNCTION: LEGORR 0x0048ea90
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) uint64_t crt_integer64_multiply(uint32_t lo_a, int32_t hi_a,
                                                  uint32_t lo_b, int32_t hi_b)
{
    __asm {
        mov eax, [esp + 8]
        mov ecx, [esp + 0x10]
        or ecx, eax
        mov ecx, [esp + 0xc]
        jne cross_terms
        mov eax, [esp + 4]
        mul ecx
        ret 0x10
    cross_terms:
        push ebx
        mul ecx
        mov ebx, eax
        mov eax, [esp + 8]
        mul dword ptr [esp + 0x14]
        add ebx, eax
        mov eax, [esp + 8]
        mul ecx
        add edx, ebx
        pop ebx
        ret 0x10
    }
}
#else
uint64_t crt_integer64_multiply(uint32_t lo_a, int32_t hi_a,
                                uint32_t lo_b, int32_t hi_b)
{
    const uint64_t low64 = (uint64_t)lo_a * (uint64_t)lo_b;
    if (hi_a == 0 && hi_b == 0) {
        return low64;
    }
    const uint32_t high32 = (uint32_t)(low64 >> 32)
                          + (uint32_t)((uint32_t)(uint32_t)hi_a * lo_b)
                          + (uint32_t)(lo_a * (uint32_t)(uint32_t)hi_b);
    return ((uint64_t)high32 << 32) | (uint32_t)low64;
}
#endif

/* CrtInteger64Divide (FUN_0048e9e0, __alldiv) — see crt.hpp. Preserve the
 * original two-DWORD sign normalization, trial quotient and multiply-back
 * correction. Keeping the divisions unsigned avoids signed overflow for
 * INT64_MIN / -1 and matches the helper's wrapping word-pair result. */
// FUNCTION: LEGORR 0x0048e9e0
uint64_t crt_integer64_divide(uint32_t lo_a, uint32_t hi_a,
                              uint32_t lo_b, uint32_t hi_b)
{
    uint32_t negative = 0;
    uint32_t dividend_lo = lo_a;
    uint32_t dividend_hi = hi_a;
    uint32_t divisor_lo = lo_b;
    uint32_t divisor_hi = hi_b;
    uint32_t quotient_lo;
    uint32_t quotient_hi;

    if ((int32_t)dividend_hi < 0) {
        uint32_t borrow = dividend_lo != 0;
        dividend_lo = 0u - dividend_lo;
        dividend_hi = 0u - borrow - dividend_hi;
        negative++;
    }
    if ((int32_t)divisor_hi < 0) {
        uint32_t borrow = divisor_lo != 0;
        divisor_lo = 0u - divisor_lo;
        divisor_hi = 0u - borrow - divisor_hi;
        negative++;
    }

    if (divisor_hi == 0) {
        uint32_t remainder;
        quotient_hi = dividend_hi / divisor_lo;
        remainder = dividend_hi % divisor_lo;
        quotient_lo = (uint32_t)((((uint64_t)remainder << 32) | dividend_lo)
                                 / divisor_lo);
    }
    else {
        uint32_t shifted_dividend_lo = dividend_lo;
        uint32_t shifted_dividend_hi = dividend_hi;
        uint32_t shifted_divisor_lo = divisor_lo;
        uint32_t shifted_divisor_hi = divisor_hi;
        uint64_t trial_product;
        uint32_t trial_quotient;
        uint32_t product_hi;
        uint32_t product_lo;
        uint32_t product_overflow;

        while (shifted_divisor_hi != 0) {
            uint32_t divisor_carry = shifted_divisor_hi & 1;
            uint32_t dividend_carry = shifted_dividend_hi & 1;
            shifted_divisor_hi >>= 1;
            shifted_divisor_lo = (shifted_divisor_lo >> 1)
                               | (divisor_carry << 31);
            shifted_dividend_hi >>= 1;
            shifted_dividend_lo = (shifted_dividend_lo >> 1)
                                | (dividend_carry << 31);
        }

        trial_quotient = (uint32_t)((((uint64_t)shifted_dividend_hi << 32)
                                    | shifted_dividend_lo)
                                   / shifted_divisor_lo);
        trial_product = (uint64_t)divisor_lo * trial_quotient;
        product_lo = (uint32_t)trial_product;
        product_hi = (uint32_t)(trial_product >> 32);
        product_overflow = product_hi + trial_quotient * divisor_hi;
        if (product_overflow < product_hi || dividend_hi < product_overflow
            || (dividend_hi == product_overflow && dividend_lo < product_lo)) {
            trial_quotient--;
        }
        quotient_lo = trial_quotient;
        quotient_hi = 0;
    }

    if (negative == 1) {
        uint32_t borrow = quotient_lo != 0;
        quotient_lo = 0u - quotient_lo;
        quotient_hi = 0u - borrow - quotient_hi;
    }
    return ((uint64_t)quotient_hi << 32) | quotient_lo;
}

/* CrtUnsigned64Divide (FUN_00498bf0, __aulldiv) — see crt.hpp. The
 * unsigned sibling: the same shift/trial/repair shape, so the same fold;
 * the high half of the answer is pinned zero only when the repair needs
 * it (the estimate can overflow the 32-bit trial), never by construction. */
// FUNCTION: LEGORR 0x00498bf0
uint64_t crt_unsigned64_divide(uint32_t lo_a, uint32_t hi_a,
                              uint32_t lo_b, uint32_t hi_b)
{
    const uint64_t n = ((uint64_t)hi_a << 32) | lo_a;
    const uint64_t d = ((uint64_t)hi_b << 32) | lo_b;
    return n / d;
}

// GLOBAL: LEGORR 0x004afd72
static uint16_t s_ctype_character_flags[256] = {
    0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x0020u, 0x0028u, 0x0028u, 0x0028u, 0x0028u, 0x0028u, 0x0020u, 0x0020u,
    0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x0048u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u, 0x0084u,
    0x0084u, 0x0084u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0010u, 0x0081u, 0x0081u, 0x0081u, 0x0081u, 0x0081u, 0x0081u, 0x0001u,
    0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u,
    0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u, 0x0001u,
    0x0001u, 0x0001u, 0x0001u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0010u,
    0x0010u, 0x0082u, 0x0082u, 0x0082u, 0x0082u, 0x0082u, 0x0082u, 0x0002u,
    0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u,
    0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u, 0x0002u,
    0x0002u, 0x0002u, 0x0002u, 0x0010u, 0x0010u, 0x0010u, 0x0010u, 0x0020u
};

// GLOBAL: LEGORR 0x004afd68
static uint16_t* volatile s_ctype_pointer = s_ctype_character_flags;
// GLOBAL: LEGORR 0x005492b8
static uint8_t s_multibyte_character_flags[257];
// GLOBAL: LEGORR 0x005493bc
static uint32_t s_active_codepage;
// GLOBAL: LEGORR 0x005493c0
static uint32_t s_active_locale_table;
// GLOBAL: LEGORR 0x005493c8
static uint32_t s_codepage_metadata_word0;
// GLOBAL: LEGORR 0x005493cc
static uint32_t s_codepage_metadata_word1;
// GLOBAL: LEGORR 0x005493d0
static uint32_t s_codepage_metadata_word2;
// GLOBAL: LEGORR 0x004aff74
static volatile uint32_t s_max_multibyte_width = 1u;

/* CrtCodePageLocaleIdLookup (FUN_00496530): the four DBCS pages with
 * built-in Windows locale mappings; all other pages have no mapped LCID. */
// FUNCTION: LEGORR 0x00496530
uint32_t crt_codepage_locale_id_lookup(uint32_t codepage)
{
    switch (codepage) {
    case 0x3a4u: return 0x411u;
    case 0x3a8u: return 0x804u;
    case 0x3b5u: return 0x412u;
    case 0x3b6u: return 0x404u;
    default: return 0u;
    }
}

/* CrtCtypeTablesReset (FUN_00496590): reset only the lead-byte flag table
 * and the active code-page conversion metadata. The separate _pctype table
 * and the GetACP/GetOEMCP fallback flag are left as the image leaves them. */
// FUNCTION: LEGORR 0x00496590
void crt_ctype_tables_reset(void)
{
    memset(s_multibyte_character_flags, 0, sizeof s_multibyte_character_flags);
    s_active_codepage = 0u;
    s_active_locale_table = 0u;
    s_codepage_metadata_word0 = 0u;
    s_codepage_metadata_word1 = 0u;
    s_codepage_metadata_word2 = 0u;
}

/* CrtScanFormatCharNormalize (FUN_00494c30) — see crt.hpp. The image's
 * classification table marks decimal digits with bit 4; non-digits take
 * the original ASCII hex-letter fold. */
// FUNCTION: LEGORR 0x00494c30
uint32_t crt_scan_format_char_normalize(uint32_t value)
{
    uint32_t digit;
    if (s_max_multibyte_width < 2u) {
        digit = (value < 256u) ? (s_ctype_pointer[value] & 4u) : 0u;
    }
    else {
        digit = crt_character_class_flag_test(value, 4u, 0u) ? 4u : 0u;
    }
    if (digit != 0u) {
        return value;
    }
    return (value & 0xffffffdfu) - 7u;
}

/* CrtCharacterClassFlagTest (FUN_00495db0): the first flag source is the
 * current multibyte table; if it does not match, test the ordinary CRT
 * classification word only when a word mask was requested. */
// FUNCTION: LEGORR 0x00495db0
uint32_t crt_character_class_flag_test(uint32_t value, uint32_t flags,
                                      uint8_t byte_mask)
{
    const uint8_t character = (uint8_t)value;
    if ((s_multibyte_character_flags[(unsigned)character + 1u]
         & byte_mask) != 0u) {
        return 1u;
    }
    if (flags != 0u
        && (s_ctype_pointer[character] & (uint16_t)flags) != 0u) {
        return 1u;
    }
    return 0u;
}

// FUNCTION: LEGORR 0x00495d90
uint32_t crt_is_dbcs_lead_byte(uint32_t value)
{
    return crt_character_class_flag_test(value, 0u, 4u);
}

/* CrtMultibyteCharacterSearch (FUN_0049b5b0): when no multibyte code page
 * is active, this is the CRT's byte strchr path. Otherwise step over lead
 * and trail bytes together and match the original high-byte-first pair. */
// FUNCTION: LEGORR 0x0049b5b0
char* crt_multibyte_character_search(char* text, uint32_t value)
{
    uint8_t* cursor = (uint8_t*)text;
    uint32_t character;

    if (s_active_codepage == 0u) {
        return (char*)strchr(text, (int)value);
    }
    character = *cursor;
    if (character == 0u) {
        return (value == 0u) ? (char*)cursor : NULL;
    }
    for (;;) {
        uint8_t* last_byte;
        if ((s_multibyte_character_flags[character + 1u] & 4u) == 0u) {
            last_byte = cursor;
            if (value == character) {
                return (char*)cursor;
            }
        }
        else {
            last_byte = cursor + 1;
            if (cursor[1] == 0u) {
                return NULL;
            }
            if (value == ((character << 8) | cursor[1])) {
                return (char*)cursor;
            }
        }
        character = last_byte[1];
        cursor = last_byte + 1;
        if (character == 0u) {
            return (value == 0u) ? (char*)cursor : NULL;
        }
    }
}

/* CrtEnvironmentArrayClone (FUN_0049b540): allocate one extra pointer for
 * the terminator, then duplicate each string in order. The port routes the
 * corpus's __amsg_exit(9) allocation failure through its fatal-exit seam. */
// FUNCTION: LEGORR 0x0049b540
char** crt_environment_array_clone(char** entries)
{
    int count = 0;
    int i;
    char** copy;

    if (entries == NULL) {
        return NULL;
    }
    while (entries[count] != NULL) {
        ++count;
    }
    copy = (char**)engine_heap_alloc((long)((count + 1) * sizeof(char*)));
    if (copy == NULL) {
        crt_fatal_message_exit();
        return NULL;
    }
    for (i = 0; i < count; ++i) {
        copy[i] = crt_string_duplicate(entries[i]);
    }
    copy[count] = NULL;
    return copy;
}

/* CrtCommandLineArgumentScan (FUN_00495f80): the CRT's two-pass command-line
 * parser. Counts include the final null pointer and each argument's NUL byte,
 * matching the allocation sizes consumed by CrtCommandLineArgumentsInit. */
// FUNCTION: LEGORR 0x00495f80
void crt_command_line_argument_scan(const unsigned char* command_line,
                                    char** argv_out,
                                    unsigned char* string_out,
                                    int* argc_and_null_count,
                                    int* string_byte_count)
{
    const unsigned char* cursor = command_line;
    unsigned char* output = string_out;
    char** argv = argv_out;
    int argc_count = 1;
    int byte_count = 0;

    if (argv != NULL) {
        *argv++ = (char*)output;
    }

    /* argv[0] follows a separate rule: quotes only have meaning when the
     * first byte is a quote, and otherwise the name ends at whitespace. */
    if (*cursor == '"') {
        ++cursor;
        while (*cursor != '"' && *cursor != 0u) {
            if (crt_character_class_flag_test(*cursor, 0u, 4u) != 0u) {
                if (output != NULL) {
                    *output++ = *cursor;
                }
                ++cursor;
                ++byte_count;
            }
            if (output != NULL) {
                *output++ = *cursor;
            }
            ++cursor;
            ++byte_count;
        }
        if (*cursor == '"') {
            ++cursor;
        }
    }
    else {
        while (*cursor != 0u && *cursor != ' ' && *cursor != '\t') {
            const unsigned char ch = *cursor;
            if (output != NULL) {
                *output++ = ch;
            }
            ++cursor;
            ++byte_count;
            if (crt_character_class_flag_test(ch, 0u, 4u) != 0u
                && *cursor != 0u) {
                if (output != NULL) {
                    *output++ = *cursor;
                }
                ++cursor;
                ++byte_count;
            }
        }
    }
    if (output != NULL) {
        *output++ = 0u;
    }
    ++byte_count;

    while (*cursor != 0u) {
        int in_quotes = 0;
        int quote_state = 0;
        while (*cursor == ' ' || *cursor == '\t') {
            ++cursor;
        }
        if (*cursor == 0u) {
            break;
        }
        if (argv != NULL) {
            *argv++ = (char*)output;
        }
        ++argc_count;

        for (;;) {
            unsigned int slash_count = 0u;
            int copy_character = 1;
            unsigned char ch;

            while (*cursor == '\\') {
                ++slash_count;
                ++cursor;
            }
            if (*cursor == '"') {
                if ((slash_count & 1u) == 0u) {
                    if (in_quotes && cursor[1] == '"') {
                        ++cursor;
                    }
                    else {
                        copy_character = 0;
                    }
                    in_quotes = !quote_state;
                    quote_state = in_quotes;
                }
                slash_count >>= 1;
            }
            while (slash_count != 0u) {
                if (output != NULL) {
                    *output++ = '\\';
                }
                ++byte_count;
                --slash_count;
            }

            ch = *cursor;
            if (ch == 0u || (!in_quotes && (ch == ' ' || ch == '\t'))) {
                break;
            }
            if (copy_character) {
                if (crt_character_class_flag_test(ch, 0u, 4u) != 0u) {
                    if (output != NULL) {
                        *output++ = ch;
                    }
                    ++byte_count;
                    ++cursor;
                    if (output != NULL) {
                        *output++ = *cursor;
                    }
                    ++byte_count;
                    if (*cursor != 0u) {
                        ++cursor;
                    }
                }
                else {
                    if (output != NULL) {
                        *output++ = ch;
                    }
                    ++byte_count;
                    ++cursor;
                }
            }
            else {
                ++cursor;
            }
        }
        if (output != NULL) {
            *output++ = 0u;
        }
        ++byte_count;
    }

    if (argv != NULL) {
        *argv = NULL;
    }
    ++argc_count;
    *argc_and_null_count = argc_count;
    *string_byte_count = byte_count;
}

/* CrtDecimalMantissaRoundAndNormalize (FUN_00497e20). The decimal record is
 * the locale-conversion cache shape: the exponent is its second DWORD, while
 * the final field points at the generated digit string. Native-width pointer
 * fields keep this record usable on the portable 64-bit backend. */
// FUNCTION: LEGORR 0x00497e20
void crt_decimal_mantissa_round_and_normalize(char* output, int precision,
                                               CrtDecimalFormatContext* context)
{
    char* source = context->digits;
    char* destination = output + 1;
    int remaining = precision;
    int count = precision;

    *output = '0';
    while (count > 0) {
        char digit = *source;
        if (digit == '\0') {
            digit = '0';
        }
        else {
            ++source;
        }
        *destination++ = digit;
        --count;
        --remaining;
    }
    *destination = '\0';

    if (remaining >= 0 && *source > '4') {
        char* carry = destination - 1;
        while (*carry == '9') {
            *carry = '0';
            --carry;
        }
        *carry = (char)(*carry + 1);
    }

    if (*output == '1') {
        ++context->exponent;
        return;
    }

    /* Discard the guard digit at output[0] when rounding did not carry into
     * it. The source range includes the terminator. */
    {
        char* src = output + 1;
        char* dst = output;
        do {
            *dst++ = *src;
        } while (*src++ != '\0');
    }
}

/* --- the CRT file-descriptor flag window (2026-10-25) ---------------------
 * CrtFileDescriptorFlag40Get (FUN_00498b40): the corpus reads bit 0x40 of
 * the byte at block+4 in its file-descriptor table — 32 eight-byte entries
 * per block, blocks indexed by one of the 64 pointers at DAT_0076ddc0. The
 * bound word at DAT_0076dec0 follows that 0x100-byte pointer table. This
 * table is CRT platform state with no ported owner: the pointer blocks and
 * count word land as zero-init own storage behind install/peek hooks (the
 * unowned-global convention).
 * Deviations: the block window and null blocks are guarded (the corpus
 * strides), and a negative descriptor answers 0. */
#define CRT_FD_BLOCKS 64
// GLOBAL: LEGORR 0x0076ddc0
static void* s_fd_flag_blocks[CRT_FD_BLOCKS]; /* DAT_0076ddc0 role */
// GLOBAL: LEGORR 0x0076dec0
static int   s_fd_flag_count;                 /* DAT_0076dec0 role */

// FUNCTION: LEGORR 0x00498b40
unsigned crt_file_descriptor_flag40_get(unsigned fd)
{
    const int descriptor = (int)fd;
    if (fd >= (unsigned)s_fd_flag_count) {
        return 0;
    }
    const unsigned char* block =
        (const unsigned char*)s_fd_flag_blocks[descriptor >> 5];
    return (unsigned)(block[(descriptor & 0x1f) * 8 + 4] & 0x40u);
}

// FUNCTION: LEGORR 0x004991f0
int crt_file_descriptor_handle_get(unsigned fd)
{
    const int descriptor = (int)fd;
    if (fd < (unsigned)s_fd_flag_count) {
        const unsigned char* entry =
            (const unsigned char*)s_fd_flag_blocks[descriptor >> 5] +
            (descriptor & 0x1f) * 8;
        if ((entry[4] & 1u) != 0u) {
            return *(const int*)entry;
        }
    }
    s_math_error_code = 9u;
    s_win32_error_code = 0u;
    return -1;
}

/* CrtFileDescriptorAllocate (FUN_00498fe0): scan 32-entry blocks in the
 * 64-slot pointer table. A new block starts with inactive handles, zero
 * flags and the CRT's default mode byte 10. */
// FUNCTION: LEGORR 0x00498fe0
int crt_file_descriptor_allocate(void)
{
    int block_index;
    for (block_index = 0; block_index < CRT_FD_BLOCKS; ++block_index) {
        unsigned char* block = (unsigned char*)s_fd_flag_blocks[block_index];
        if (block == NULL) {
            int slot;
            block = (unsigned char*)engine_heap_alloc(0x100);
            if (block == NULL) {
                return -1;
            }
            s_fd_flag_count += 0x20;
            s_fd_flag_blocks[block_index] = block;
            for (slot = 0; slot < 0x20; ++slot) {
                unsigned char* entry = block + slot * 8;
                *(int*)entry = -1;
                entry[4] = 0;
                entry[5] = 10;
            }
            return block_index << 5;
        }
        else {
            int slot;
            for (slot = 0; slot < 0x20; ++slot) {
                unsigned char* entry = block + slot * 8;
                if ((entry[4] & 1u) == 0u) {
                    *(int*)entry = -1;
                    return (block_index << 5) + slot;
                }
            }
        }
    }
    return -1;
}

/* CrtFileDescriptorSetHandle (FUN_004990a0): install a handle only into an
 * allocated descriptor whose handle sentinel is -1. The image's standard
 * handle synchronization mode is initialized to 2 and has no writer, so
 * its conditional Win32 synchronization path is dormant in this build. */
// FUNCTION: LEGORR 0x004990a0
int crt_file_descriptor_set_handle(unsigned fd, int handle)
{
    if (fd < (unsigned)s_fd_flag_count) {
        unsigned char* entry =
            (unsigned char*)s_fd_flag_blocks[fd >> 5] + (fd & 0x1fu) * 8u;
        if (*(int*)entry == -1) {
            *(int*)entry = handle;
            return 0;
        }
    }
    s_math_error_code = 9u;
    s_win32_error_code = 0u;
    return -1;
}

/* CrtFileDescriptorCloseSlot (FUN_00499150): invalidate an open slot's
 * handle while retaining its descriptor flags for the next CRT operation. */
// FUNCTION: LEGORR 0x00499150
int crt_file_descriptor_close_slot(unsigned fd)
{
    if (fd < (unsigned)s_fd_flag_count) {
        unsigned char* entry =
            (unsigned char*)s_fd_flag_blocks[fd >> 5] + (fd & 0x1fu) * 8u;
        if ((entry[4] & 1u) != 0u && *(int*)entry != -1) {
            *(int*)entry = -1;
            return 0;
        }
    }
    s_math_error_code = 9u;
    s_win32_error_code = 0u;
    return -1;
}

// FUNCTION: LEGORR 0x0049aea0
int crt_file_descriptor_flag80_set_or_clear(unsigned fd, int mode)
{
    if (fd < (unsigned)s_fd_flag_count && (fd >> 5) < CRT_FD_BLOCKS) {
        unsigned char* block = (unsigned char*)s_fd_flag_blocks[fd >> 5];
        if (block != NULL) {
            unsigned char* flags = block + (fd & 0x1fu) * 8u + 4u;
            const unsigned char old_flags = *flags;
            if ((old_flags & 1u) != 0u) {
                if (mode == 0x8000) {
                    *flags = (unsigned char)(old_flags & 0x7fu);
                } else if (mode == 0x4000) {
                    *flags = (unsigned char)(old_flags | 0x80u);
                } else {
                    s_math_error_code = 0x16u;
                    return -1;
                }
                return (old_flags & 0x80u) != 0u ? 0x4000 : 0x8000;
            }
        }
    }
    s_math_error_code = 9u;
    return -1;
}

void crt_file_descriptor_flag_block_set(int index, void* block)
{
    if (index >= 0 && index < CRT_FD_BLOCKS) {
        s_fd_flag_blocks[index] = block;
    }
}

void* crt_file_descriptor_flag_block_peek(int index)
{
    if (index < 0 || index >= CRT_FD_BLOCKS) {
        return NULL;
    }
    return s_fd_flag_blocks[index];
}

void crt_file_descriptor_count_set(int count)
{
    s_fd_flag_count = count;
}

int crt_file_descriptor_count_peek(void)
{
    return s_fd_flag_count;
}

/* --- the CRT control-flags / signal leaves (2026-10-25) --------------------
 * CrtControlFlagsTranslate (FUN_004977d0): the corpus's flag fold — bits
 * 0/2/3/4/5 cross over to 0x10/8/4/2/1 (bit 1 pairs back as 0x80000), the
 * bits 10/11 pair folds to 0x100/0x200/0x300 (zero stays), an unset bits
 * 8+9 pair answers 0x20000 (only the 0x200 pair answers 0x10000), and
 * bit 12 answers 0x40000. Pure bitwork.
 *
 * CrtWideCharToMultibyte (FUN_00498b70): the corpus converts one wide
 * char through the code-page API, its C-locale half (DAT_00549180 == 0,
 * the port's pinned locale per KeyToAscii) writing the low byte for any
 * char below 0x100. Deviations: the pinned-locale half leaves the
 * code-page branch unreachable (the unported locale-map chain), and the
 * failure half stores the corpus's EILSEQ-shaped 0x2a into the modeled
 * math-error slot (_DAT_005490d0) and answers -1. */
// FUNCTION: LEGORR 0x004977d0
uint32_t crt_control_flags_translate(uint32_t flags)
{
    uint32_t out = 0u;
    if ((flags & 0x01u) != 0u) {
        out = 0x10u;
    }
    if ((flags & 0x04u) != 0u) { out |= 0x08u; }
    if ((flags & 0x08u) != 0u) { out |= 0x04u; }
    if ((flags & 0x10u) != 0u) { out |= 0x02u; }
    if ((flags & 0x20u) != 0u) { out |= 0x01u; }
    if ((flags & 0x02u) != 0u) { out |= 0x80000u; }
    /* corpus: the low word is masked once and both pair tests ride it */
    const uint32_t low16 = flags & 0xffffu;
    const uint32_t pair2 = low16 & 0xc00u;
    if (pair2 == 0x400u) {
        out |= 0x100u;
    } else if (pair2 == 0x800u) {
        out |= 0x200u;
    } else if (pair2 == 0xc00u) {
        out |= 0x300u;
    }
    if ((low16 & 0x300u) != 0u) {
        if ((low16 & 0x300u) == 0x200u) {
            out |= 0x10000u;
        }
    } else {
        out |= 0x20000u;
    }
    if ((flags & 0x1000u) != 0u) {
        out |= 0x40000u;
    }
    return out;
}

// FUNCTION: LEGORR 0x00497770
uint32_t crt_fpu_control_word_update(uint32_t value, uint32_t mask)
{
    const uint32_t translated = crt_control_flags_translate(value);
    const uint32_t result = (mask & value) | (~mask & translated);
    crt_no_op_stub_variant();
    return result;
}

// FUNCTION: LEGORR 0x004977b0
void crt_fpu_control_word_update_mask(uint32_t value, uint32_t mask)
{
    crt_fpu_control_word_update(value, mask & 0xfff7ffffu);
}

// FUNCTION: LEGORR 0x00498b70
int crt_wide_char_to_multibyte(char* dest, uint32_t wide)
{
    if (dest == NULL) {
        return 0; /* pinned: the corpus's null check answers 0 */
    }
    const uint16_t ch = (uint16_t)wide;
    if (ch <= 0xffu) {
        *dest = (char)ch;
        return 1;
    }
    s_math_error_code = 0x2au; /* pinned: the corpus's EILSEQ-shaped store */
    return -1;
}

/* CrtStrncpy (library `_strncpy`, FUN_0048e770) — see crt.hpp. The corpus's
 * aligned-dword fast path is an implementation detail: every lane copies
 * source bytes up to the count, copies the NUL when it lands, and pads the
 * rest of the window with zeros. The fold is that observable behavior;
 * source reads stay inside the count. */
// FUNCTION: LEGORR 0x0048e770
char* crt_string_copy_n(char* dest, const char* src, uint32_t count)
{
    if (count == 0u) {
        return dest;
    }
    uint32_t i = 0u;
    for (; i < count; ++i) {
        const char c = src[i];
        dest[i] = c;
        if (c == '\0') {
            ++i;
            break;
        }
    }
    for (; i < count; ++i) {
        dest[i] = 0;
    }
    return dest;
}

/* CrtWideCharToMultiByteCompat (FUN_0049abe0) — see crt.hpp. Pinned to the
 * port's C-locale half (the DAT_00549180 locale half rides the recorded
 * locale-map chain, as in the string-case and codepage leaves): the
 * dest-NULL lane answers the wide length, and the store lane copies low
 * bytes until the NUL or the count. The wide-over-0xff exit takes the
 * corpus's EILSEQ-shaped math-error store and answers -1. */
// FUNCTION: LEGORR 0x0049abe0
int crt_wide_char_to_multi_byte_compat(char* dest, const uint16_t* wide,
                                       uint32_t count)
{
    if (dest != NULL && count == 0u) {
        return 0;
    }
    if (dest == NULL) {
        return wide_string_length(wide);
    }
    uint32_t i = 0u;
    for (;;) {
        const uint32_t ch = wide[i];
        if (ch >= 0x100u) {
            s_math_error_code = 0x2au;
            return -1;
        }
        dest[i] = (char)ch;
        if (ch == 0u) {
            return (int)i;
        }
        ++i;
        if (count <= i) {
            return (int)i;
        }
    }
}

/* SignalHandlerRecordFind (FUN_0049ab90): linear search of the corpus's
 * signal-handler records — 12-byte entries (word, id, handler) in the
 * DAT_004b22a8 table, bounded by the DAT_004b2328 count. The table's
 * writers sit in the unported CRT startup band: own storage behind the
 * install hooks below (the unowned-global convention). Deviations: the
 * count-bounded scan (the corpus's end check reads the table through)
 * and the LP64 16-byte record stride (the corpus's x86 12 bytes). */
#define CRT_SIGNAL_RECORDS 10
struct crt_signal_record {
    uint32_t  word0;
    int32_t   id;
    uintptr_t handler;
};
// GLOBAL: LEGORR 0x004b22a8
static struct crt_signal_record s_signal_records[CRT_SIGNAL_RECORDS] = {
    { 0xc0000005u, 11, 0u },
    { 0xc000001du, 4, 0u },
    { 0xc0000096u, 4, 0u },
    { 0xc000008du, 8, 0u },
    { 0xc000008eu, 8, 0u },
    { 0xc000008fu, 8, 0u },
    { 0xc0000090u, 8, 0u },
    { 0xc0000091u, 8, 0u },
    { 0xc0000092u, 8, 0u },
    { 0xc0000093u, 8, 0u }
};
// GLOBAL: LEGORR 0x004b2328
static int s_signal_record_count = CRT_SIGNAL_RECORDS;

// FUNCTION: LEGORR 0x0049ab90
void* crt_signal_handler_record_find(int signal)
{
    struct crt_signal_record* found = &s_signal_records[0];
    struct crt_signal_record* end = found + s_signal_record_count;
    if (found->id != signal) {
        struct crt_signal_record* previous = found;
        do {
            found = previous + 1;
            if (end <= found) {
                break;
            }
            previous = found;
        } while (previous->id != signal);
    }
    if (end <= found || found->id != signal) {
        return NULL;
    }
    return found;
}

void crt_signal_record_install(int index, uint32_t word0, int signal,
                               const void* handler)
{
    if (index < 0 || index >= CRT_SIGNAL_RECORDS) {
        return; /* deviation: the corpus strides the table */
    }
    s_signal_records[index].word0 = word0;
    s_signal_records[index].id = signal;
    s_signal_records[index].handler = (uintptr_t)handler;
}

void crt_signal_record_peek(int index, uint32_t* out_word0, int* out_id,
                            const void** out_handler)
{
    if (index < 0 || index >= CRT_SIGNAL_RECORDS) {
        return; /* deviation: the corpus strides the table */
    }
    if (out_word0 != NULL) {
        *out_word0 = s_signal_records[index].word0;
    }
    if (out_id != NULL) {
        *out_id = s_signal_records[index].id;
    }
    if (out_handler != NULL) {
        *out_handler = (const void*)s_signal_records[index].handler;
    }
}

void crt_signal_record_count_set(int count)
{
    s_signal_record_count = count > CRT_SIGNAL_RECORDS
                                ? CRT_SIGNAL_RECORDS
                                : (count < 0 ? 0 : count);
}

int crt_signal_record_count_peek(void)
{
    return s_signal_record_count;
}

/* DoubleFrexpDecompose (FUN_00493ae0): normalize the stored significand to
 * the corpus's exponent 0x3fe, then report the exponent such that
 * input = result * 2^exponent. Subnormals are shifted until bit 52 is set. */
// FUNCTION: LEGORR 0x00493ae0
double double_frexp_decompose(uint32_t low_word, uint32_t high_word,
                              int* exponent)
{
    union {
        uint64_t bits;
        double value;
    } input, output;
    uint32_t exponent_bits;
    uint64_t fraction;
    int result_exponent;

    input.bits = ((uint64_t)high_word << 32) | low_word;
    exponent_bits = (high_word >> 20) & 0x7ffu;
    fraction = input.bits & CRT_U64(0x000fffffffffffff);
    if (exponent_bits == 0u && fraction == 0u) {
        *exponent = 0;
        return 0.0;
    }
    if (exponent_bits == 0u) {
        result_exponent = -1021;
        while ((fraction & CRT_U64(0x0010000000000000)) == 0u) {
            fraction <<= 1;
            --result_exponent;
        }
        fraction &= CRT_U64(0x000fffffffffffff);
        output.bits = ((uint64_t)(high_word & 0x80000000u) << 32) |
                      ((uint64_t)0x3feu << 52) | fraction;
    }
    else {
        result_exponent = (int)exponent_bits - 0x3fe;
        output.bits = (input.bits & CRT_U64(0x800fffffffffffff)) |
                      ((uint64_t)0x3feu << 52);
    }
    *exponent = result_exponent;
    return output.value;
}

/* CrtMathExceptionDispatch (FUN_004938f0) — see crt.hpp. The registered
 * image handlers currently resolve to the port's FLH no-op, so a found
 * handler follows the corpus's unhandled result/error-code branch. */
// FUNCTION: LEGORR 0x004938f0
double crt_math_exception_dispatch(uint32_t error_code, uint32_t operation,
                                   uint32_t argument1_low,
                                   uint32_t argument1_high,
                                   uint32_t argument2_low,
                                   uint32_t argument2_high,
                                   uint32_t result_low,
                                   uint32_t result_high,
                                   uint32_t fpu_control_word)
{
    union {
        uint64_t bits;
        double value;
    } result;
    uint32_t handler;
    int handled;

    handler = crt_math_exception_handler_get((int)operation);
    if (handler != 0u) {
        handled = (int)flh_magic_uu_noop();
    }
    else {
        handled = 0;
    }
    (void)fpu_control_word_update_masked(fpu_control_word, 0xffffu);
    if (handled == 0) {
        crt_math_error_code_set((int)error_code);
    }
    result.bits = ((uint64_t)result_high << 32) | result_low;
    return result.value;
}

/* CrtDoubleToIntegerRound (FUN_004935c0): masked status bits are cleared;
 * integer conversion truncates toward zero, and overflow saturates according
 * to the saved x87 rounding mode. */
// FUNCTION: LEGORR 0x004935c0
int crt_double_to_integer_round(uint32_t status, double* value,
                                uint32_t fpu_control_word)
{
    union {
        double value;
        uint64_t bits;
    } number;
    uint32_t remaining = status & 0x1fu;
    uint32_t rounding_mode;

    number.value = *value;
    if ((remaining & 8u) != 0u && (fpu_control_word & 1u) != 0u) {
        crt_no_op_stub_variant2();
        remaining &= ~8u;
    }
    if ((remaining & 4u) != 0u && (fpu_control_word & 4u) != 0u) {
        crt_no_op_stub_variant2();
        remaining &= ~4u;
    }
    if ((remaining & 1u) != 0u) {
        if ((fpu_control_word & 8u) != 0u) {
            crt_no_op_stub_variant2();
            remaining &= ~1u;
        }
        else {
            union {
                uint64_t bits;
                double value;
            } saturation;
            int negative = (number.bits >> 63) != 0u;
            rounding_mode = fpu_control_word & 0x0c00u;
            if (rounding_mode == 0x0400u) {
                if (number.value <= 0.0) {
                    saturation.bits = CRT_U64(0xfff0000000000000);
                }
                else {
                    saturation.bits = CRT_U64(0x7fefffffffffffff);
                }
            }
            else if (rounding_mode == 0x0800u) {
                if (number.value > 0.0) {
                    saturation.bits = CRT_U64(0x7ff0000000000000);
                }
                else {
                    saturation.bits = CRT_U64(0xffefffffffffffff);
                }
            }
            else if (rounding_mode == 0x0c00u) {
                saturation.bits = negative ? CRT_U64(0xffefffffffffffff)
                                           : CRT_U64(0x7fefffffffffffff);
            }
            else {
                saturation.bits = negative ? CRT_U64(0xfff0000000000000)
                                           : CRT_U64(0x7ff0000000000000);
            }
            *value = saturation.value;
            remaining &= ~1u;
        }
    }
    if ((remaining & 2u) != 0u && (fpu_control_word & 0x10u) != 0u) {
        uint32_t exponent_bits = (uint32_t)((number.bits >> 52) & 0x7ffu);
        int binary_exponent = (int)exponent_bits - 1023;
        uint64_t discarded_mask = 0u;

        if (exponent_bits == 0u || binary_exponent < 0) {
            if ((number.bits & CRT_U64(0x7fffffffffffffff)) != 0u) {
                crt_no_op_stub_variant2();
            }
            number.bits &= CRT_U64(0x8000000000000000);
        }
        else if (binary_exponent < 52) {
            discarded_mask = (CRT_U64(1) << (unsigned)(52 - binary_exponent)) - CRT_U64(1);
            if ((number.bits & discarded_mask) != 0u) {
                crt_no_op_stub_variant2();
            }
            number.bits &= ~discarded_mask;
        }
        *value = number.value;
        remaining &= ~2u;
    }
    if ((remaining & 0x10u) != 0u && (fpu_control_word & 0x20u) != 0u) {
        crt_no_op_stub_variant2();
        remaining &= ~0x10u;
    }
    return remaining == 0u;
}

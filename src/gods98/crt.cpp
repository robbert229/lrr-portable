// SPDX-License-Identifier: LGPL-3.0-only
#include <math.h>
#include <string.h>

#include "crt.hpp"
#include "heap.hpp"
#include "string.hpp"

/* FUN_00498000: the corpus's memmove with word loops and byte tails —
 * byte-equivalent to the host memmove (findings 260/394). */
void* crt_memory_move(void* dest, const void* src, size_t count)
{
    return memmove(dest, src, count);
}

/* FUN_00417430: __ftol under the truncate-toward-zero control word the
 * compiler switches to — a C++ cast (findings 381/393). Overflow values
 * are outside the corpus's caller set (deviation in crt.hpp). */
int float_to_long(float value)
{
    return (int)value;
}

/* FUN_0041f8e0: empty body; the corpus's callable no-op (finding 381). */
void noop_callback(void)
{
}

/* FUN_0047d3f0: popcount via the clear-lowest-set-bit loop. */
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
int bit_count32_by_position(uint32_t value)
{
    return pop_count32(value);
}

/* FUN_00497ab0: zero over exactly 3 dwords. */
int uint96_is_zero(const uint32_t limbs[3])
{
    return limbs[0] == 0u && limbs[1] == 0u && limbs[2] == 0u;
}

/* FUN_0048e870: the double's exponent field not all-ones. */
int double_is_finite(double value)
{
    uint64_t bits;
    __builtin_memcpy(&bits, &value, sizeof bits);
    return ((bits >> 52u) & 0x7ffu) != 0x7ffu;
}

/* FUN_0045b900: the corpus casts the int pair and the divisor to float
 * before both divides (the results land in a caller float pair). */
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* out_x, float* out_y)
{
    *out_x = (float)x / (float)divisor;
    *out_y = (float)y / (float)divisor;
}

/* FUN_0045b930: the compiler's two-shot __ftol — each float truncates
 * toward zero into its int output (same deviation as float_to_long). */
void float_pair_to_integer_pair(float x, float y, int* out_x, int* out_y)
{
    *out_x = (int)x;
    *out_y = (int)y;
}

/* FUN_00499fb0: the corpus's carry test — the sum below either operand
 * means the addition carried out of the 32-bit result. */
int crt_unsigned_add_with_carry(uint32_t a, uint32_t b, uint32_t* sum)
{
    const uint32_t s = a + b;
    *sum = s;
    return (s < a || s < b) ? 1 : 0;
}

/* FUN_00493a80: the exponent/quiet-bit pattern classifier over the two
 * bit words, corpus masks verbatim (finding 399). The corpus masks the
 * high word's top 16 bits with 0x7ff8 — exponent plus quiet bit. */
int double_classify(uint32_t lo, uint32_t hi)
{
    const uint32_t exp_quiet = (hi >> 16) & 0x7ff8u;
    if (hi == 0x7ff00000u && lo == 0u) {
        return 1;                                   /* +infinity */
    }
    if (hi == 0xfff00000u && lo == 0u) {
        return 2;                                   /* -infinity */
    }
    if (exp_quiet == 0x7ff8u) {
        return 3;                                   /* quiet-NaN-shaped */
    }
    if (exp_quiet == 0x7ff0u && ((hi & 0x7ffffu) != 0u || lo != 0u)) {
        return 4;                                   /* signaling-NaN-shaped */
    }
    return 0;                                       /* finite */
}

/* FUN_00499710: the 16-bit strlen — code units counted up to the NUL. */
int wide_string_length(const uint16_t* text)
{
    const uint16_t* p = text;
    while (*p != 0) {
        ++p;
    }
    return (int)(p - text);
}

/* FUN_0049add0: bounded scan — the terminator index plus one when found
 * within `bound` (count including the NUL), the bound otherwise; a zero
 * bound returns the bound without reading. */
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
 * The corpus takes the double's high word as its only live argument
 * (the first parameter is the unused low word). */
uint32_t double_exponent_bits_or_special(uint32_t high_word)
{
    if ((high_word & 0x7ff00000u) != 0x7ff00000u) {
        return high_word & 0x7ff00000u;
    }
    return high_word;
}

/* FormatArgumentReadDword (FUN_00491430, finding 395): read the dword
 * at the cursor, then advance it by four. */
uint32_t format_argument_read_dword(uint8_t** cursor)
{
    uint32_t v;
    memcpy(&v, *cursor, sizeof(v));
    *cursor = *cursor + 4;
    return v;
}

/* FormatArgumentReadWord (FUN_00491470, finding 395): read the word,
 * advance by two, return zero-extended (the corpus's CONCAT of the
 * advanced pointer's high half — zero for 32-bit cursors). */
uint32_t format_argument_read_word(uint8_t** cursor)
{
    uint16_t v;
    memcpy(&v, *cursor, sizeof(v));
    *cursor = *cursor + 2;
    return v;
}

/* FormatArgumentReadDouble (FUN_00491450, finding 395): read eight
 * bytes, advance by eight. */
double format_argument_read_double(uint8_t** cursor)
{
    double v;
    memcpy(&v, *cursor, sizeof(v));
    *cursor = *cursor + 8;
    return v;
}

/* X87RoundToIntegral (FUN_00493150, finding 379): FRNDINT under the
 * default control word — nearest-even integral, still floating.
 * rint() matches it exactly (and keeps -0, per the x87 answer). */
double x87_round_to_integral(double value)
{
    return rint(value);
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
double double_build_with_exponent(uint32_t significand_low,
                                  uint32_t significand_high,
                                  int16_t exponent)
{
    const uint32_t bias_shifted =
        (uint32_t)(uint16_t)((uint32_t)(int32_t)(exponent + 0x3fe)) << 4;
    const uint16_t top = (uint16_t)(
        ((significand_high >> 16) & 0x800fu) | bias_shifted);
    const uint64_t bits = ((uint64_t)top << 48)
        | ((uint64_t)(uint16_t)(significand_high & 0xffffu) << 32)
        | (uint64_t)significand_low;
    double value = 0.0;
    __builtin_memcpy(&value, &bits, sizeof(value));
    return value;
}

/* ------------------------------------------------------------------ *
 *  Constant and DAT-global band (see crt.hpp). */

double constant_zero_float_get(void)
{
    return 0.0;
}

int32_t constant_negative_10000_get(void)
{
    return -10000;
}

uint32_t processor_feature_fallback_false(void)
{
    return 0u;
}

uint32_t flh_magic_uu_noop(void)
{
    return 0u;
}

/* The two DAT globals, static-backed until the corpus's writers (all
 * still unported) land — message.cpp's s_system_state precedent. */
static double   s_global_float_00556f00 = 0.0;
static uint32_t s_global_dword_004ff9bc = 0u;

double global_float_field_00556f00_get(void)
{
    return s_global_float_00556f00;
}

uint32_t global_value_004ff9bc_store_and_return(const void* value)
{
    if (value == NULL) {
        return 0u; /* deviation: corpus dereferences blindly */
    }
    uint32_t v = 0u;
    memcpy(&v, value, sizeof(v));
    s_global_dword_004ff9bc = v;
    return v;
}

/* --- unowned DAT-global counter/accumulator band (see crt.hpp) -----------
 * None of these globals has a ported owner or feeder, so each lands as a
 * zero-initialized module static (the s_system_state precedent). */
static uint32_t s_counter_004ff990 = 0u;
static float    s_float_00554adc   = 0.0f;
static float    s_float_00554efc   = 0.0f;
static uint32_t s_counter_00555324 = 0u;
static uint32_t s_counter_00555328 = 0u;
static uint32_t s_counter_0055532c = 0u;
static float    s_float_00555330   = 0.0f;
static float    s_float_00555120   = 0.0f;

void global_counter_004ff990_increment(void)
{
    s_counter_004ff990 = s_counter_004ff990 + 1u;
}

void global_counter_004ff990_reset(void)
{
    s_counter_004ff990 = 0u;
}

void global_float_at_00554adc_add_one(void)
{
    s_float_00554adc = s_float_00554adc + 1.0f; /* corpus: x - -1.0 */
}

void global_float_at_00554efc_add_one(void)
{
    s_float_00554efc = s_float_00554efc + 1.0f;
}

void global_counter_at_00555324_increment(void)
{
    s_counter_00555324 = s_counter_00555324 + 1u;
}

void global_counter_at_00555328_increment(void)
{
    s_counter_00555328 = s_counter_00555328 + 1u;
}

void global_counter_at_0055532c_increment(void)
{
    s_counter_0055532c = s_counter_0055532c + 1u;
}

void global_float_accumulator_00555330_add(float value)
{
    s_float_00555330 = s_float_00555330 + value;
}

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

/* --- display-resolution filter tuples @ 0x49d870 (see crt.hpp) ------
 * Zero-initialized statics; the corpus's blind slot writes land inside the
 * guarded 16-entry window. The tuple count mirrors DAT_005754e0. */
typedef struct FilterTuple {
    int32_t w, h, bpp;
} FilterTuple;

#define FILTER_TUPLE_ENTRIES 16
static FilterTuple s_filter_tuples[FILTER_TUPLE_ENTRIES];
static int         s_filter_tuple_count;

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

/* --- the remaining unowned DAT one-liners (see crt.hpp) ---------------------
 * Zero-initialized module statics per the s_system_state precedent — the
 * globals' meanings are unresolved (finding 383) and none has a ported
 * feeder. */
static uint32_t s_flag_004ff978 = 0u;
static uint32_t s_value_004ffe48 = 0u;

uint32_t global_flag_004ff978_get(void)
{
    return s_flag_004ff978;
}

void global_value_004ffe48_set(uint32_t value)
{
    s_value_004ffe48 = value;
}

/* --- the unowned DAT word-store band (see crt.hpp) -------------------------- */
static uint32_t s_advisor_state_word = 0u; /* DAT_004b31bc */
static uint32_t s_task_refresh_flag = 0u;  /* DAT_00557860 */
static uint32_t s_level_stored_word = 0u;  /* DAT_004b8a54 */
static uint32_t s_mode_zero_flag = 0u;     /* DAT_004c7178 */
static uint32_t s_rockfall_base = 0u;      /* DAT_004c79a4 */
static uint32_t s_ui_flag_word = 0u;       /* DAT_004c7ea8 */
static uint32_t s_config_word = 0u;        /* DAT_004a1ee4 */

uint32_t global_flag_bit0_is_set(void)
{
    return s_advisor_state_word & 1u;
}

void state_flag_toggle_when_zero(void)
{
    s_task_refresh_flag = (s_task_refresh_flag == 0u) ? 1u : 0u;
}

uint32_t global_task_refresh_flag_get(void)
{
    return s_task_refresh_flag;
}

void level_state_stored_value_set(uint32_t v)
{
    s_level_stored_word = v;
}

void global_mode_zero_input_set(int value)
{
    s_mode_zero_flag = (value == 0) ? 1u : 0u;
}

void rockfall_style_table_base_set(uint32_t v)
{
    s_rockfall_base = v;
}

void global_flag_bit0_clear(void)
{
    s_ui_flag_word &= ~1u;
}

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
static int s_display_w;   /* DAT_00505924 */
static int s_display_h;   /* DAT_00505928 */

int display_width_get(void)  { return s_display_w; }
int display_height_get(void) { return s_display_h; }

void display_dimensions_set(int w, int h)
{
    s_display_w = w;
    s_display_h = h;
}

/* DAT_0050013c record block: 18 records of 0x30 bytes (the 0x50013c…
 * 0x50037c span the panel reset walks), flag word at record+0. Only the
 * flag words are modeled — the record payloads belong to the unported
 * panel layer. */
#define REQUEST_RECORD_COUNT 18
static uint32_t s_request_flags[REQUEST_RECORD_COUNT];

int request_record_ready_flag4(int index)
{
    if (index < 0 || index >= REQUEST_RECORD_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    const uint32_t f = s_request_flags[index];
    return (f & 4u) != 0u && (f & 8u) == 0u ? 1 : 0;
}

int request_record_ready_flag2(int index)
{
    if (index < 0 || index >= REQUEST_RECORD_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    const uint32_t f = s_request_flags[index];
    return (f & 2u) != 0u && (f & 8u) == 0u ? 1 : 0;
}

int request_record_flag8_is_set(int index)
{
    if (index < 0 || index >= REQUEST_RECORD_COUNT) {
        return 0; /* deviation: the corpus strides blindly */
    }
    return (int)(s_request_flags[index] & 8u);
}

void request_record_flags_set(int index, uint32_t flags)
{
    if (index >= 0 && index < REQUEST_RECORD_COUNT) {
        s_request_flags[index] = flags;
    }
}

uint32_t request_record_flags_peek(int index)
{
    if (index < 0 || index >= REQUEST_RECORD_COUNT) {
        return 0u;
    }
    return s_request_flags[index];
}

/* --- the remaining DAT one-liner stores/reads (see crt.hpp) ----------------- */
static uint32_t s_value_0050080c = 0u;
static uint32_t s_font_words[16];   /* DAT_00500a9c window */
static uint32_t s_cursor_word = 0u; /* DAT_00500a98 */

void global_value_0050080c_set(uint32_t value)
{
    s_value_0050080c = value;
}

uint32_t global_value_0050080c_peek(void)
{
    return s_value_0050080c;
}

uint32_t indexed_global_value_at_get(int index)
{
    if (index < 0 || index >= 16) {
        return 0u; /* deviation: the corpus strides blindly */
    }
    return s_font_words[index];
}

uint32_t global_cursor_word_get(void)
{
    return s_cursor_word;
}

void font_cursor_word_set(int index, uint32_t v)
{
    if (index == -1) {
        s_cursor_word = v; /* the DAT_00500a98 word itself */
        return;
    }
    if (index >= 0 && index < 16) {
        s_font_words[index] = v;
    }
}

/* --- pure-return stubs + advisor word clear (see crt.hpp) -------------------- */
void crt_no_op_stub(void)
{
    /* FUN_00491ff5: pure return — faithful. */
}

void crt_no_op_stub_variant(void)
{
    /* FUN_00497870: pure return — faithful. */
}

void advisor_clear_active_state(void)
{
    s_advisor_state_word &= ~2u;
}

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


void indexed_pair_set(int index, uint32_t x, uint32_t y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return; /* deviation: the corpus strides blindly */
    }
    s_pair_a[index] = x;
    s_pair_b[index] = y;
}

void indexed_pair_get(int index, uint32_t* out_x, uint32_t* out_y)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT
        || out_x == NULL || out_y == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    *out_x = s_pair_a[index];
    *out_y = s_pair_b[index];
}

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

void panel_item_flag16_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x10u : *w & ~0x10u;
}

void panel_item_flag32_set(int panel, int item, int value)
{
    uint32_t* w = panel_item_word(panel, item);
    if (w == NULL) {
        return;
    }
    *w = value != 0 ? *w | 0x20u : *w & ~0x20u;
}

void panel_state_flags_toggle(int index)
{
    if (index < 0 || index >= DAT_WINDOW_COUNT) {
        return;
    }
    const uint32_t f = s_request_flags[index];
    if ((f & 2u) != 0u) {
        s_request_flags[index] = f & ~2u | 0xcu;
    } else {
        s_request_flags[index] = f & ~4u | 0xau;
    }
}

static float    s_reward_score;  /* DAT_00555738 */
static float    s_reward_count;  /* DAT_0055573c */

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

uint32_t crt_surface_handle_next(void)
{
    static uint32_t s_handle_counter = 0u;
    ++s_handle_counter;
    return s_handle_counter;
}

void crt_surface_blit_record(uint32_t handle, float x, float y)
{
    if (s_call_n < CRT_SURFACE_CALLS) {
        s_call_handle[s_call_n] = handle;
        s_call_x[s_call_n] = x;
        s_call_y[s_call_n] = y;
        ++s_call_n;
    }
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

void uint96_zero(uint32_t limbs[3])
{
    if (limbs == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    limbs[0] = 0u;
    limbs[1] = 0u;
    limbs[2] = 0u;
}

/* GlobalValueClearIfEqual (FUN_0040e910) — the screen-effect handle word
 * (the corpus's DAT_004c7ea4, unowned) clears only on a matching release. */
static int s_global_value_word;

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
static void* s_linked_list_head;  /* DAT_004aa458 role */
static void* s_record_list_head;  /* DAT_00506518 role */

static void record_list_push(void** head, void* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    *(void**)((char*)record + 0x1c) = *head;
    *head = record;
}

void linked_list_push_at_global_head(void* record)
{
    record_list_push(&s_linked_list_head, record);
}

void* linked_list_global_head_peek(void) { return s_linked_list_head; }

void linked_list_global_head_set(void* record)
{
    s_linked_list_head = record;
}

void global_record_list_push_head(void* record)
{
    record_list_push(&s_record_list_head, record);
}

void* global_record_list_head_peek(void) { return s_record_list_head; }

/* SceneEffectRegistryReset (FUN_00465380) — the transient-effect registry
 * words (head/tail/shared-texture, unowned) clear and the reset answers 1
 * exactly as the corpus does. */
static uint32_t s_scene_effect_words[3];

int scene_effect_registry_reset(void)
{
    s_scene_effect_words[0] = 0u;
    s_scene_effect_words[1] = 0u;
    s_scene_effect_words[2] = 0u;
    return 1;
}

void scene_effect_registry_words_peek(uint32_t out_words[3])
{
    if (out_words == NULL) {
        return;
    }
    for (int i = 0; i < 3; ++i) {
        out_words[i] = s_scene_effect_words[i];
    }
}

/* CrtFatalMessageExit (FUN_00498340) — the corpus's exhausted-memory
 * abort. The port aborts with the host runtime, or routes through the
 * observer hook when installed so the path is testable (deviation). */
static void (*s_fatal_exit_observer)(void);

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
void crt_no_op_stub_variant2(void)
{
    return;
}

/* LinkedRecordChainFree / GlobalLinkedRecordChainClear — see crt.hpp. */
static void* s_linked_list_aux; /* DAT_004aa450 role: unowned, zero-init */

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

void global_linked_record_chain_clear(void)
{
    s_linked_list_aux = NULL;
    linked_record_chain_free(s_linked_list_head);
    s_linked_list_head = NULL;
}

void* linked_list_global_aux_peek(void) { return s_linked_list_aux; }
void  linked_list_global_aux_set(void* word) { s_linked_list_aux = word; }

/* ReleaseLinkedObjectList (FUN_00479240) — see crt.hpp. */
void linked_object_list_release(void* list)
{
    if (list == NULL) {
        return; /* deviation: the corpus strides the head blindly */
    }
    void* owner = NULL;
    __builtin_memcpy(&owner, (char*)list, sizeof owner);
    engine_heap_free(owner);
    for (void* node = list; node != NULL;) {
        void* next = NULL;
        __builtin_memcpy(&next, (char*)node + 0x14, sizeof next);
        global_record_list_push_head(node);
        node = next;
    }
}

/* ModelRenderListClear (FUN_00482d30) — see crt.hpp. */
static void* s_model_render_list_head; /* DAT_005343c4 role: unowned */

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

/* CrtOwnedStreamBufferRelease (FUN_00494df0) — see crt.hpp. */
void crt_owned_stream_buffer_release(char* record)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    /* Deviation: the port lays the corpus's dword field window out as host
     * words — field n at record + n*sizeof(void*), flags last. */
    const long field = (long)sizeof(void*);
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, record + 3 * field, sizeof flags);
    if ((flags & 0x83u) != 0u && (flags & 0x8u) != 0u) {
        void* buffer = NULL;
        __builtin_memcpy(&buffer, record + 2 * field, sizeof buffer);
        engine_heap_free(buffer);
        flags &= 0xfffffbf7u; /* corpus mask: bits 3 and 11 */
        __builtin_memcpy(record + 3 * field, &flags, sizeof flags);
        void* zero = NULL;
        __builtin_memcpy(record + 0 * field, &zero, sizeof zero);
        __builtin_memcpy(record + 1 * field, &zero, sizeof zero);
        __builtin_memcpy(record + 2 * field, &zero, sizeof zero);
    }
}

/* --- the shared texture directory word (see crt.hpp) ------------------------ */
static char* s_shared_texture_dir; /* DAT_0076cd88 role: unowned */

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
static uint32_t s_renderer_flag_word; /* DAT_0076cd94 role: unowned */
static void*    s_scene_end_hook;     /* DAT_0076cd80 role: unowned */
static void*    s_scene_end_value;    /* DAT_0076cd84 role: unowned */
static void*    s_scene_sample_hook;  /* DAT_0076cd78 role: unowned */
static void*    s_scene_sample_value; /* DAT_0076cd7c role: unowned */

void renderer_flag_bit6_set(int value)
{
    s_renderer_flag_word = (value != 0) ? (s_renderer_flag_word | 0x40u)
                                        : (s_renderer_flag_word & ~0x40u);
}

uint32_t renderer_flag_word_peek(void) { return s_renderer_flag_word; }

void scene_end_hook_register(void* hook, void* value)
{
    s_scene_end_hook = hook;
    s_scene_end_value = value;
}

void* scene_end_hook_peek(void) { return s_scene_end_hook; }
void* scene_end_hook_value_peek(void) { return s_scene_end_value; }

void scene_sample_hook_register(void* hook, void* value)
{
    s_scene_sample_hook = hook;
    s_scene_sample_value = value;
    s_renderer_flag_word |= 0x40u;
}

void* scene_sample_hook_peek(void) { return s_scene_sample_hook; }
void* scene_sample_hook_value_peek(void) { return s_scene_sample_value; }

/* --- the string registry (see crt.hpp) ------------------------------------- */
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

void* string_keyed_cache_entry_find(const char* key)
{
    void* entry = s_string_registry_head;
    if (entry == NULL) {
        return NULL;
    }
    const long field = (long)sizeof(void*);
    for (;;) {
        char* name = NULL;
        __builtin_memcpy(&name, (char*)entry + 0 * field, sizeof name);
        if (name != NULL && s_strcasecmp(name, key) == 0) {
            return entry;
        }
        void* next = NULL;
        __builtin_memcpy(&next, (char*)entry + 3 * field, sizeof next);
        if (next == NULL) {
            return NULL;
        }
        entry = next;
    }
}

void* string_registry_intern(const char* key)
{
    if (key == NULL || *key == '\0') {
        return NULL; /* deviation: the corpus strides a null string blindly */
    }
    void* entry = string_keyed_cache_entry_find(key);
    if (entry != NULL) {
        return entry;
    }
    const long field = (long)sizeof(void*);
    void* record = engine_heap_alloc(4 * field);
    if (record == NULL) {
        return NULL; /* corpus asserts in the heap seam */
    }
    const size_t n = strlen(key) + 1u;
    char* copy = (char*)engine_heap_alloc((long)n);
    if (copy == NULL) {
        engine_heap_free(record);
        return NULL;
    }
    memcpy(copy, key, n);
    const void* zero = NULL;
    __builtin_memcpy((char*)record + 0 * field, &copy, sizeof copy);
    __builtin_memcpy((char*)record + 1 * field, &zero, sizeof zero);
    __builtin_memcpy((char*)record + 2 * field, &zero, sizeof zero);
    __builtin_memcpy((char*)record + 3 * field, &s_string_registry_head,
                     sizeof s_string_registry_head);
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

void* string_cache_lookup_or_load(const char* key)
{
    return s_entry_cache_slot_get(key, 1, s_bitmap_surface_loader);
}

void* resource_name_cache_get_or_load(const char* key)
{
    return s_entry_cache_slot_get(key, 2, s_font_resource_loader);
}

/* --- the selection marker table (see crt.hpp) ------------------------------
 * DAT_004c7e00..e7c is 32 pointer slots plus the count word at e80; the
 * descriptor sits at DAT_004c7df8. All unowned: zero-init statics here. */
static void*    s_marker_table[32];
static uint32_t s_marker_count;
static void*    s_marker_descriptor;

void selection_marker_resources_release(void)
{
    for (int i = 0; i < 32; ++i) {
        if (s_marker_table[i] != NULL) {
            engine_heap_free(s_marker_table[i]);
        }
    }
    /* Deviation: the corpus frees without clearing the slots (a repeated
     * release would double-free); the port zeroes the window. */
    memset(s_marker_table, 0, sizeof s_marker_table);
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
void string_prefix_reserve(char* s, int count)
{
    if (count == 0 || s == NULL) {
        return; /* deviation: the corpus strides s blindly */
    }
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    crt_memory_move(s + count, s, len + 1); /* tail NUL rides the move */
}

/* CrtStreamBufferAllocate (FUN_00498ae0) — see crt.hpp. DAT_005493e4 is
 * unowned: zero-init hooked static. */
static long s_stream_alloc_count;

long stream_allocation_count_peek(void) { return s_stream_alloc_count; }
void stream_allocation_count_reset(void) { s_stream_alloc_count = 0; }

void crt_stream_buffer_allocate(char* record)
{
    ++s_stream_alloc_count;
    if (record == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    /* Port layout (the crt_owned_stream_buffer_release convention): field n
     * at record + n*sizeof(void*); field 6 is the corpus's capacity word. */
    const long field = (long)sizeof(void*);
    void* buffer = engine_heap_alloc(0x1000);
    __builtin_memcpy(record + 2 * field, &buffer, sizeof buffer);
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, record + 3 * field, sizeof flags);
    if (buffer != NULL) {
        flags |= 8u;
        const uint32_t cap = 0x1000u;
        __builtin_memcpy(record + 6 * field, &cap, sizeof cap);
    } else {
        flags |= 4u;
        const uint32_t cap = 2u;
        __builtin_memcpy(record + 6 * field, &cap, sizeof cap);
        buffer = record + 5 * field; /* corpus: internal buffer at field 5 */
        __builtin_memcpy(record + 2 * field, &buffer, sizeof buffer);
    }
    __builtin_memcpy(record + 3 * field, &flags, sizeof flags);
    void* zero = NULL;
    __builtin_memcpy(record + 1 * field, &zero, sizeof zero);
    __builtin_memcpy(record + 0 * field, &buffer, sizeof buffer);
}

/* UInt96Add (FUN_00499fe0) — see crt.hpp. Every carry-add writes its sum
 * whether or not it carries; only the bump is gated. */
void crt_uint96_add(uint32_t* dst, const uint32_t* src)
{
    if (dst == NULL || src == NULL) {
        return; /* deviation: the corpus strides both limbs blindly */
    }
    uint32_t s = 0;
    int carry = crt_unsigned_add_with_carry(dst[0], src[0], &s);
    dst[0] = s;
    if (carry != 0) {
        carry = crt_unsigned_add_with_carry(dst[1], 1u, &s);
        dst[1] = s;
        if (carry != 0) {
            dst[2] = dst[2] + 1u;
        }
    }
    carry = crt_unsigned_add_with_carry(dst[1], src[1], &s);
    dst[1] = s;
    if (carry != 0) {
        dst[2] = dst[2] + 1u;
    }
    if (crt_unsigned_add_with_carry(dst[2], src[2], &s) != 0) {
        /* corpus: the carry out of the high limb is discarded */
    }
    dst[2] = s;
}

/* UInt96IncrementAtBit (FUN_00497970) — see crt.hpp. The carry walks DOWN
 * the limb indices (toward limb 0), the opposite direction from the
 * uint96-add chain; the corpus's bit-mask reduction rides the byte-trick
 * expression, which simplifies to bit % 32 for the non-negative inputs. */
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
 * runs only while "bit is set AND something lives at or above bit + 1",
 * the inverse of the intuition the name invites; after that the limb keeps
 * the at-or-above half (mask -1 << (31 - bit % 32), the complement of the
 * has-no-bits mask) and every higher limb goes to zero. */
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
    uint32_t result = 0u; /* corpus local_4: the increment's undefined
                           * return value, pinned 0 (deviation) */
    if ((limbs[limb] & (1u << shift)) != 0u &&
        crt_uint96_has_no_bits_at_or_above(limbs, bit + 1) == 0u) {
        crt_uint96_increment_at_bit(limbs, bit - 1);
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
static uint32_t s_renderer_word_a;      /* DAT_0076ab24 */
static uint32_t s_renderer_word_b;      /* DAT_0076ab28 */
static uint32_t s_renderer_word_c;      /* DAT_0076ab3c */
static uint32_t s_renderer_global_dword; /* DAT_0076ac84 */
static uint32_t s_render_surface_secondary; /* DAT_0076ac8c */
static uint32_t s_render_feature_flags; /* DAT_0050608c */

uint32_t renderer_word_a_get(void) { return s_renderer_word_a; } /* FUN_00410b30 */
uint32_t renderer_word_b_get(void) { return s_renderer_word_b; } /* FUN_00410b40 */
uint32_t renderer_word_c_get(void) { return s_renderer_word_c; } /* FUN_00410b50 */
void renderer_words_set(uint32_t a, uint32_t b, uint32_t c)
{
    s_renderer_word_a = a;
    s_renderer_word_b = b;
    s_renderer_word_c = c;
}

uint32_t renderer_global_dword_get(void) { return s_renderer_global_dword; }
void renderer_global_dword_set(uint32_t value) { s_renderer_global_dword = value; }

uint32_t render_surface_secondary_get(void) { return s_render_surface_secondary; }
void render_surface_secondary_set(uint32_t value) { s_render_surface_secondary = value; }

uint32_t render_feature_flags_get(void) { return s_render_feature_flags; }
void render_feature_flags_set(uint32_t value) { s_render_feature_flags = value; }

/* SetLowTextureFlag (2026-10-25): the corpus's DAT_00505f84 engine word has
 * a second half-model (EngineContext::feature in engine.hpp). This band's
 * store carries no context, so the port models it as a module static with
 * install/peek hooks — the two half-models do not interoperate
 * (unowned-global deviation). */
static uint32_t s_engine_feature_word; /* DAT_00505f84 */

uint32_t engine_feature_word_peek(void)
{
    return s_engine_feature_word;
}

void engine_feature_word_install(uint32_t value) /* port hook */
{
    s_engine_feature_word = value;
}

void low_texture_flag_set(void) /* FUN_00477bd0 */
{
    if ((s_engine_feature_word & 0x40000u) == 0u) {
        s_engine_feature_word |= 0x20u;
    }
}

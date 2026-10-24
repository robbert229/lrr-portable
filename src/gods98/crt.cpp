// SPDX-License-Identifier: LGPL-3.0-only
#include <math.h>
#include <string.h>

#include "crt.hpp"

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

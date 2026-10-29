// SPDX-License-Identifier: LGPL-3.0-only
/* CRT and compiler-runtime leaves (findings 260/381/393/394): the small
 * runtime thunks the corpus calls directly.
 *
 * Deviations (documented): the corpus's word-at-a-time memmove loops are
 * byte-equivalent memmove behavior, so the port calls memmove. The
 * corpus's FloatToLong is the compiler's __ftol helper — under the x87
 * control word it sets, that is C++ truncation toward zero; the port uses
 * the C++ cast (the corpus's 32-bit fist result vs the host cast is the
 * usual x87/IEEE deviation; overflow values are outside the corpus's
 * caller set). NoOpCallback's return value is undefined per finding 381 —
 * callers only use it as a callable no-op. */
#ifndef LEGORRR_GODS98_CRT_HPP
#define LEGORRR_GODS98_CRT_HPP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FUN_00498000: memmove — copies the byte range with overlap handling,
 * backward when the destination starts inside the source range, forward
 * otherwise. Returns the destination. */
void* crt_memory_move(void* dest, const void* src, size_t count);

/* FUN_00417430: __ftol — float to 32-bit integer, truncated toward zero
 * (the corpus's 32-bit fist result; the port mirrors the width). */
int float_to_long(float value);

/* FUN_0041f8e0: the callable no-op installed by several engine paths. */
void noop_callback(void);

/* CRT runtime-error reporting (FUN_004968b0 / FUN_004968f0 / FUN_00499730).
 * The callback is invoked between the 0xfc and 0xff marker messages when
 * the CRT's runtime-error reporting gate is enabled. */
void crt_runtime_error_message_enter_exit(void);
void crt_runtime_error_message(int error_code);
int crt_runtime_error_message_box_display(const char* message,
                                          const char* title,
                                          unsigned int flags);
void crt_terminate_handler_invoke_and_abort(void);

/* FUN_0047d3f0: bit count (corpus: clear-lowest-set-bit loop). */
int bit_count32(uint32_t value);

/* FUN_0047d700: bit count by position loop — same answer, corpus keeps the
 * separate body. */
int pop_count32(uint32_t value);

/* FUN_0047d720: lowest set bit's index, or 0xffffffff when value is zero. */
uint32_t find_first_set_bit32(uint32_t value);

/* FUN_0047e7e0: the position-loop bit count again (the corpus's second
 * identical body; kept distinct to mirror the address map). */
int bit_count32_by_position(uint32_t value);

/* FUN_00497ab0: zero test over 3 dwords (the "96-bit" pair of 32-bit limbs
 * plus its high word — the corpus checks exactly 3). */
int uint96_is_zero(const uint32_t limbs[3]);

/* FUN_0048e870: the double finite test — exponent field not all ones,
 * so true for finite values and false for Inf/NaN. */
int double_is_finite(double value);

/* FUN_0045b900: divide an integer pair by `divisor`, both operands cast to
 * float before the divide; results land in the float pair. */
void integer_pair_divide_to_float_pair(int x, int y, int divisor,
                                       float* out_x, float* out_y);

/* FUN_0045b930: __ftol applied twice — the float pair truncated toward
 * zero into the integer pair (same deviation as float_to_long). */
void float_pair_to_integer_pair(float x, float y, int* out_x, int* out_y);

/* FUN_00499fb0: unsigned 32-bit add with carry — stores the sum and
 * answers 1 when the addition carried (finding 302; the primitive the
 * corpus's 96-bit increment/add/multiply helpers build on). */
int crt_unsigned_add_with_carry(uint32_t a, uint32_t b, uint32_t* sum);

/* FUN_00493a80: the double special-value classifier over the raw bit
 * pair — 1 positive infinity, 2 negative infinity, 3 quiet-NaN-shaped
 * (exponent all ones, quiet bit set), 4 signaling-NaN-shaped (exponent
 * all ones, quiet bit clear, mantissa nonzero), 0 finite (finding
 * 399). Sign does not steer cases 3/4, matching the corpus. */
int double_classify(uint32_t lo, uint32_t hi);

/* FUN_00499710: UTF-16 code units before the NUL terminator (finding 377
 * — the corpus walks 16-bit units the same way strlen walks bytes). */
int wide_string_length(const uint16_t* text);

/* FUN_0049add0: scan at most `bound` wide characters; answer the
 * terminator index plus one (the count including the NUL) when found
 * inside the bound, otherwise the bound itself — and the bound for a
 * zero bound, as in the corpus (findings 342/397). */
int bounded_wide_string_length_including_null(const uint16_t* text,
                                              int bound);

/* FUN_00492068: double high-word exponent selector — the exponent field
 * for ordinary values, the untouched high word when the exponent is all
 * ones (Inf/NaN) (findings 386/399; CrtAsin's pre-dispatch probe). The
 * corpus callee takes a cdecl double: the low word is unused, the high
 * word is live — two uint32_t params so MSVC5 loads [esp+8]. */
uint32_t double_exponent_bits_or_special(uint32_t low_word,
                                         uint32_t high_word);

/* Format-argument cursor reads (FUN_00491430 / FUN_00491470 /
 * FUN_00491450; findings 231/395 — EngineFormatStringToStream's
 * width/precision/value walk): each reads the value at the cursor and
 * advances it. The word read returns the zero-extended 16-bit value
 * (the corpus's CONCAT upper half is the pointer's own high word — zero
 * in 32-bit space). Reads go through memcpy — the cursor may sit at an
 * unaligned continuation of a previous wider read. */
uint32_t format_argument_read_dword(uint8_t** cursor);
uint32_t format_argument_read_word(uint8_t** cursor);
double format_argument_read_double(uint8_t** cursor);

/* FUN_00493150: x87 round-to-integral (FRNDINT) — the nearest integer
 * under the round-to-nearest-even mode, as a floating value. Ported as
 * double (the port has no x87 80-bit register; every x87 leaf here is a
 * documented single/double deviation). */
double x87_round_to_integral(double value);

/* CRT math runtime leaves (FUN_0048d940 / FUN_0048e1aa / FUN_0048e280 /
 * FUN_0048e680). The dispatch initializer seeds the six callback words;
 * intrinsic dispatch requires the original x87/caller-frame context and
 * currently lands as a port seam. The numeric leaves preserve the ordinary
 * finite and infinity behavior; CRT matherr callbacks remain unported. */
void crt_math_dispatch_table_initialize(void);
void crt_math_intrinsic_dispatch(void);
void crt_floating_point_exception_result_dispatch(void);
void crt_floating_point_exception_result_dispatch_variant(void);
double crt_double_remainder_reduction(double dividend, double divisor);
double crt_fmod_extended_precision(double dividend, double divisor);
uint32_t crt_fpu_precision_mode_code_get(void);
double crt_round_to_integral_with_math_error_handling(double value);
double crt_round_to_integral_with_math_error_handling_variant(double value);
double crt_double_absolute_value(double value);

/* CrtRunCallbackArray (FUN_0048e5a0): call each nonnull cdecl callback
 * in the half-open pointer range, in forward order. */
typedef void (*CrtCallback)(void);
void crt_run_callback_array(CrtCallback* start, CrtCallback* end);

/* FUN_00493a40 (DoubleBuildWithExponent, findings 383/399): rebuild the
 * 8-bit-exponent floating value from the two significand words and the
 * exponent bias input — the corpus's exact bit recipe: top word =
 * ((param_2 >> 16) & 0x800f) | ((param_3 + 0x3fe) << 4), significand =
 * (param_2 & 0xffff) : param_1. Ported as double (x87 deviation, as
 * above); the bits are built with integer arithmetic and bitcast. */
double double_build_with_exponent(uint32_t significand_low,
                                  uint32_t significand_high,
                                  int16_t exponent);

/* DoubleFrexpDecompose (FUN_00493ae0): returns the input significand
 * normalized to [0.5, 1) (with its sign), and stores the binary exponent. */
double double_frexp_decompose(uint32_t low_word, uint32_t high_word,
                              int* exponent);

/* CRT math exception dispatch (FUN_004938f0). The image passes its
 * _exception fields as eight dwords plus the saved FPU control word. */
double crt_math_exception_dispatch(uint32_t error_code, uint32_t operation,
                                   uint32_t argument1_low,
                                   uint32_t argument1_high,
                                   uint32_t argument2_low,
                                   uint32_t argument2_high,
                                   uint32_t result_low,
                                   uint32_t result_high,
                                   uint32_t fpu_control_word);
double crt_math_exception_handle_single_result(uint32_t operation,
                                                uint32_t result_low,
                                                uint32_t result_high,
                                                uint32_t fpu_control_word);
void crt_floating_point_exception_raise(uint32_t* record,
                                       uint32_t* control_word,
                                       uint8_t exception_flags,
                                       uint32_t precision,
                                       const uint32_t* argument,
                                       uint32_t* result);
double crt_math_exception_handle_result_with_status(uint32_t status,
                                                     uint32_t operation,
                                                     uint32_t argument_low,
                                                     uint32_t argument_high,
                                                     uint32_t result_low,
                                                     uint32_t result_high,
                                                     uint32_t fpu_control_word);

/* CrtDoubleToIntegerRound (FUN_004935c0): fold exception status into the
 * caller's double result and report whether all status bits were handled. */
int crt_double_to_integer_round(uint32_t status, double* value,
                                uint32_t fpu_control_word);

/* CrtScanFormatCharNormalize (FUN_00494c30): leaves decimal digits alone;
 * maps hexadecimal A-F/a-f to 10-15 using the corpus's mask arithmetic. */
uint32_t crt_scan_format_char_normalize(uint32_t value);

/* ------------------------------------------------------------------ *
 *  Constant and DAT-global band (2026-10-24; findings 380/383/386/
 *  392/399/406/440). The corpus's fixed-return leaves and the two
 *  module-backed DAT globals, modeled like message.cpp's s_system_state
 *  precedent: no ported writer touches these addresses yet, so the
 *  storage answers its corpus-initial value until the writers land.
 *  Deviations: corpus x87 float10 constants and globals read as double
 *  (as across this module); the corpus's ignored arguments keep the
 *  definitions void (its callers pass them and drop them).
 * ------------------------------------------------------------------ */

/* ConstantZeroFloatGet (FUN_00408930, findings 380/392): the constant
 * 0.0 leaf. */
double constant_zero_float_get(void);

/* ConstantNegative10000Get (FUN_0047c160, findings 383/406): returns
 * -10000 (the corpus's dword 0xffffd8f0) — the DirectSound
 * minimum-volume constant. */
int32_t constant_negative_10000_get(void);

/* ProcessorFeatureFallbackFalse (FUN_00490230, findings 386/399): zero
 * — the fallback when IsProcessorFeaturePresent cannot be resolved. */
uint32_t processor_feature_fallback_false(void);

/* CrtFloatingPointFeatureProbe (FUN_00490280): dynamically queries
 * IsProcessorFeaturePresent(0) on the WinAPI backend; native uses the
 * missing-export fallback and returns zero. */
uint32_t crt_floating_point_feature_probe(void);

/* FlhMagicUuNoop (FUN_00456210, finding 440): the FLH magic 'UU'
 * pass's silent leaf — returns zero without changing state; the corpus
 * call sites (FLIC and CRT paths) do not reveal a further role. */
uint32_t flh_magic_uu_noop(void);

/* GlobalFloatField00556F00Get (FUN_004230b0, findings 295/392): reads
 * the global float at DAT_00556f00 as the corpus does (fld DWORD).
 * Its meaning is unresolved — storage stays static-backed. */
float global_float_field_00556f00_get(void);

/* GlobalValue_004ff9bcStoreAndReturn (FUN_004567c0, findings 386/440):
 * stores the pointed-to dword into DAT_004ff9bc-backed storage and
 * returns it. NULL answers zero and stores nothing (documented
 * deviation; corpus callers always pass a live pointer). */
uint32_t global_value_004ff9bc_store_and_return(const void* value);

/* GlobalCounter_004ff990Increment / GlobalCounter_004ff990Reset
 * (FUN_00454880 / FUN_00454890, findings 386/440): bump / zero the global
 * counter. No ported code owns or feeds this counter, so it lands as a
 * zero-initialized module static (the s_system_state precedent). */
void global_counter_004ff990_increment(void);
void global_counter_004ff990_reset(void);

/* GlobalFloatAt00554adcAddOne / GlobalFloatAt00554efcAddOne
 * (FUN_00462830 / FUN_00462850, finding 357): add 1.0 to the named global
 * float. Both land as zero-initialized module statics — no ported code
 * owns or feeds them (findings leave their unit unresolved). */
void global_float_at_00554adc_add_one(void);
void global_float_at_00554efc_add_one(void);

/* GlobalCounterAt00555324 / 00555328 / 0055532cIncrement (FUN_00462870 /
 * FUN_00462880 / FUN_00462890, finding 357): bump the three named global
 * counters; zero-initialized module statics (unowned precedents). */
void global_counter_at_00555324_increment(void);
void global_counter_at_00555328_increment(void);
void global_counter_at_0055532c_increment(void);

/* GlobalFloatAccumulator_00555330Add / GlobalFloatAccumulate_00555120
 * (FUN_004628a0 / FUN_004628c0, findings 383/386): add the argument to
 * the named global float accumulator; zero-initialized module statics. */
void global_float_accumulator_00555330_add(float value);
void global_float_accumulate_00555120(float value);

/* Tests/diagnostics census of the DAT statics above (the
 * message_active_count precedent): read-only views over the module's
 * static-backed counter/accumulator globals. */
uint32_t crt_counter_004ff990_peek(void);
float    crt_float_00554adc_peek(void);
float    crt_float_00554efc_peek(void);
uint32_t crt_counter_00555324_peek(void);
uint32_t crt_counter_00555328_peek(void);
uint32_t crt_counter_0055532c_peek(void);
float    crt_float_00555330_peek(void);
float    crt_float_00555120_peek(void);

#ifdef __cplusplus
}
#endif

/* Display-mode list and resolution-filter rows (findings 425/282). The
 * entry layout keeps the corpus's 0x110-byte stride: width, height, bpp,
 * then the mode name at +0x0c. Enumeration remains a platform seam; the
 * caller supplies rows to the port-side registry. */
#define CRT_DISPLAY_MODE_NAME_MAX 260
#define CRT_DISPLAY_MODE_ENTRIES 256
typedef struct CrtDisplayModeEntry {
    int32_t width, height, bpp;
    char name[CRT_DISPLAY_MODE_NAME_MAX];
} CrtDisplayModeEntry;
typedef struct CrtDisplayModeList {
    uint32_t count;
    uint32_t selected_index;
    uint32_t selected_mode_index;
    int32_t default_action_enabled;
    int32_t empty_message_kind; /* 1 = bit-depth modes, 2 = screen modes */
    int32_t desktop_bpp;
    uint32_t mode_indices[CRT_DISPLAY_MODE_ENTRIES];
} CrtDisplayModeList;
typedef struct CrtDisplayModeDialog {
    int32_t mode;
    uint32_t mode_label_resource; /* 0x4b2d38 or 0x4b2d44 */
    int32_t previous_mode;
    int32_t refresh_rate_control_enabled;
    int32_t checkbox_3ec_checked;
    int32_t checkbox_3ed_checked;
    CrtDisplayModeList modes;
} CrtDisplayModeDialog;
int  display_mode_entry_append(int mode, int width, int height, int bpp,
                               const char* name);
int  display_mode_entry_count(void);
int  display_mode_entry_get(int index, CrtDisplayModeEntry* out);
int  display_mode_name_index_find(const char* name, uint32_t* out_index);
void display_mode_list_populate(int mode, int desktop_bpp,
                                CrtDisplayModeList* out);
void display_mode_dialog_set_mode_and_refresh_choices(
    CrtDisplayModeDialog* dialog, int mode, int desktop_bpp);
void display_refresh_rate_controls_sync(CrtDisplayModeDialog* dialog,
                                        uint32_t adapter_flags,
                                        int desktop_bpp);

#define CRT_DISPLAY_RESOLUTION_ENTRIES 256
typedef struct CrtDisplayResolutionEntry {
    char name[CRT_DISPLAY_MODE_NAME_MAX];
    uint32_t flags;
} CrtDisplayResolutionEntry;
typedef struct CrtDisplayResolutionList {
    uint32_t count;
    uint32_t selected_index;
    uint32_t error_resource;
} CrtDisplayResolutionList;
void display_resolution_entries_reset(int enumeration_succeeded);
int  display_resolution_entry_append(const char* name, uint32_t flags);
int  display_resolution_entry_count(void);
int  display_resolution_entry_get(int index,
                                  CrtDisplayResolutionEntry* out);
void display_resolution_combo_populate(CrtDisplayResolutionList* out);

/* The filter table mirrors DAT_00568060/64/68. Its 16-entry guard is a
 * documented safety deviation from the corpus's blind write. */
void display_resolution_filter_tuple_append(int width, int height, int bpp);
int  display_resolution_filter_tuple_count(void);
void display_resolution_filter_tuple_get(int index, int* w, int* h, int* bpp);
int  display_mode_matches_filter_tuple(int mode_index);

/* --- the remaining unowned DAT one-liners (2026-10-24; finding 383) --------
 * GlobalFlagGet_004ff978 (FUN_00456640): the unresolved flag the corpus
 * gates a caller on; zero-init static, no ported feeder.
 * GlobalValueSet_004ffe48 (FUN_00458660): the unresolved store's word. */
uint32_t global_flag_004ff978_get(void);
void     global_value_004ffe48_set(uint32_t value);

/* --- the unowned DAT word-store band (2026-10-24; findings 37/256/392/419) -
 * Corpus leaves storing or toggling globals with no ported owner/feeder;
 * each lands as a zero-initialized module static per the s_system_state
 * precedent, with port-side peek hooks standing in for the unported
 * corpus readers. DAT_004b31bc is the advisor controller state word
 * (findings 37/38 — its readers live in the blocked advisor family);
 * DAT_00557860 is the task-refresh flag the 0x407340/60 pair toggles and
 * reads (finding 392); the rest are the unresolved or config-domain
 * stores below. */
uint32_t global_flag_bit0_is_set(void);            /* FUN_00401c20 */
void     state_flag_toggle_when_zero(void);        /* FUN_00407340 */
uint32_t global_task_refresh_flag_get(void);       /* FUN_00407360 */
void     level_state_stored_value_set(uint32_t v); /* FUN_00409d40 */
void     global_mode_zero_input_set(int value);    /* FUN_0040ab10 */
uint32_t global_mode_zero_input_peek(void);         /* port hook */
void     rockfall_style_table_base_set(uint32_t v);/* FUN_0040bfc0 */
void     global_flag_bit0_clear(void);             /* FUN_0040e7e0 */
void     global_config_value_set(uint32_t v);      /* FUN_0040f5e0 */
uint32_t advisor_state_word_peek(void);            /* port hook */
void     advisor_state_word_set(uint32_t v);       /* port hook */
uint32_t level_stored_word_peek(void);             /* port hook */
uint32_t mode_zero_input_flag_peek(void);          /* port hook */
uint32_t rockfall_table_base_peek(void);           /* port hook */
uint32_t ui_flag_word_peek(void);                  /* port hook */
uint32_t global_config_word_peek(void);            /* port hook */

/* --- display dimensions + request-record predicates (2026-10-24) -----------
 * DisplayWidthGet (FUN_00401c30) / DisplayHeightGet (FUN_00401c40): the window-mode
 * extent pair the renderer's window apply stores (finding 172) — the
 * renderer side is unported, so zero-init statics with setters stand in.
 * The request-record predicates (FUN_0045ab30/60/90) AND the flag word of
 * the 18 0x30-stride request records in the corpus's DAT_0050013c block
 * (panel layer unported): a zero-init flag-word window, corpus-blind
 * indexing guarded to 18 entries (deviation), port-side setter for the
 * unported record writers. */
int  display_width_get(void);
int  display_height_get(void);
void display_dimensions_set(int w, int h); /* port hook */
int  request_record_ready_flag4(int index);   /* FUN_0045ab30 */
int  request_record_ready_flag2(int index);   /* FUN_0045ab60 */
int  request_record_flag8_is_set(int index);  /* FUN_0045ab90 */
void request_record_flags_set(int index, uint32_t flags); /* port hook */
uint32_t request_record_flags_peek(int index); /* port hook */

/* --- the remaining DAT one-liner stores/reads (2026-10-24; finding 392) -----
 * GlobalValueSet_0050080c (FUN_0045c4a0): the unresolved store's word.
 * IndexedGlobalValueGet_00500a9c / GlobalValueGet_00500a98
 * (FUN_0045cc10 / FUN_0045cc60): the font-cursor words - the corpus's
 * table reads whose feeder (the font slot loader) is unported, so both
 * land as zero-init storage with the corpus's stride-1 index math
 * guarded to a documented 16-entry window (deviation). */
void     global_value_0050080c_set(uint32_t value);
uint32_t global_value_0050080c_peek(void);            /* port hook */
uint32_t indexed_global_value_at_get(int index);      /* FUN_0045cc10 */
uint32_t global_cursor_word_get(void);                /* FUN_0045cc60 */
void     font_cursor_word_set(int index, uint32_t v); /* port hook */

/* --- pure-return stubs + advisor word clear (2026-10-24; findings 37/38/393) -
 * The corpus keeps several address-range stubs that only return: the FLIC
 * opcode-27 fallback and the two CRT variants land as faithful no-op
 * functions. AdvisorClearActiveState (FUN_00401b20) clears bit 1 of the
 * advisor state word this module's band models. */
void crt_no_op_stub(void);              /* FUN_00491ff5 */
void crt_no_op_stub_variant(void);      /* FUN_00497870 */
void advisor_clear_active_state(void);  /* FUN_00401b20 */

/* ByteRangeSwap (FUN_0048e030): byte-for-byte swap of two ranges; the
 * corpus's same-pointer shortcut keeps (documented NULL guard). */
void byte_range_swap(void* a, void* b, size_t n);

/* --- the DAT window band (2026-10-24; findings 250/252/357) -----------------
 * Indexed pair/pointer/float windows over the corpus's data-section
 * tables. The corpus addresses (DAT_00500128/12c, DAT_005013a8…,
 * DAT_00552144/48, DAT_00552154, DAT_00555738/3c) have no ported owners
 * — the tables land as zero-initialized module statics with guarded
 * entry windows and port-side hooks standing in for the unported panel/
 * view writers (unowned-global convention; deviations documented). */
void indexed_pair_set(int index, uint32_t x, uint32_t y);
                                        /* FUN_0045a6e0 */
void indexed_pair_get(int index, uint32_t* out_x, uint32_t* out_y);
                                        /* FUN_0045a700 */
void indexed_float_pair_set(int index, uint32_t x, uint32_t y);
                                        /* FUN_00463730 */
void indexed_float_pair_peek(int index, float* out_x, float* out_y); /* port hook */
void indexed_text_metrics_pair_set(int index, uint32_t x, uint32_t y);
                                        /* FUN_00463ed0 */
void indexed_text_metrics_pair_get(int index, uint32_t* out_x,
                                   uint32_t* out_y); /* FUN_00463ea0 */
void indexed_record_pointer_set(int index, void* record); /* port hook */
void* indexed_record_pointer_peek(int index);
void region_bounds_set(const uint32_t* xy_pair, uint32_t w, uint32_t h);
                                        /* FUN_0045d370 */
void region_bounds_peek(uint32_t* out_xy, float* out_wh);
void panel_draw_records_set(int index, void* records); /* port hook */
void* panel_draw_records_peek(int index);
void panel_item_flag16_set(int panel, int item, int value);
                                        /* FUN_0045aac0 */
void panel_item_flag32_set(int panel, int item, int value);
                                        /* FUN_0045abb0 */
void panel_state_flags_toggle(int index); /* FUN_0045ab00 */
void reward_score_add(float value);      /* FUN_00462800 */
float reward_score_total_peek(void);
uint32_t reward_score_count_peek(void);

/* --- the panel/view blit band (2026-10-24; findings 250/252/256) ------------
 * The view blit block (the DAT_00552140/14c/150/158 windows, stride 0x20 per
 * view) has no ported owner — zero-init windows with hooks. The blit leaves
 * reach the corpus's surface seam (already cited as ported); the port models
 * that seam with the recorder hooks here: loads hand back monotonic handles
 * and every surface call is recorded.
 * ViewBlitRectPositionsApply (FUN_00463d20): compose the view origin from
 * the float-pair window plus the region-indexed pair window, blit the view
 * handle, walk the two rect records for their conditional size-pair blits,
 * and finish through the metrics record's blit.
 * PanelItemFlag8SetIfEnabled (FUN_0045abf0): the item word's bit 8 only
 * follows the flag while bit 32 is set (the corpus's enable gate).
 * PanelTextureSlotsAllocate (FUN_0045ac30): build the panel draw-record
 * block — count entries of 0x2c through the engine heap, gathered per the
 * slot-index array, with the three texture streams loaded through the
 * modeled seam. The corpus overwrites the panel's previous block without
 * freeing it (faithful). */
void view_blit_rect_positions_apply(int view); /* FUN_00463d20 */
void panel_item_flag8_set_if_enabled(int panel, int item, int value);
                                        /* FUN_0045abf0 */
void panel_texture_slots_allocate(int panel, int count, const int* slot_index,
                                  const float* stream, const int* tex1,
                                  const int* tex2, const int* tex3,
                                  const int* ids); /* FUN_0045ac30 */
void view_blit_window_set(int view, uint32_t handle, void* rect_a,
                          void* rect_b, int region_index); /* port hook */
void view_blit_window_peek(int view, uint32_t* handle, void** rect_a,
                           void** rect_b, int* region_index);
void panel_draw_count_set(int panel, int count);  /* port hook */
int  panel_draw_count_peek(int panel);            /* port hook */
/* The modeled surface seam: monotonic load handles + a recording buffer.
 * Blits record (handle, x, y); transforms record (handle, 0, 0). Handles
 * live as uint32 bits; the float-valued window fields carry them
 * bit-for-bit the way the corpus's float "handles" do. */
uint32_t crt_surface_handle_next(void);
void     crt_surface_blit_record(uint32_t handle, float x, float y);
/* The same seam with the corpus's optional rect witnesses: rect1 is the
 * source rect (four floats, absent when the corpus passes NULL), rect3
 * the size pair. crt_surface_call_get_rect answers the witnesses for a
 * recorded call (presence flags required, data optional). */
void     crt_surface_blit_record_rect(uint32_t handle, float x, float y,
                                      const float* rect1,
                                      const float* rect3);
int      crt_surface_call_get_rect(int index, float* rect1, int* rect1_set,
                                   float* rect3, int* rect3_set);
int      crt_surface_calls_count(void);
int      crt_surface_call_get(int index, uint32_t* handle, float* x,
                              float* y);
void     crt_surface_calls_clear(void);
float    crt_float_bits(uint32_t bits);

/* --- the small global primitives band (2026-10-24; findings 245/388/417) ----
 * One-liner engine helpers whose corpus globals have no ported owner —
 * zero-initialized module statics with hooks per convention.
 * UInt96Zero (FUN_00497aa0): zero the three limbs (the sibling is already
 * ported). GlobalValueClearIfEqual (FUN_0040e910): the screen-effect
 * handle word clears only when the incoming value equals it (the corpus's
 * stale-handle release rule). LinkedListPushAtGlobalHead /
 * GlobalRecordListPushHead (FUN_0046a470/FUN_004792c0): push a record at
 * the head of the corpus's two global lists — the link lands at +0x1c and
 * the heads land here. SceneEffectRegistryReset (FUN_00465380): clear the
 * transient-effect registry head/tail/texture words and answer 1.
 * CrtFatalMessageExit (FUN_00498340): the corpus's exhausted-memory fatal
 * path (`__amsg_exit` 2). The port aborts like the CRT unless an observer
 * hook stands in (documented deviation). */
void uint96_zero(uint32_t limbs[3]);   /* FUN_00497aa0 */
void uint96_shift_right(uint32_t limbs[3], int count); /* FUN_00497ad0 */
void global_value_clear_if_equal(int value); /* FUN_0040e910 */
void global_value_word_set(int value);       /* port hook */
int  global_value_word_peek(void);
void linked_list_push_at_global_head(void* record); /* FUN_0046a470 */
void* linked_list_global_head_peek(void);           /* port hook */
void  linked_list_global_head_set(void* record);      /* port hook */
void global_record_list_push_head(void* record);    /* FUN_004792c0 */
void* global_record_list_head_peek(void);           /* port hook */
int  scene_effect_registry_reset(void);             /* FUN_00465380 */
void scene_effect_registry_words_peek(uint32_t out_words[3]);
void crt_fatal_message_exit(void);                  /* FUN_00498340 */
void crt_fatal_exit_observer_set(void (*observer)(void));

/* CrtNoOpStubVariant2 (FUN_00493c40): the corpus's second no-op CRT stub
 * variant — an empty body, ported faithfully as a call-through. */
void crt_no_op_stub_variant2(void); /* FUN_00493c40 */

/* LinkedRecordChainFree (FUN_0046a370): frees a record chain recursively —
 * the successor chain through the +1c link (the port's widened link slot,
 * same deviation as the pushes), then the record itself through the engine
 * heap free. GlobalLinkedRecordChainClear (FUN_0046a390): clears the
 * auxiliary global word (DAT_004aa450, unowned, zero-init hook), frees the
 * chain at the global head, and clears the head. */
void linked_record_chain_free(void* record);  /* FUN_0046a370 */
void global_linked_record_chain_clear(void);  /* FUN_0046a390 */
void* linked_list_global_aux_peek(void);      /* port hook */
void  linked_list_global_aux_set(void* word); /* port hook */

/* ReleaseLinkedObjectList (FUN_00479240): frees the list record's field-0
 * owner word through the engine heap, then recycles the chain — linked
 * through record field 5, a corpus slot the port keeps as a documented
 * widened link slot — by pushing every node onto the global record list
 * through the ported push. Deviation: the corpus strides the chain
 * blindly; the port guards the record pointer. */
void linked_object_list_release(void* list);          /* FUN_00479240 */

/* ModelRenderListClear (FUN_00482d30): walks the corpus's model-render
 * list head (unowned global, zero-init storage with hooks) freeing each
 * node through the engine heap, following the successor at node +0x44,
 * then clears the head. CrtOwnedStreamBufferRelease (FUN_00494df0): frees
 * a stream record's owned buffer when the flag word raises any of bits
 * 0/1/7 together with bit 3, then clears bits 3 and 11 and zeroes the
 * pointer words (deviation: the port lays the corpus dword field window
 * out as host words — field n at record + n*sizeof(void*), flags last;
 * NULL record guarded — corpus strides blindly). */
void model_render_list_clear(void);                 /* FUN_00482d30 */
/* RendererCommandNodePrepend (FUN_00482ce0): zero a widened host-word
 * node window, store the command kind word, copy sixteen payload dwords
 * when a payload is given (zeros otherwise), link the old list head into
 * the corpus's +0x44 widened slot, and raise the head. Deviations: the
 * node allocation is widened for the host-word layout (the
 * CrtOwnedStreamBufferRelease convention; the corpus allocates 0x48),
 * the alloc guard (the corpus strides it blindly). Shares the
 * ModelRenderListClear head storage + hook. */
void renderer_command_node_prepend(uint32_t kind, const uint32_t* payload);
void model_render_list_head_set(void* node);        /* port hook */
void* model_render_list_head_peek(void);            /* port hook */
void crt_owned_stream_buffer_release(char* record); /* FUN_00494df0 */

/* SharedTextureDirectorySet (FUN_004728c0, findings 420/13): duplicates the
 * argument string and installs it as the shared texture directory word
 * (DAT_0076cd88 — no ported owner, so it lands here as a NULL-init static
 * with hooks). Deviation: the corpus overwrites without freeing the prior
 * copy (leak), reproduced faithfully; tests free the last copy through the
 * peek hook. */
void shared_texture_directory_set(const char* path); /* FUN_004728c0 */
char* shared_texture_directory_peek(void);           /* port hook */
void  shared_texture_directory_clear(void);          /* port hook: free */

/* --- scene hook + renderer flag-word band (2026-10-25) ----------------------
 * The scene-end/sample hook register pair and the renderer flag word live in
 * the DAT_0076cd74..94 window: no ported owner, so the words land as
 * zero-init module statics with hooks. The corpus initializes the flag word
 * to 1 (the kind-table initializer, whose port models the table only); the
 * deviation matters nowhere in the port because the flag word's only corpus
 * reader — the hook dispatch in the scene frame-callback band — is the
 * renderer seam. */

/* RendererFlagBit6Set (FUN_004728e0): sets or clears bit 0x40 in the
 * renderer flag word. */
void renderer_flag_bit6_set(int value);             /* FUN_004728e0 */
uint32_t renderer_flag_word_peek(void);             /* port hook */

/* DDiSceneEndHookRegister (FUN_00472900): stores the scene-end callback
 * pair — the corpus strides its code-pointer globals blindly; the port
 * stores host pointers. */
void scene_end_hook_register(void* hook, void* value); /* FUN_00472900 */
void* scene_end_hook_peek(void);                       /* port hook */
void* scene_end_hook_value_peek(void);                 /* port hook */

/* DDiSceneSampleHookRegister (FUN_00472920): stores the sample callback
 * pair and raises the flag word's bit 0x40 alongside it. */
void scene_sample_hook_register(void* hook, void* value); /* FUN_00472920 */
void* scene_sample_hook_peek(void);                       /* port hook */
void* scene_sample_hook_value_peek(void);                 /* port hook */

/* --- string registry (2026-10-25) ------------------------------------------
 * The DAT_004db8cc string registry has no other corpus owner: the intern is
 * its sole writer, so the head rides a module static. Deviation: the corpus
 * entry is a sixteen-byte record of dwords (string at field 0, two cache
 * handles, next link at field 3 = entry+0xc); the port carries host-width
 * slots, and its case-insensitive compare folds ASCII only where the
 * corpus's __strcmpi follows the locale. */

/* StringKeyedCacheEntryFind (FUN_00410450): the case-insensitive walk over
 * the registry; answers the entry or NULL. */
void* string_keyed_cache_entry_find(const char* key); /* FUN_00410450 */

/* StringRegistryIntern (FUN_00410490): interning — a null-or-empty key
 * answers NULL, a known key answers its entry, and a miss appends a new
 * entry (key copy through the heap seam) at the head. */
void* string_registry_intern(const char* key);        /* FUN_00410490 */
void  string_registry_reset(void);                    /* port hook: free */

/* --- entry cache get-or-load pair (2026-10-25) ------------------------------
 * The corpus loaders ride seam cites (the port models the surface-load path
 * in the texture module); here the loader call is a hook-boundary seam that
 * answers NULL with no hook installed (load-answers-NULL precedent). The
 * loaded object is unowned: registry reset drops the slots with the entry
 * (the corpus leaks them too). */

/* StringCacheLookupOrLoad (FUN_00410520): intern the key and return the
 * entry's bitmap-slot value, loading once through the seam on a miss. */
void* string_cache_lookup_or_load(const char* key);   /* FUN_00410520 */

/* ResourceNameCacheGetOrLoad (FUN_00410560): the same for the entry's
 * font-slot value. */
void* resource_name_cache_get_or_load(const char* key); /* FUN_00410560 */

/* port hooks: install the loader seams (NULL uninstalls, answers NULL). */
void string_cache_bitmap_loader_set(void* (*hook)(const char*));
void string_cache_font_loader_set(void* (*hook)(const char*));

/* --- pair-table window pair (2026-10-25) -----------------------------------
 * CaseInsensitivePairTableFind (FUN_00488730) / CaseInsensitivePairTableAppend
 * (FUN_00488790): a caller-supplied {key, value} window; entries ride as
 * widened slots (the corpus strides its 8-byte pair entries by 4-byte
 * fields). Quirk pinned: the corpus bounds its scan with the walk index
 * masked to its low 16 bits. Deviations: the port guards the table, the
 * key, and the count pointer. */
void* pair_table_find(const void* table, uint32_t count, const char* key);
                                                /* FUN_00488730 */
void pair_table_append(void* table, int* count, const char* key,
                       void* value);             /* FUN_00488790 */

/* SelectionMarkerResourcesRelease (FUN_0040ce30, findings 378/434): frees
 * each non-null pointer in the marker resource table (DAT_004c7e00..e7c,
 * the trailing count word reset), then frees the table descriptor
 * (DAT_004c7df8) when present. The table/descriptor have no ported owner,
 * so they land here as zero-init statics with install hooks. */
void selection_marker_resources_release(void);     /* FUN_0040ce30 */
void selection_marker_slot_set(int index, void* ptr); /* port hook */
void* selection_marker_slot_peek(int index);          /* port hook */
void selection_marker_descriptor_set(void* desc);     /* port hook */
void* selection_marker_descriptor_peek(void);         /* port hook */
void selection_marker_count_set(uint32_t count);      /* port hook */
uint32_t selection_marker_count_peek(void);           /* port hook */

/* OwnedResourceTreeRelease (FUN_0048c180, findings 386/438): releases the
 * owned resource tree — the +0 payload, the +8 payload, the recursive
 * right child at +4 (owning its node), and the node itself when owned.
 * Deviation: the corpus strides dwords (x86 handles); the port reads the
 * slots widened and guards the NULL node. */
void owned_resource_tree_release(void** node, int own_node); /* FUN_0048c180 */

/* --- string + stream allocation leaves (2026-10-24; findings 375/93) ------
 * StringPrefixReserve (FUN_004907c0): when `count` is nonzero the NUL-
 *   terminated string is shifted right by `count` bytes in place through
 *   the ported memory move — the vacated prefix is where the caller writes
 *   (the corpus's scan is a strlen; the tail NUL rides the move).
 * CrtStreamBufferAllocate (FUN_00498ae0): bumps the stream-allocation
 *   counter (DAT_005493e4, unowned: hooked static), tries the engine heap
 *   for a 0x1000 buffer, and installs it as the record's buffer window
 *   with flag bit 3 when it lands; on failure the record's own internal
 *   buffer takes over with flag bit 2 raised instead. The record's dword
 *   window follows the original seven-DWORD x86 layout; 64-bit builds use
 *   native pointer-sized fields for the port's stream-record layout. */
void string_prefix_reserve(char* s, int count);   /* FUN_004907c0 */
void crt_stream_buffer_allocate(char* record);    /* FUN_00498ae0 */
long stream_allocation_count_peek(void);          /* port hook */
void stream_allocation_count_reset(void);         /* port hook */

/* --- the 96-bit add (2026-10-24; findings 375/102) --------------------------
 * UInt96Add (FUN_00499fe0): adds the three-limb value `src` onto `dst`
 *   through the ported carry add. Quirk pinned from the pseudocode: the
 *   limb-0 carry chain bumps the limb-1 word with +1 and, if that itself
 *   carries, the limb-2 word; the limb-1/limb-1 add follows with the
 *   (possibly bumped) limb word, bumping limb 2 again on its carry; the
 *   limb-2 add's carry out is discarded.
 * Deviation: the corpus strides both limbs blindly; the port guards them. */
void crt_uint96_add(uint32_t* dst, const uint32_t* src); /* FUN_00499fe0 */
/* FUN_0049a0b0: convert decimal digit bytes into a normalized 96-bit
 * significand and its signed exponent word at byte offset 10. */
void crt_decimal_digit_sequence_to_extended96(const char* digits, int count,
                                              void* out_extended96);
/* FUN_00497f40: expand the IEEE double bit pattern into the runtime's
 * 10-byte extended80 representation in a 12-byte temporary. */
void crt_double_to_extended80(void* out_extended80, const void* source_double);
void crt_extended80_multiply(uint16_t* destination,
                             const uint16_t* source); /* FUN_0049a540 */
/* UInt96IncrementAtBit (FUN_00497970): the bit-indexed add. Adds the
 * single-bit mask 1 << (31 - bit % 32) into limb bit / 32 (the corpus's
 * sign-biased index arithmetic reproduced) and propagates the carry toward
 * the LOWER indices while the chain has it — this function's own limb
 * order, not uint96 add's little-endian model.
 * Deviation: the corpus strides the limbs blindly (the port guards 0..2). */
void crt_uint96_increment_at_bit(uint32_t limbs[3], int bit);
                                                /* FUN_00497970 */
/* UInt96HasNoBitsAtOrAbove (FUN_00497900): 1 when no bit at or above the
 * index is set. The corpus's in-limb mask tests the bits ABOVE the named
 * bit (1 << (31 - bit % 32) is the named bit itself, the complement mask
 * clears it), then requires the higher limbs all zero. Deviation: the
 * corpus strides the limbs blindly (the port guards 0..2, NULL reads
 * "all clear"). */
uint32_t crt_uint96_has_no_bits_at_or_above(const uint32_t limbs[3], int bit);
                                                /* FUN_00497900 */
/* UInt96ClearFromBit (FUN_004979e0): when the named bit is set AND some
 * bit at or above bit + 1 exists, first bumps bit - 1 with the increment
 * chain; then keeps the at-or-above bit half of the limb and zeroes every
 * higher limb. The value carried through local_4 is the increment's final
 * carry bit; it reports when rounding overflowed the top of the 96-bit word. */
uint32_t crt_uint96_clear_from_bit(uint32_t limbs[3], int bit);
                                                /* FUN_004979e0 */

/* --- the renderer-word band (2026-10-25) -------------------------------------
 * Six renderer/global-word one-liners over the corpus's unowned .data band:
 * the port models each word as a zero-init module static (the corpus's
 * writers are unported), so every getter answers its installed half. */

/* GlobalPointerGet variants: the three global words (see crt.cpp) —
 * FUN_00410b30, FUN_00410b40, FUN_00410b50. */
uint32_t renderer_word_a_get(void);
uint32_t renderer_word_b_get(void);
uint32_t renderer_word_c_get(void);
extern uint32_t g_renderer_word_c;      /* DAT_0076ab3c (renderer word C) */
void     renderer_words_set(uint32_t a, uint32_t b, uint32_t c); /* install */

/* GlobalDwordGet (FUN_004065d0): the DAT_0076ac84 global dword. */
uint32_t renderer_global_dword_get(void);
void     renderer_global_dword_set(uint32_t value); /* install */

/* RenderSurfaceSecondaryGet (FUN_004065e0): the DAT_0076ac8c surface word. */
uint32_t render_surface_secondary_get(void);
void     render_surface_secondary_set(uint32_t value); /* install */

/* RenderFeatureFlagsGet (FUN_00477f70): the DAT_0050608c feature word. */
uint32_t render_feature_flags_get(void);
void     render_feature_flags_set(uint32_t value); /* install */

/* Engine feature word band (2026-10-25): the DAT_00505f84 half-model lives
 * here with install/peek hooks (see crt.cpp; the EngineContext::feature
 * half in engine.hpp does not interoperate — deviation). The corpus's
 * low-texture flag setter turns on bit 5 while bit 0x40000 is clear. */
uint32_t engine_feature_word_peek(void);
void     engine_feature_word_install(uint32_t value); /* port hook */
void     low_texture_flag_set(void);                  /* FUN_00477bd0 */

/* --- integer/string helper band (2026-10-25) -------------------------------
 * UInt96Copy (FUN_00497a80): copies the three dwords src→dst in ascending
 * order — the corpus's dword walk reads each source word after earlier
 * writes, so an overlapping source shows the same word duplication (the
 * faithful model is the ordered copy). */
void uint96_copy(uint32_t dst[3], const uint32_t src[3]);

/* CrtExtended80ToBinaryFloat (FUN_00497b90) and its fixed format wrappers.
 * Source is the CRT's six-word 80-bit record; outputs are binary32/binary64
 * words. Status is 0 for finite, 1 for overflow, and 2 for underflow. */
uint32_t crt_extended80_to_binary_float(const uint16_t* source,
                                        uint32_t* destination,
                                        const int32_t* format);
uint32_t crt_extended_float_convert_format_a(const uint16_t* source,
                                             uint32_t* destination);
uint32_t crt_extended_float_convert_format_b(const uint16_t* source,
                                             uint32_t* destination);
uint32_t crt_double_to_extended_float_convert_a(uint32_t* destination,
                                                double value);
uint32_t crt_double_to_extended_float_convert_b(uint32_t* destination,
                                                double value);

/* CrtStringDuplicate (FUN_0049b660): strlen+1 through the engine
 * allocator, bytes and NUL copied over. A null string answers null
 * (deviation: the corpus's strlen strides it blindly). */
char* crt_string_duplicate(const char* text);

/* CrtEnvironmentArrayClone (FUN_0049b540): allocate a null-terminated
 * pointer array and duplicate each entry through the engine heap. */
char** crt_environment_array_clone(char** entries);

/* CRT process-environment support (FUN_00495df0 / 00496190 / 0049ae10 /
 * 0049af20 / 0049af60 / 0049afe0 / 0049b2b0 / 0049b4c0). The environment
 * block and strings use engine-heap ownership, matching the CRT startup
 * and _putenv paths. CompareString compatibility returns the Win32 ordering
 * values 1/2/3, or zero on API failure. */
void crt_environment_pointer_array_build(void);
char* crt_environment_block_get(void);
char* crt_environment_variable_lookup(const char* name);
int crt_counted_string_compare(const char* left, const char* right, int count);
int crt_wide_environment_string_list_convert(void);
int crt_compare_string_compat(uint32_t locale, uint32_t flags,
                              const char* left, int left_count,
                              const char* right, int right_count,
                              uint32_t codepage);
int crt_put_env_update(char* text, int sync_process);
int crt_environment_entry_find(const char* name, int name_length);

/* CrtCommandLineArgumentScan (FUN_00495f80): parse argv[0] and the remaining
 * command line. Null output pointers request the sizing pass; counts include
 * the trailing null argv slot and all string terminators. */
void crt_command_line_argument_scan(const unsigned char* command_line,
                                    char** argv_out,
                                    unsigned char* string_out,
                                    int* argc_and_null_count,
                                    int* string_byte_count);

/* CrtDecimalMantissaRoundAndNormalize (FUN_00497e20): copy/pad and round a
 * decimal digit run, adjusting its exponent on carry into the guard digit. */
struct CrtDecimalFormatContext {
    int32_t first;
    int32_t exponent;
    char* locale_result;
    char* digits;
};
void crt_decimal_mantissa_round_and_normalize(char* output, int precision,
                                               CrtDecimalFormatContext* context);
void crt_extended_float_power_of_ten_scale(uint16_t value[6],
                                           uint32_t exponent,
                                           int initialize); /* FUN_0049a800 */
uint32_t crt_extended_float_to_decimal(uint32_t significand_low,
                                       uint32_t significand_high,
                                       uint16_t sign_exponent,
                                       int precision,
                                       uint8_t flags,
                                       uint8_t* output); /* FUN_0049a1b0 */
CrtDecimalFormatContext* crt_locale_data_get_or_initialize(
    uint32_t double_low, uint32_t double_high); /* FUN_00497ec0 */
char* crt_scientific_float_format_append(const uint32_t* value,
                                         char* output,
                                         int precision,
                                         int uppercase); /* FUN_00490400 */
char* crt_fixed_float_fraction_format(const uint32_t* value,
                                      char* output,
                                      uint32_t precision); /* FUN_00490540 */
void crt_float_decimal_format_prepare(const uint32_t* value,
                                      char* output,
                                      int precision,
                                      int uppercase); /* FUN_00490640 */
void crt_scientific_float_format_guarded(const uint32_t* value,
                                         char* output,
                                         int precision,
                                         int uppercase); /* FUN_004906f0 */
void crt_fixed_decimal_format_guarded(const uint32_t* value,
                                      char* output,
                                      uint32_t precision); /* FUN_00490720 */

/* CrtCharacterClassFlagTest (FUN_00495db0): test the locale byte flags,
 * then the base CRT character-class flags for the low input byte. */
uint32_t crt_character_class_flag_test(uint32_t value, uint32_t flags,
                                      uint8_t byte_mask);
uint32_t crt_is_dbcs_lead_byte(uint32_t value); /* FUN_00495d90 */
uint32_t crt_character_classify_mask(int value, uint32_t mask); /* FUN_0048f1d0 */
int crt_get_string_type_a_compat(uint32_t info_type, const char* source,
                                 int source_count, uint16_t* result,
                                 uint32_t codepage, uint32_t locale);
int crt_locale_character_map_string(uint32_t locale, uint32_t flags,
                                     const char* source, int source_count,
                                     void* destination,
                                     int destination_count,
                                     uint32_t codepage);
uint32_t crt_multibyte_to_wide_char(uint16_t* output,
                                    const uint8_t* source,
                                    uint32_t max_bytes);
uint32_t crt_character_class_flag8_check(int value);
int crt_locale_codepage_table_update(int codepage);
void crt_ansi_codepage_tables_init(void);
uint32_t crt_character_to_lower_locale(uint32_t value);

/* CrtCodePageLocaleIdLookup (FUN_00496530): map the four CRT DBCS code
 * pages to their Windows locale identifiers, or return zero. */
uint32_t crt_codepage_locale_id_lookup(uint32_t codepage);
int crt_codepage_get(int codepage); /* FUN_004964e0 */
int32_t crt_calendar_fields_to_time_scalar(int year, int month, int day,
                                            int hour, int minute, int second,
                                            int daylight_mode); /* FUN_00496f50 */
void crt_timezone_initialize_once(void); /* FUN_00499860 */
void crt_timezone_state_init(void); /* FUN_00499880 */
int crt_daylight_saving_time_check(const int* calendar_fields); /* FUN_00499b30 */
void crt_daylight_transition_boundary_compute(int is_start,
                                              int use_nth_weekday,
                                              uint32_t year,
                                              int month_index,
                                              int week,
                                              int weekday,
                                              int fixed_day,
                                              int hour,
                                              int minute,
                                              int second,
                                              int millisecond); /* FUN_00499da0 */

/* CrtCtypeTablesReset (FUN_00496590): clear the active multibyte/code-page
 * state while preserving the base _pctype table and OS-fallback flag. */
void crt_ctype_tables_reset(void);

/* CrtMultibyteCharacterSearch (FUN_0049b5b0): find a byte or DBCS pair in
 * a zero-terminated string using the active CRT code-page tables. */
char* crt_multibyte_character_search(char* text, uint32_t value);
int crt_parse_signed_decimal(const unsigned char* text); /* FUN_0048dad0 */
void crt_normalize_decimal_point(char* text); /* FUN_004902b0 */
char* crt_strchr(char* text, int character); /* FUN_004914a0 */
char* crt_dbcs_aware_bounded_string_copy(char* dest, const char* src,
                                         uint32_t count); /* FUN_004976d0 */

/* CrtUnsignedIntegerFormatRadix (FUN_0049dba0): digits least-significant
 * first, then reversed into the buffer; values ≥ 10 fold in through
 * `+ 'W'`, the corpus's lowercase letters. Its final flag requests a
 * leading minus and wrapping unsigned negation. The port guards radix < 2
 * with a lone "0" (deviation: the corpus's divide loop never ends). */
void crt_unsigned_format_radix(uint32_t value, char* out, int radix,
                               int signed_value);

/* SignedDecimalToStringConvert (FUN_0049db60): radix 10 over a negative
 * value takes the sign half ('-' + unsigned magnitude, wrapping like the
 * corpus — INT_MIN lands as 2147483648); every other case is the plain
 * unsigned format. Answers the buffer. */
char* crt_signed_to_string(int value, char* out, int radix);

/* StringToUpper / StringToLowercase (FUN_0049dcd0 / FUN_0049dc10): the
 * corpus's C-locale half (DAT_00549180 == 0, the port's pinned locale
 * per KeyToAscii) folds A-Z / a-z in place; the locale half rides the
 * unported locale-map chain (recorded). The conversion covers the whole
 * string; the return is the buffer. Deviation: the corpus strides the
 * buffer blindly; the port guards the null pointer. */
char* crt_string_to_upper(char* text);   /* FUN_0049dcd0 */
char* crt_string_to_lower(char* text);   /* FUN_0049dc10 */

/* --- math error band (2026-10-25) ------------------------------------------
 * CrtMathErrorCodeSet (FUN_004939a0): stores the corpus's _DAT_005490d0
 * math-error slot — parameter 1 stores 0x21, parameters 2 and 3 store
 * 0x22, everything else leaves the slot. The port models the slot as
 * guarded own storage (the corpus's fd/codec readers ride the unported
 * CRT runtime band — deviation). */
void crt_math_error_code_set(int param);
uint32_t crt_math_error_code_peek(void); /* port hook */

/* FPU control/status helpers (FUN_00493be0 / 0x493bf0 / 0x493c10).
 * The status readers return the signed x87 status word. The masked control
 * update writes (value & mask) | (old & ~mask) and returns the previous
 * signed control word. */
int fpu_status_word_get_variant_a(void);
int fpu_status_word_get_variant_b(void);
int fpu_control_word_update_masked(uint32_t value, uint32_t mask);

/* CrtMathExceptionCodeFromStatus (FUN_00493a00): bit 0x20 answers 5,
 * bit 0x8 answers 1, bit 0x4 answers 2, bit 0x1 answers 3; otherwise it returns
 * the status low byte as the shipped `and eax,0xff` instruction does. */
int crt_math_exception_code_from_status(uint32_t status);

/* global_flag_004ff978_install: port hook writing the DAT_004ff978 word
 * the corpus's EngineModeSet stores through. */
void global_flag_004ff978_install(uint32_t value);

/* CrtMathExceptionHandlerGet (FUN_004939d0): scans the corpus's 27-entry,
 * 8-byte-stride math-handler table (DAT_004b2170 role; the corpus writer
 * is the unported CRT startup band — the port models the table as own
 * storage with an install hook). An id match answers the entry's handler
 * word, else zero. Deviation: the corpus strides the table blind. */
uint32_t crt_math_exception_handler_get(int id);
void crt_math_exception_handler_install(int index, int id,
                                        uint32_t handler); /* hook */

/* CrtInteger64Multiply (FUN_0048ea90, __allmul): the compiler's signed
 * 64-bit multiply helper — unsigned low-word product, with the two
 * cross-word signed products folded into the high half. */
uint64_t crt_integer64_multiply(uint32_t lo_a, int32_t hi_a,
                                uint32_t lo_b, int32_t hi_b);

/* CrtInteger64Divide / CrtUnsigned64Divide (FUN_0048e9e0 __alldiv /
 * FUN_00498bf0 __aulldiv): the compiler's 64-bit division helpers, both
 * word-pair shaped. The unsigned x86 path preserves the target's 32-bit
 * normalization, quotient estimate, and multiply-back correction; the
 * portable fallback uses equivalent word-pair arithmetic. The signed
 * helper folds its sign-normalizing algorithm to int64 division and pins
 * its wrapping INT64_MIN / -1 quotient. Both leave division by zero to the
 * native divide fault. */
uint64_t crt_integer64_divide(uint32_t lo_a, uint32_t hi_a,
                              uint32_t lo_b, uint32_t hi_b);
#if defined(_MSC_VER) && defined(_M_IX86)
uint64_t __stdcall crt_unsigned64_divide(uint32_t lo_a, uint32_t hi_a,
                                        uint32_t lo_b, uint32_t hi_b);
#else
uint64_t crt_unsigned64_divide(uint32_t lo_a, uint32_t hi_a,
                               uint32_t lo_b, uint32_t hi_b);
#endif

/* CrtFileDescriptorFlag40Get (FUN_00498b40): answers the corpus CRT table's
 * per-descriptor flag byte bit 0x40 — the byte at block+4 over 32
 * eight-byte entries per block, blocks indexed from the DAT_0076ddc0
 * pointer table, bounded by the DAT_0076dec0 count word. That table is
 * platform state with no ported owner: blocks and count ride zero-init
 * own storage behind the install/peek hooks below (the unowned-global
 * convention). The helper returns the masked value (0 or 0x40). */
unsigned crt_file_descriptor_flag40_get(unsigned fd);  /* FUN_00498b40 */
int crt_file_descriptor_handle_get(unsigned fd); /* FUN_004991f0 */
int crt_file_descriptor_allocate(void); /* FUN_00498fe0 */
int crt_file_descriptor_set_handle(unsigned fd, int handle); /* FUN_004990a0 */
int crt_file_descriptor_close_slot(unsigned fd); /* FUN_00499150 */
int crt_find_handle_close(int handle); /* FUN_0048f950 */
int crt_file_descriptor_close(unsigned fd); /* FUN_00494d10 */
int crt_standard_stream_buffer_install(int* stream); /* FUN_004974d0 */
void crt_stream_buffer_flush_and_release(int installed, int* stream); /* FUN_00497570 */
void crt_file_descriptor_table_init(void); /* FUN_004965d0 */
int crt_win32_file_descriptor_read(uint32_t fd, char* buffer, uint32_t count); /* FUN_00497130 */
uint32_t crt_stream_read_byte(int* stream); /* FUN_00497040 */
int crt_win32_file_descriptor_open(const char* path, uint32_t flags,
                                   uint32_t share, uint32_t attributes); /* FUN_00499240 */
int crt_file_descriptor_flag80_set_or_clear(unsigned fd, int mode);
void  crt_file_descriptor_flag_block_set(int index, void* block);
void* crt_file_descriptor_flag_block_peek(int index);
void  crt_file_descriptor_count_set(int count);
int   crt_file_descriptor_count_peek(void);
void crt_win32_error_map_to_errno(uint32_t error); /* FUN_00496ee0 */
uint32_t crt_win32_file_descriptor_seek(uint32_t fd, int32_t distance,
                                        uint32_t method); /* FUN_00496c00 */
int crt_win32_file_descriptor_write(uint32_t fd, char* buffer,
                                     uint32_t count); /* FUN_00496cc0 */
uint32_t crt_file_stream_close(uint32_t value, int* stream); /* FUN_004908a0 */
uint32_t crt_flush_stream_buffer(int* stream); /* FUN_0048fd20 */
void crt_buffered_character_write(uint32_t value, int* stream, int* written);
void crt_write_repeated_char(uint32_t value, int count, int* stream,
                             int* written);
void crt_write_string_bounded(const char* text, int count, int* stream,
                              int* written);

/* CrtStreamUngetByte (FUN_00498f50): puts one byte back into the stream's
 * input buffer when its flags permit it; returns the byte or -1. */
uint32_t crt_stream_unget_byte(uint32_t value, char* record);

/* CrtControlFlagsTranslate (FUN_004977d0): the file-flag fold — bits
 * 0/2/3/4/5 cross to 0x10/8/4/2/1, bit 1 pairs back as 0x80000, the bits
 * 10/11 pair folds 0x100/0x200/0x300 (zero stays), the bits 8/9 pair
 * answers 0x20000 when clear and 0x10000 when only 0x200, bit 12 answers
 * 0x40000. Pure bitwork. CrtWideCharToMultibyte (FUN_00498b70): one wide
 * char through the pinned C-locale half (the code-page branch rides the
 * unported locale-map chain — deviation); chars below 0x100 write one
 * byte and answer 1, anything else stores 0x2a in the math-error slot
 * and answers -1. A null destination answers 0 (pinned). */
uint32_t crt_control_flags_translate(uint32_t flags); /* FUN_004977d0 */
uint32_t crt_fpu_control_word_update(uint32_t value, uint32_t mask);
void crt_fpu_control_word_update_mask(uint32_t value, uint32_t mask);
int crt_wide_char_to_multibyte(char* dest, uint32_t wide);
                                                       /* FUN_00498b70 */

/* CrtStrncpy (library `_strncpy`, FUN_0048e770): copy at most `count`
 * source bytes into `dest`, copying the NUL when it lands and padding the
 * rest of the window with zeros. The corpus's aligned fast paths fold
 * into this behavior; the answer is the destination. */
char* crt_string_copy_n(char* dest, const char* src, uint32_t count);

/* CrtWideCharToMultiByteCompat (FUN_0049abe0): the wide->byte convert's
 * compat wrapper, pinned to the port's C-locale half (the locale half
 * rides the recorded locale-map chain, as in the code-page leaf). With
 * dest clear it answers the NUL-excluded wide length (WideStringLength);
 * with dest set it copies low bytes until the NUL (answering the length
 * so far) or the count (answering the count), and a wide char over 0xff
 * takes the corpus's EILSEQ-shaped math-error store, answering -1. */
int crt_wide_char_to_multi_byte_compat(char* dest, const uint16_t* wide,
                                       uint32_t count);

/* SignalHandlerRecordFind (FUN_0049ab90): finds the first matching signal
 * id in the ten initialized 12-byte records at DAT_004b22a8, answering the
 * record pointer or NULL. The count at DAT_004b2328 and table are mapped
 * globals; install/peek functions are port hooks. LP64 widens the handler
 * field and record stride to 16 bytes. */
void* crt_signal_handler_record_find(int signal);  /* FUN_0049ab90 */
void crt_signal_record_install(int index, uint32_t word0, int signal,
                               const void* handler);          /* hook */
void crt_signal_record_peek(int index, uint32_t* out_word0, int* out_id,
                            const void** out_handler);        /* hook */
void crt_signal_record_count_set(int count);                  /* hook */
int  crt_signal_record_count_peek(void);                      /* hook */

#endif /* LEGORRR_GODS98_CRT_HPP */

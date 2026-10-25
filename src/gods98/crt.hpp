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
 * ones (Inf/NaN) (findings 386/399; CrtAsin's pre-dispatch probe). */
uint32_t double_exponent_bits_or_special(uint32_t high_word);

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

/* FUN_00493a40 (DoubleBuildWithExponent, findings 383/399): rebuild the
 * 8-bit-exponent floating value from the two significand words and the
 * exponent bias input — the corpus's exact bit recipe: top word =
 * ((param_2 >> 16) & 0x800f) | ((param_3 + 0x3fe) << 4), significand =
 * (param_2 & 0xffff) : param_1. Ported as double (x87 deviation, as
 * above); the bits are built with integer arithmetic and bitcast. */
double double_build_with_exponent(uint32_t significand_low,
                                  uint32_t significand_high,
                                  int16_t exponent);

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

/* FlhMagicUuNoop (FUN_00456210, finding 440): the FLH magic 'UU'
 * pass's silent leaf — returns zero without changing state; the corpus
 * call sites (FLIC and CRT paths) do not reveal a further role. */
uint32_t flh_magic_uu_noop(void);

/* GlobalFloatField00556F00Get (FUN_004230b0, findings 295/392): reads
 * the global float at DAT_00556f00. Its meaning is unresolved — the
 * faithful read stays, storage stays static-backed (x87 deviation). */
double global_float_field_00556f00_get(void);

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

/* DisplayResolutionFilterTupleAppend (2026-10-24; findings 425/282): the
 * resolution-filter table append. The corpus writes blindly at
 * count * 0x110-stride slots of the DAT_00568060/64/68 trio; the port
 * keeps 16 guarded entries (documented deviation) — nothing in the port
 * seeds it (the seeding DirectDrawDeviceInitialize is a COM seam). The
 * read side, DisplayModeMatchesFilterTuple, stays blocked on the
 * display-mode enumeration globals. */
void display_resolution_filter_tuple_append(int width, int height, int bpp);
int  display_resolution_filter_tuple_count(void);
void display_resolution_filter_tuple_get(int index, int* w, int* h, int* bpp);

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
 *   window rides the port's stream-record layout (field n at record +
 *   n*sizeof(void*) — the crt_owned_stream_buffer_release convention).
 * Deviation: the corpus strides the record blindly; the port guards NULL. */
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
 * higher limb. The corpus returns FUN_00497970's undefined value through
 * local_4 — pinned 0 here (deviation), everything else faithful, guards
 * included. */
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

/* CrtStringDuplicate (FUN_0049b660): strlen+1 through the engine
 * allocator, bytes and NUL copied over. A null string answers null
 * (deviation: the corpus's strlen strides it blindly). */
char* crt_string_duplicate(const char* text);

/* CrtUnsignedIntegerFormatRadix (FUN_0049dba0): digits least-significant
 * first, then reversed into the buffer; values ≥ 10 fold in through
 * `+ 'W'`, the corpus's lowercase letters. The port guards radix < 2
 * with a lone "0" (deviation: the corpus's divide loop never ends). */
void crt_unsigned_format_radix(uint32_t value, char* out, int radix);

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

#endif /* LEGORRR_GODS98_CRT_HPP */

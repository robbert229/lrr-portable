// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game layer — entity table + `.ol` object list (findings 10/07/12).
 *
 * `EntityPointer` (corpus engine-region.c, 0x42ed70) keeps four typed
 * entity arrays: type 1 = vehicles (stride 0x1ec), type 2 = minifigures
 * (stride 0x74 — DAT_005561d8), type 3 (stride 0x74 — 005561dc),
 * type 4 = buildings/objects (stride 0x14c — 005561e0); the `.ol` object
 * list the LevelLoad `OListFile` key names (`M01.ol`) is a block-text file
 * whose top-level `name { … }` records describe placed objects.
 *
 * The corpus's `.ol` consumer (`FUN_0042e020`) and its type resolver
 * (`FUN_0042e8b0` = ObjectTypeByName) are decompiled and mirrored here: the
 * *grammar* is corpus-verified by `entity_test --real` against all 44
 * shipped object lists (root `Lego*`, one record per object named
 * `Object1..` / other names, fields `type <name>`, `xPos`, `yPos`,
 * `heading`, `health` (default 100), optional `driving <ref>`; `Model`
 * appears nowhere in the shipped data and is kept as the port's extension
 * key), and the *codes* are decompile-verified — 1..4 from the shipped
 * `Lego.cfg` `…Types` groups, 5..19 special kinds, TVCamera −1, 0 = not
 * registered.
 */
#ifndef LEGORRR_GAME_ENTITY_HPP
#define LEGORRR_GAME_ENTITY_HPP

#include "game.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Entity type constants (ObjectTypeByName FUN_0042e8b0 — decompile-verified).
 * 1..4 are the four typed tables (the `.ol`-placeable kinds); 5..19 are the
 * special-object kinds the corpus stores in per-kind singletons, which the
 * port groups under the shared special table. -1 = TVCamera (follow-camera
 * branch, no entity); 0 = unregistered. */
#define ENTITY_TYPE_VEHICLE  1
#define ENTITY_TYPE_MINIFIG  2
#define ENTITY_TYPE_OBJ      3   /* creatures/general objects (RockMonsterTypes) */
#define ENTITY_TYPE_BUILDING 4
#define ENTITY_TYPE_SPECIAL  5   /* first code of the special-object range */

/* A placed-object record produced by the `.ol` loader. */
typedef struct ObjectPlacement {
    char  name[64];
    int   type;          /* corpus code (1..19, -1 camera)           */
    char  type_name[24]; /* .ol `type` name (e.g. "SmallDigger")     */
    float x, y, z;       /* xPos, yPos, 0                            */
    float rot;           /* alias of heading                         */
    float heading;       /* .ol `heading` (degrees)                  */
    int   health;        /* .ol `health` (default 100)               */
    char  driving[32];   /* .ol `driving` target object name, or ""  */
    char  model[128];    /* optional `Model` key (port extension)    */
} ObjectPlacement;

/* Allocate a slot of `type`; returns the slot index or -1 when the table is
 * full or type is out of 1..4. */
int entity_create(GameContext* ctx, int type, const char* name,
                  float x, float y, float z, float rot);

/* Corpus strides by type (EntityPointer, finding 10 §"Kind registry" note). */
int  entity_stride(int type);
int  entity_count(GameContext* ctx, int type);
const EntitySlot* entity_at(GameContext* ctx, int type, int index);

/* EntityQueryFlagCount (FUN_0041b3c0): catalog-kind query over the stats
 * flag words and the own-storage catalog count/pair windows (install hooks
 * below; the corpus's loader-owned globals have no ported owner). */
int entity_query_flag_count(int query);           /* FUN_0041b3c0 */
void     catalog_count_install(int which, uint32_t value); /* port hook */
uint32_t catalog_count_peek(int which);           /* port hook */
void     catalog_name_table_install(int kind, const void* names); /* hook */
const void* catalog_name_table_peek(int kind);    /* port hook */
void     catalog_id_pair_install(int entry, uint32_t size); /* port hook */

/* CatalogCallbackByIdDispatch (FUN_0041b680): callback dispatch over the
 * catalog query lanes and the key-group code band shared with the resource
 * module's stride-8 window (the pair word is that window's count). */
int catalog_callback_by_id_dispatch(const GameContext* ctx, int query,
                                    int (*callback)(int, int, int, void*),
                                    void* payload);     /* FUN_0041b680 */

/* Resolver pair (FUN_0042e8b0, name -> (type, registry row)): table matches
 * write both outputs, the special names write only their dedicated rows.
 * The (name, &type, &row, NULL) corpus call form; NULL name answers 0. */
int entity_type_row_for_name(const char* name, int* type_out, int* row_out);

/* Load the level's `.ol` object list (`OListFile` key) from `cfg` into the
 * entity table. Returns the number of objects placed, or -1 when the
 * resource is missing/unparsable. */
int object_list_load(GameContext* ctx, BlockFile* cfg, const char* level_name);

/* Load a `.ol` object list by resource path directly (the grammar half of
 * object_list_load, for probes/fixtures that already know the path; the
 * level wrapper resolves `OListFile` then calls this). */
int object_list_load_file(GameContext* ctx, const char* olist);

/* Attach a model to a slot by resource path (`.ol Model` key). The first
 * load per path goes through `model_open`; later loads reuse a process-
 * lifetime cache (the demo never frees models — documented). Empty path
 * clears. */
void entity_set_model(EntitySlot* e, const char* path);

/* The parsed LWO Model* for a slot (NULL when none/load failed). */
const void* entity_model(const EntitySlot* e);

/* --- entity record accessors (finding 457: the corpus +0x334 record) -----
 * Corpus families FUN_00469cb0 … FUN_0046a310 read fixed fields from the
 * entity's selected stats record (*(entity + 0x334)). The port mirrors that
 * record as EntitySlot.stats_record; NULL resolves to a zero template,
 * matching the shipped static templates (the Stats configuration loader
 * that fills real records is unported — reads
 * therefore default to 0/0.0f, a documented TODO(corpus) deviation). The
 * corpus's float fields return long double (x87 float10); the port hands
 * back the plain float stored in the record. Offsets are record-relative;
 * names are offset-preserving where semantics are unproved. */

#define ENTITY_STATS_RECORD_SIZE 0x150 /* the corpus record stride */

float entity_stats_float_at20(const EntitySlot* e);   /* FUN_00469cb0 */
float entity_stats_float_at24_indexed(const EntitySlot* e, int index);
                                      /* FUN_00469cc0 (terrain-speed array) */
float entity_stats_float_atd4(const EntitySlot* e);   /* FUN_00469ce0 */
float entity_stats_float_at9c(const EntitySlot* e);   /* FUN_00469d00 */
float entity_stats_float_atd8(const EntitySlot* e);   /* FUN_00469d20 */
float entity_stats_float_at6c(const EntitySlot* e);   /* FUN_00469d40 */
float entity_stats_float_at74(const EntitySlot* e);   /* FUN_00469d50 */
float entity_stats_float_at78(const EntitySlot* e);   /* FUN_00469d60 */
float entity_stats_float_ate8(const EntitySlot* e);   /* FUN_00469d70 */
float entity_stats_float_at70(const EntitySlot* e);   /* FUN_00469d90 */
void* entity_stats_field_at_a0(const EntitySlot* e);  /* FUN_00469da0: the
                                         * record+0xa0 address itself */
float entity_stats_float_at7c(const EntitySlot* e);   /* FUN_00469db0 */
float entity_stats_float_at80(const EntitySlot* e);   /* FUN_00469dc0 */
float entity_energy_decay_rate_get(const EntitySlot* e); /* FUN_00469de0 (+84) */
float entity_stats_float_atdc(const EntitySlot* e);   /* FUN_00469e20 */
uint32_t entity_stats_word_atac(const EntitySlot* e); /* FUN_00469e40 */
uint32_t entity_stats_word_get(const EntitySlot* e);  /* FUN_00469e60 (+144) */
uint32_t entity_state_flags_get(const EntitySlot* e); /* FUN_00469e80 (+148) */
uint32_t entity_capability_flags_get(const EntitySlot* e); /* FUN_00469ea0 (+14c) */
float entity_stats_float_at88(const EntitySlot* e);   /* FUN_00469f20
                                         * (the "screen shake" reader) */
uint32_t entity_stats_word_at_c0_or_c4(const EntitySlot* e, int selector);
                                      /* FUN_00469f90: +c4 if selector
                                       * nonzero, else +c0 */
float entity_stats_float_at8c(const EntitySlot* e);   /* FUN_00469fe0 */
float entity_stats_float_at90(const EntitySlot* e);   /* FUN_0046a000 */
float entity_stats_float_at98(const EntitySlot* e);   /* FUN_0046a020 */
float entity_stats_float_at94(const EntitySlot* e);   /* FUN_0046a040 */
float entity_stats_float_atf4(const EntitySlot* e);   /* FUN_0046a080 */
float entity_speed_component_get(const EntitySlot* e); /* FUN_0046a0a0 (+f8) */
float entity_stats_float_at130(const EntitySlot* e);  /* FUN_0046a170 */
float entity_stats_float_at134(const EntitySlot* e);  /* FUN_0046a190 */
float entity_stats_float_at120(const EntitySlot* e);  /* FUN_0046a1b0 */
float entity_stats_float_at128(const EntitySlot* e);  /* FUN_0046a1d0 */
float entity_stats_float_at124(const EntitySlot* e);  /* FUN_0046a1f0 */
uint32_t entity_stats_word_at13c(const EntitySlot* e); /* FUN_0046a210 */
float entity_stats_float_at12c(const EntitySlot* e);  /* FUN_0046a230 */
float entity_stats_float_at138(const EntitySlot* e);  /* FUN_0046a250 */
float entity_collision_radius_get(const EntitySlot* e); /* FUN_0046a270 (+e4) */
float entity_stats_float_at114(const EntitySlot* e);  /* FUN_0046a290 */
float entity_stats_float_at118(const EntitySlot* e);  /* FUN_0046a2b0 */
float entity_stats_float_at11c(const EntitySlot* e);  /* FUN_0046a2d0 */
float entity_stats_float_at110(const EntitySlot* e);  /* FUN_0046a2f0 */
float entity_stats_float_at10c(const EntitySlot* e);  /* FUN_0046a310 */

/* --- entity-state eligibility predicates (findings 318/319/471/472) -------
 * Pure reads over the +0x3e0/+0x3e4/+0x3e8 flag mirrors and the +0x340
 * float — no cell or context state. */

/* FUN_0043baf0 — true when either flag word intersects its literal mask:
 * +0x3e0 & 0x107c33de or +0x3e4 & 0x01c00203. */
bool entity_flags_mask107c33de_or_1c00203_any_test(const EntitySlot* e);

/* FUN_004385b0 — the shared eligibility gate the command and activity
 * layers consult: no +0x3e0 bits 0x700100 or bit 0x20000, no +0x3e4 bit
 * 0x100000, the +0x340 float not negative, and — when the caller passes a
 * zero second argument — +0x3e8 bit 0x20000000 set with bit 0x80000000
 * clear. */
int entity_shared_eligibility_check(const EntitySlot* e, int relaxed);

/* FUN_004436f0 — kind-3 predicate: type == 3 and +0x3e8 bit 0x04000000. */
int entity_type3_flag04000000_test(const EntitySlot* e);

/* FUN_00447730 — routing-progress advance (findings 377/448): add 25 to
 * the +0x344 progress float, clear the +0x2fc link field, clamp to the
 * entity's +0x340 bound, and answer 1 when that bound is reached. */
int entity_progress_advance_by_25(EntitySlot* e);

/* --- object pointer registry (2026-10-24) ---------------------------------
 * ObjectPointerRegistryClearMatch / ObjectPointerRegistryGetSlot
 * (FUN_0042dfc0 / FUN_0042dff0, findings 292/456/247): the corpus's
 * global table of ten entity pointers plus its live count. Its only
 * feeder — the object-list record parse — is modeled without pushing
 * entries, so the count stays zero: clearing answers the no-op, and the
 * slot lookup never fires (documented deviation). The peek is a
 * tests/diagnostics view (the message_active_count precedent). */
void object_pointer_registry_clear_match(int32_t pointer);
int object_pointer_registry_get_slot(uint32_t index, int32_t* out);
uint32_t object_pointer_registry_count_peek(void);

/* --- context-selected globals (2026-10-24) ---------------------------------
 * ContextSelectedGlobalPointerGet / EntityMatchesCurrentSelectionOrOwner
 * (FUN_004295a0 / FUN_004295c0, findings 313/324/478): the corpus's
 * context-selected pointer leaf — the DAT_005560f8 / DAT_00556104 pair
 * chosen by the context-mode global — and the "matches current selection
 * or owner" predicate over the current-selection pointer (DAT_00556160)
 * and the entity's +0x36c owner pointer. The mode and current-selection
 * globals are modeled by the media layer (media_min_distance_mode /
 * _context, fed by the ported SoundMinimumDistanceUpdateMode); only the
 * pointer pair has no ported feeder, so both land zero-initialized
 * (documented deviation). The set/peek functions are the
 * tests/diagnostics view. The +0x36c owner read is pointer-sized on the
 * port (the corpus reads the 32-bit word). */
uint32_t context_selected_global_pointer_get(void);
int entity_matches_current_selection_or_owner(const void* entity);
uint32_t context_selected_pointer_pair_peek(int second);
void context_selected_pointer_pair_set(uint32_t first, uint32_t second);


/* SelectionCandidateEligibilityCheck (FUN_00452870, findings 77/85/468):
 * the selection filter's eligibility predicate — requires the +0x3e8
 * state bit 0x100, the shared-eligibility predicate, no +0x3e4 low-byte
 * bits 1/3 (the corpus's `(byte)word & 10`), no +0x3e0 bits 0x24000000, and
 * — the corpus's documented quirk branch `*param_1 != 1 || (word &
 * 0x40000)`'s right term never fires for non-type-1 entities; type 1
 * rejects on +0x3e0 bit 0x40000 (finding 468). NULL rejects (deviation). */
int selection_candidate_eligibility_check(const EntitySlot* e);

/* EntityFlag40000ThresholdCompare (FUN_00437cc0, findings 362/476): the
 * record-queue validator — the state flags (stats record +0x148) must carry
 * bit 0x40000 and the nested float at +0xe8 must fall below the entity's
 * own +0x340 bound; missing state records read zero, so the test fails. */
int entity_flag40000_threshold_compare(const EntitySlot* e);

#ifdef __cplusplus
}
#endif

/* EntityStatsWordFlag10Test/20Test/40Test (FUN_00439cf0/00439d10/00439d30):
 * the stats-word bit tests — the corpus masks the stats reader's word and
 * hands back the masked result (a bare bit, not a bool). */
uint32_t entity_stats_flag10_test(const EntitySlot* e); /* FUN_00439cf0 */
uint32_t entity_stats_flag20_test(const EntitySlot* e); /* FUN_00439d10 */
uint32_t entity_stats_flag40_test(const EntitySlot* e); /* FUN_00439d30 */

/* GlobalAnimationStateSet (FUN_004383a0): stores the global animation-state
 * counter word (DAT_004eadc4 — the unowned global lands zero-init, finding
 * 464). The corpus's other feeder is the kind-4 dispatch increment, whose
 * dispatch row stays blocked. */
void     entity_global_animation_state_set(uint32_t value); /* FUN_004383a0 */
uint32_t entity_global_animation_state_peek(void);          /* port hook */
/* --- eligibility + admission leaves (2026-10-24; findings 332/460/251/435/471)
 * EntityFlag100AndSharedEligibilityCheck (FUN_00471ce0): with the stats
 * word's bit 0x100 set, consults the shared eligibility gate — the relaxed
 * clause is skipped only for type-4 entities (the corpus's second argument
 * is its own type test). EntityQuadMeshRenderRecordAllowed (FUN_0040aae0):
 * allowed with stats bit 0x08000000 clear and the type neither 8 nor 13.
 * TaskTypeAndConditionCheck (FUN_0043c490): the gate in relaxed mode over
 * the target and the caller's condition; the corpus's first argument is
 * discarded in the body (calling-convention residue, 0x460200 precedent). */
int entity_flag100_and_shared_eligibility_check(const EntitySlot* e);
                                                /* FUN_00471ce0 */
int entity_quad_mesh_render_record_allowed(const EntitySlot* e);
                                                /* FUN_0040aae0 */
int task_type_and_condition_check(const EntitySlot* target, int condition);
                                                /* FUN_0043c490 */

/* EntityTypeAndStateFlagPredicate (FUN_0045e400, findings 324/105): the
 * predicate answers the inverted bit 0x20000000 of the entity's stats state word
 * (the ported +0x148 reader) for entity kinds outside the corpus's excluded
 * set {4, 5, 8, 9, 0xC, 0xD, 0xE, 0x10, 0x11, 0x12, 0x13}; those kinds answer
 * zero whatever the flags say.
 * Deviation: the corpus strides the entity blindly; the port guards it. */
int entity_type_and_state_flag_predicate(const EntitySlot* e); /* FUN_0045e400 */

/* ObjectFilterMatchesActionTarget (FUN_00404f50, findings 318/535): the
 * action-target filter. Type 5 matches unconditionally (subject to the
 * identity pair); types 7/9/0xF additionally require the referenced
 * entity to pass the shared eligibility check when a handle is present;
 * every other type answers 0. When the filter carries a name target
 * (+0x20), the target record's identity words must agree: word 0 equals
 * the name, word 1 equals the filter selector (+0x24) unless that is -1,
 * and word 2 equals the filter's +0x28 companion.
 * Deviations: the corpus strides both records blindly (the port guards);
 * the +0x10 entity handle rides the port's widened-link convention. */
int object_filter_matches_action_target(const void* filter_rec,
                                        const void* target_rec);
                                                /* FUN_00404f50 */

/* EntityDetachCarriedObjectAndResetState (FUN_0043bbc0): the carrier
 * detach. Only type-3 carriers act: a linked type-6 carry loses the pair
 * (link, state word, attach back-pointer, the flag word's 0x400 gate) and
 * both +0x340 floats drop to -1.0, and a linked reference has its dword
 * folded into the level's field 0x9c. The flag word's bit 0x800000 always
 * rides along with the type-3 pass.
 * Deviations: the corpus strides the slot words blindly (the port guards
 * the slot pointer); the field-0x9c fold threads the level block the
 * corpus reads through its global. */
void entity_detach_carried_object_and_reset_state(struct EntitySlot* e,
                                                  struct LevelState* lvl);
                                                /* FUN_0043bbc0 */

/* EntityClearLinkedObjectState (FUN_00447780): the linked-object clear.
 * With the flag word's gate bit 0x400 raised: the carry link and state
 * word drop, the gate drops, the carried slot's flag word bit 0x2000
 * raises, and the entity's kind-4 command-chain records drop through the
 * shared chain drop (the corpus's dispatch extras only mean the carried
 * reference / store flag for kinds 7/0xc/0x17).
 * Deviations: the corpus dereferences a missing carried slot blindly
 * (the port guards); the null slot and gate skip is the faithful shape. */
void entity_clear_linked_object_state(EntitySlot* e);
                                                /* FUN_00447780 */

/* PooledRecordChunkAllocateAndLink (FUN_00438310): grows a power-of-two
 * chunk of entity records onto the pool free list — stride links join the
 * records and the last link joins the previous chunk head, with the chunk
 * table tracking a byte counter. Deviations: widened link slot (stride
 * 0x410 vs the corpus's 0x40c) and a bounded chunk table. */
void pooled_record_chunk_allocate_and_link(void); /* FUN_00438310 */
void* pooled_record_head_peek(void);
uint32_t pooled_record_chunk_count_peek(void);

/* PooledRecordAllocateAndReset (FUN_004382c0): pops the pool's free-list
 * head, zero-clears the record's first 0x40c bytes, self-links the record
 * in the link slot (the corpus's busy mark), and seeds the two float
 * words at +0x2f4 / +0x2f8 with 0xbf800000. When the head runs empty the
 * corpus grows a chunk first; the port answers NULL only if that grow
 * finds the allocator empty (deviation: the corpus strides the word). */
void* pooled_record_allocate_and_reset(void);

/* ResolvedEntityOperationDispatch (FUN_00460910): resolves the name through
 * the corpus resolver's (type, row) pair and feeds the count-difference
 * scan — row to the column slot (unknown names answer 0). Corpus shape:
 * the resolver zeroes the row slot at entry and only table-row matches
 * (and the ore pair) write it, so special names ride the scan with zero. */
int resolved_entity_operation_dispatch(const char* name, int slot,
                                       int subtract);          /* FUN_00460910 */

/* --- DAT_00502bd8 indexed stats-type tables (2026-10-25) -------------------
 * The corpus family FUN_00469b20 … FUN_00469be0 / FUN_00469e00 /
 * FUN_00469ec0 … FUN_00469f00 resolves a template record through
 * DAT_00502bd8's two-level table (`*(*DAT_00502bd8 + type * 4) + row * 4`)
 * and reads fixed fields or stride-0x150 instance blocks from the template.
 * The stats-config loader that fills these tables is unported (the
 * stats-config blocker), so the port models the table as guarded own
 * storage: cells start empty and every read answers 0; install registers
 * a template window and its span in 0x150 strides, and window guards
 * replace the corpus's blind strides (the unowned-global / stride
 * deviations). Offsets stay corpus-relative. */
#define STATS_TYPE_TABLE_TYPES 16 /* window guard: corpus table bounds unknown */
#define STATS_TYPE_TABLE_ROWS 18  /* window guard: corpus row bounds unknown */

void stats_type_table_install(int type, int row, const void* record,
                              int stride_count);
const void* stats_type_table_cell(int type, int row);

uint32_t stats_table_word_at_b8(int type, int row);  /* FUN_00469b20 */
float stats_table_float_at_dc(int type, int row);    /* FUN_00469e00 */
uint32_t stats_table_word_at_144(int type, int row); /* FUN_00469ec0 */
uint32_t stats_table_word_at_148(int type, int row); /* FUN_00469ee0 */
uint32_t stats_table_word_at_14c(int type, int row); /* FUN_00469f00 */

/* Strided triplets: the corpus takes the instance index as its third
 * argument (offset + index * 0x150 into the template). */
uint32_t stats_table_word_at_b4(int type, int row, int stride_index);
                                                   /* FUN_00469af0 */
uint32_t stats_table_word_at_d0(int type, int row, int stride_index);
                                                   /* FUN_00469f60 */
uint32_t stats_table_word_at_100(int type, int row, int stride_index);
                                                   /* FUN_0046a0f0 */

/* Entity-indexed twins: the corpus's parameter is a config-block window
 * holding the type dword at +0x00, the row dword at +0x04, and the
 * instance index dword at +0x330. */
uint32_t stats_instance_word_at_bc(const void* config_block); /* FUN_00469b50 */
uint32_t stats_instance_word_at_e0(const void* config_block); /* FUN_00469b80 */
uint32_t stats_instance_word_at_ec(const void* config_block); /* FUN_00469bb0 */
uint32_t stats_instance_word_at_f0(const void* config_block); /* FUN_00469be0 */

#endif /* LEGORRR_GAME_ENTITY_HPP */

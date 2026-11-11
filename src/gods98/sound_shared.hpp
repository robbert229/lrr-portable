// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/sound.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_SOUND_SHARED_HPP
#define LRR_GODS98_SOUND_SHARED_HPP

/* Corpus chain pool DAT_005023e0 (stride 8: slot word + next word),
 * allocation counter DAT_00502a28. The corpus identifies nodes by their
 * address and treats 0 as null, so the port reserves node id 0 for "no
 * link" and allocates every node exactly once (monotone counter). */
typedef struct VariantNode {
    int slot;   /* bank slot: -1 failed link, stale until first written */
    int next;   /* node id, 0 = end of chain */
} VariantNode;

/* Corpus batch-deferred record: DAT_00502a30 +0 alias, +4 position
 * word, +8/+12/+16 coordinate words, +20 mode word (stride 6 words,
 * count DAT_00502b20). */
typedef struct BatchEntry {
    int      alias;      /* +0  DAT_00502a30 */
    int      position;   /* +4  DAT_00502a34 */
    float    xyz[3];     /* +8  DAT_00502a38/3c/40 */
    uint32_t mode;       /* +20 DAT_00502a44 */
} BatchEntry;

extern const char* const s_alias_builtins[SOUND_ALIAS_BUILTINS];
extern char* s_alias_dynamic[SOUND_ALIAS_MAX - SOUND_ALIAS_BUILTINS];
extern int s_alias_dynamic_count;
extern double s_cooldown_alias;
extern int s_queue_alias[SOUND_REQUEST_QUEUE_CAPACITY];
extern int s_queue_loop[SOUND_REQUEST_QUEUE_CAPACITY];
extern int s_sound_queue_count;
extern float s_cooldown_timer;
extern int s_cooldown_handle;
extern int s_variant_slot[SOUND_ALIAS_MAX];
extern int s_variant_next[SOUND_ALIAS_MAX];
extern VariantNode s_variant_pool[SOUND_VARIANT_POOL_NODES];
extern BatchEntry s_batch_entry[SOUND_BATCH_QUEUE_CAPACITY];
extern int s_batch_count;
extern int s_active_alias[SOUND_REQUEST_QUEUE_CAPACITY];
extern int s_active_loop[SOUND_REQUEST_QUEUE_CAPACITY];
extern int s_active_count;
int s_play_seam(int mode, int position, int slot, int loop, const float* xyz);

#endif /* LRR_GODS98_SOUND_SHARED_HPP */

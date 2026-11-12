// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-021 Lego.c's copies of gods98 Maths.h's inline helpers. The original
 * unit expands them in place, so they have no address of their own. Only
 * lego_unit.c includes this file, just before its first user (finding 556:
 * an include at the top shifts the symbol counter of every function).
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). In a header they add no Reccmp lint
 * debt. */
#ifndef LEGORRR_GAME_LEGO_MATHS_H
#define LEGORRR_GAME_LEGO_MATHS_H

/* ---- group M ---- */



/* ---- group A ---- */



/* ---- group B ---- */



/* ---- group C ---- */

/* Maths_Vector3DSubtract. */
static __inline LegoVector3FC* lego_C_vector3_subtract(LegoVector3FC* r,
                                                      const LegoVector3FC* a,
                                                      const LegoVector3FC* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

/* Lego.h's block macros: the current level's block at (bx, by) and its
 * flag tests. */
static __inline Lego_Block* lego_C_block_at(int bx, int by)
{
    return &LEGO_LEVEL_PTR(Lego_Block, legoGlobs_currLevel->blocks)
        [legoGlobs_currLevel->width * by + bx];
}

static __inline int lego_C_block_is_wall(unsigned int bx, unsigned int by)
{
    return (bx < legoGlobs_currLevel->width - 1 &&
            by < legoGlobs_currLevel->height - 1)
               ? ((lego_C_block_at(bx, by)->flags1 & 0x10) &&
                  !(lego_C_block_at(bx, by)->flags1 & 8))
               : 0;
}

static __inline uint32_t lego_C_block_flag4(unsigned int bx, unsigned int by)
{
    return (bx < legoGlobs_currLevel->width - 1 &&
            by < legoGlobs_currLevel->height - 1)
               ? (lego_C_block_at(bx, by)->flags1 & 4)
               : 0;
}

static __inline int lego_C_block_is_reinforced(unsigned int bx,
                                               unsigned int by)
{
    return (lego_C_block_at(bx, by)->flags1 & 0x400) ||
           (lego_C_block_at(bx, by)->flags2 & 4);
}

static __inline uint32_t lego_C_block_flag20000000(int bx, int by)
{
    return (bx < 0 || by < 0 || bx >= (int)legoGlobs_currLevel->width ||
            by >= (int)legoGlobs_currLevel->height)
               ? 0
               : (lego_C_block_at(bx, by)->flags1 & 0x20000000);
}



/* ---- group D ---- */



/* ---- group E ---- */

/* gods98 Memory.h's Mem_Free: an inline over the CRT's free. Through the
 * inline, every argument of a run of frees loads into eax, as the
 * original's shutdown does; direct free() calls rotate eax/ecx/edx. Needs
 * <stdlib.h>. */
static __inline void lego_e_mem_free(void* addr)
{
    free(addr);
}



/* ---- group F ---- */



/* ---- group G ---- */

typedef struct LegoVector3FG {
    float x;
    float y;
    float z;
} LegoVector3FG;

/* Maths_Vector3DAdd. */
static __inline LegoVector3FG* lego_G_vector3d_add(LegoVector3FG* r,
                                                   const LegoVector3FG* a,
                                                   const LegoVector3FG* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    r->z = a->z + b->z;
    return r;
}

/* Maths_Vector3DSubtract. */
static __inline LegoVector3FG* lego_G_vector3d_subtract(
    LegoVector3FG* r, const LegoVector3FG* a, const LegoVector3FG* b)
{
    r->x = a->x - b->x;
    r->y = a->y - b->y;
    r->z = a->z - b->z;
    return r;
}

/* Maths_Vector3DScale. */
static __inline LegoVector3FG* lego_G_vector3d_scale(LegoVector3FG* r,
                                                     const LegoVector3FG* a,
                                                     float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}



#endif /* LEGORRR_GAME_LEGO_MATHS_H */

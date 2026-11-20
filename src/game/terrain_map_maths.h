// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-026's copy of gods98 Maths.h's inline vector helpers (modulus, add,
 * subtract, scale, normalise, distance, and the 2D modulus, scale,
 * normalise, add and distance), over TerrainMapVector3 and
 * TerrainMapPoint2. The original unit expands them in place, so they have
 * no address of their own. Only terrain_map.c includes this file.
 *
 * They are static __inline functions, not macros: as macros the expansions
 * compile differently (finding 544). The bodies keep Maths.h's forms: the
 * 3D add and subtract parenthesised, the moduli written out as
 * sqrt((x * x) + (y * y) ...), normalise as Scale(r, r, 1 / Modulus(r)) and
 * the 2D distance written out. The 3D distance stays a subtract into a
 * temporary and its modulus: Maths.h's written-out form drops
 * terrain_map_block_ray_intersection (0x4515e0) from 100% to 47.64%. */
#ifndef LEGORRR_GAME_TERRAIN_MAP_MATHS_H
#define LEGORRR_GAME_TERRAIN_MAP_MATHS_H

static __inline float terrain_map_vector3d_modulus(const TerrainMapVector3* v)
{
    return (float)sqrt((v->x * v->x) + (v->y * v->y) + (v->z * v->z));
}

static __inline TerrainMapVector3* terrain_map_vector3d_add(
    TerrainMapVector3* r, const TerrainMapVector3* a,
    const TerrainMapVector3* b)
{
    r->x = (a->x + b->x);
    r->y = (a->y + b->y);
    r->z = (a->z + b->z);
    return r;
}

static __inline TerrainMapVector3* terrain_map_vector3d_subtract(
    TerrainMapVector3* r, const TerrainMapVector3* a,
    const TerrainMapVector3* b)
{
    r->x = (a->x - b->x);
    r->y = (a->y - b->y);
    r->z = (a->z - b->z);
    return r;
}

static __inline TerrainMapVector3* terrain_map_vector3d_scale(
    TerrainMapVector3* r, const TerrainMapVector3* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    r->z = a->z * f;
    return r;
}

static __inline TerrainMapVector3* terrain_map_vector3d_normalise(
    TerrainMapVector3* r)
{
    terrain_map_vector3d_scale(r, r, 1.0f / terrain_map_vector3d_modulus(r));
    return r;
}

static __inline float terrain_map_vector3d_distance(const TerrainMapVector3* a,
                                                    const TerrainMapVector3* b)
{
    TerrainMapVector3 d;

    terrain_map_vector3d_subtract(&d, a, b);
    return terrain_map_vector3d_modulus(&d);
}

static __inline float terrain_map_vector2d_modulus(const TerrainMapPoint2* v)
{
    return (float)sqrt((v->x * v->x) + (v->y * v->y));
}

static __inline TerrainMapPoint2* terrain_map_vector2d_scale(
    TerrainMapPoint2* r, const TerrainMapPoint2* a, float f)
{
    r->x = a->x * f;
    r->y = a->y * f;
    return r;
}

static __inline TerrainMapPoint2* terrain_map_vector2d_normalise(
    TerrainMapPoint2* r)
{
    terrain_map_vector2d_scale(r, r, 1.0f / terrain_map_vector2d_modulus(r));
    return r;
}

static __inline TerrainMapPoint2* terrain_map_vector2d_add(
    TerrainMapPoint2* r, const TerrainMapPoint2* a, const TerrainMapPoint2* b)
{
    r->x = a->x + b->x;
    r->y = a->y + b->y;
    return r;
}

static __inline float terrain_map_vector2d_distance(const TerrainMapPoint2* a,
                                                    const TerrainMapPoint2* b)
{
    return (float)sqrt(((a->x - b->x) * (a->x - b->x)) +
                       ((a->y - b->y) * (a->y - b->y)));
}

#endif /* LEGORRR_GAME_TERRAIN_MAP_MATHS_H */

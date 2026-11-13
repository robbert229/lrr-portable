// SPDX-License-Identifier: LGPL-3.0-or-later
/* lwo.c (original translation unit G98-031): gods98's LightWave object
 * loader. It reads an IFF "FORM"/"LWOB" .lwo file into an APPOBJ (vertices,
 * polygons, surfaces), with an optional ".uv" file kept open beside it.
 *
 * C-compatible header (ADR 0005). lwo.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include
 * this file: every declaration a header adds shifts MSVC's symbol numbering
 * (finding 538). src/game/test_c_boundaries.cpp checks it against the C++
 * declarations of the same functions.
 *
 * The five helpers that port code calls keep the port's names and
 * contracts: string_case_substring_find (string.hpp),
 * owned_resource_tree_release (crt.hpp), lwo_string_duplicate,
 * lwo_texture_projection_mode_parse and lwo_read_big_endian_float
 * (model.hpp). They are declared here with the port's types; lwo.c defines
 * them with the original's, under its own prototypes, and the calls link by
 * name. The other functions take the original's arguments; the native port
 * never calls them (Mesh.c is still the port's, and it reads objects
 * through the port tail's model_open, gods98/lwo_port.cpp). The names are
 * descriptive; the comments give the gods98 names. */
#ifndef LEGORRR_GODS98_LWO_H
#define LEGORRR_GODS98_LWO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct File;

/* The object's counts. */
typedef struct LWSIZE {
    unsigned int lwVertCount;    /* +0x00 */
    unsigned int lwPolyCount;    /* +0x04 */
    unsigned int lwSurfaceCount; /* +0x08 */
} LWSIZE;

/* A polygon: its vertex count and indices, and its surface. */
typedef struct LWPOLY {
    unsigned int plyCount;   /* +0x00 */
    unsigned int srfIndex;   /* +0x04 */
    unsigned short* plyData; /* +0x08 */
} LWPOLY;

/* The SRFS chunk's names. */
typedef struct LWSURFLIST {
    unsigned int srfCount;
    char** srfName;
} LWSURFLIST;

/* A surface (0x54 bytes). A TTEX chunk chains a texture layer, a surface
 * record of its own, on srfNextTex. */
typedef struct LWSURFACE {
    char* srfName;                  /* +0x00 */
    struct LWSURFACE* srfNextTex;   /* +0x04 */
    char* srfPath;                  /* +0x08: TIMG */
    unsigned char srfCol[4];        /* +0x0c: COLR */
    unsigned int srfUnknown10;      /* +0x10: never set */
    unsigned int srfTexFlags;       /* +0x14: TFLG, 0x80 for a sequence */
    unsigned int srfFlags;          /* +0x18: FLAG */
    unsigned int srfTexMode;        /* +0x1c: CTEX/TTEX projection */
    unsigned int srfUnknown20;      /* +0x20: never set */
    float srfTSIZ[3];               /* +0x24 */
    float srfTCTR[3];               /* +0x30 */
    float srfLuminous;              /* +0x3c: LUMI */
    float srfTransparent;           /* +0x40: TRAN */
    float srfDiffuse;               /* +0x44: DIFF */
    float srfReflect;               /* +0x48: REFL */
    float srfSpecular;              /* +0x4c: SPEC */
    float srfGlossy;                /* +0x50: GLOS */
} LWSURFACE;

/* A loaded object (0x20 bytes). */
typedef struct APPOBJ {
    char* aoPath;              /* +0x00: the object's directory */
    LWSIZE aoSize;             /* +0x04 */
    float* aoVerts;            /* +0x10 */
    LWPOLY* aoPoly;            /* +0x14 */
    LWSURFACE* aoSurface;      /* +0x18 */
    struct File* aoFileUV;     /* +0x1c */
} APPOBJ;

/* The five functions with port contracts. */
#if !defined(LEGORRR_LWO_UNIT)

/* stristr (FUN_0048c040). */
char* string_case_substring_find(char* haystack, const char* needle,
                                 uint32_t* out_index, size_t* out_length);

/* FUN_0048c0c0: a copy in a zeroed block of even size. */
char* lwo_string_duplicate(const char* text);

/* FUN_0048c120: 0 planar, 1 cylindrical, 2 spherical. */
int lwo_texture_projection_mode_parse(const char* name);

/* FUN_0048c180: free a surface's strings and texture layers. */
void owned_resource_tree_release(void** node, int own_node);

/* FUN_0048c1d0: read a big-endian float and advance the cursor. */
int lwo_read_big_endian_float(const uint8_t** cursor, uint8_t out[4]);

#endif /* !LEGORRR_LWO_UNIT */

/* LwoPoints (FUN_0048c210). */
int lwo_points_read(struct File* file, LWSIZE* size, int csize,
                    float** verts, int verbose);

/* LwoCurves (FUN_0048c360). */
int lwo_curves_read(struct File* file, int csize, int verbose);

/* LwoPolygons (FUN_0048c3e0). */
int lwo_polygons_read(struct File* file, LWSIZE* size, LWPOLY** polys,
                      int csize, int verbose);

/* LwoSurfaceNames (FUN_0048c690). */
int lwo_surface_names_read(struct File* file, LWSIZE* size,
                           LWSURFLIST** list, int csize,
                           unsigned int* count, int verbose);

/* LwoSurfaceSettings (FUN_0048c820). */
int lwo_surface_settings_read(struct File* file, LWSIZE* size,
                              LWSURFACE** surf, int csize, int verbose);

/* LoadLWO (FUN_0048d2c0). */
int lwo_load(const char* fname, LWSIZE* size, float** verts,
             LWPOLY** polys, LWSURFACE** surf, struct File** uvFile,
             int verbose);

/* LoadAppObj (FUN_0048d7c0). */
int lwo_app_obj_load(const char* fname, APPOBJ** ao, int verbose);

/* FreeLWOB (FUN_0048d870). */
int lwo_app_obj_free(APPOBJ* ao);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_LWO_H */

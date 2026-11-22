// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-031 (text 0048c040-0048d920; .rdata
 * 0049eaf0; literals 004af9f4-004afc98; finding 505): gods98's LightWave
 * object loader (Lwt.c in the gods98 source tree; the binary attests only
 * the chunk names).
 *
 * LoadAppObj reads "<name>.lwo", an IFF "FORM" of type "LWOB", into an
 * APPOBJ: the vertices (PNTS), the polygons (POLS), the surface names (SRFS)
 * and their settings (SURF). CRVS chunks are walked and dropped; other
 * chunks are skipped. When the caller asks for it, "<name>.uv" is opened
 * beside it and kept open in the APPOBJ for Mesh.c. The surfaces are then
 * put in SRFS order; a name with no SURF chunk gets a white "Default"
 * surface. FreeLWOB frees the lot.
 *
 * The native port never calls LoadAppObj or FreeLWOB: Mesh.c is still the
 * port's (gods98/mesh_unit.cpp), and it loads LightWave objects through the
 * port's own reader, model_open and model_parse, which stay unannotated in
 * the port tail, gods98/lwo_port.cpp. The five helpers whose contracts
 * match keep the port's names (lwo.h), so the port's reader and its tests
 * call the original's functions.
 *
 * C-compatible (ADR 0005): C in the matching build, C++ elsewhere. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crt/vc5names.h"
#define LEGORRR_LWO_UNIT 1
#include "lwo.h"
#include "files.h"

/* Big-endian to host. The original spells the 16-bit swap two ways; each
 * use takes the one whose code matches (see finding 601). */
#define LWO_SWAPWORD(x) ((((x) & 0xff) << 8) + ((x) >> 8))
#define LWO_SWAPSHORT(x) ((unsigned short)(((x) << 8) | ((x) >> 8)))
/* Lwt.h's own spellings of the 16-bit swap. */
#define LWSVALUE(n) ((((n) >> 8) & 0xff) + (((n) << 8) & 0xff00))
#define LWSPVALUE(n) ((((*(n)) >> 8) & 0xff) + (((*(n)) << 8) & 0xff00))
#define LWO_SWAPLONG(x) ((((x) & 0xff) << 24) + (((x) & 0xff00) << 8) + \
                         (((x) >> 8) & 0xff00) + (((x) >> 24) & 0xff))

/* The five functions lwo.h declares with the port's types, under the
 * original's. */
#ifdef __cplusplus
extern "C" {
#endif
char* string_case_substring_find(char* str, const char* substr,
                                 unsigned int* index, size_t* length);
char* lwo_string_duplicate(const char* string);
int lwo_texture_projection_mode_parse(const char* name);
void owned_resource_tree_release(LWSURFACE* surf, int freeSurf);
int lwo_read_big_endian_float(unsigned char** ptr, float* f);
#ifdef __cplusplus
}
#endif

/* stristr (FUN_0048c040): the first case-insensitive occurrence of
 * `substr` in `str`, its offset and its length. Lwt.c's private copy of
 * Utils.c's (FUN_00477610). */
// FUNCTION: LEGORR 0x0048c040
char* string_case_substring_find(char* str, const char* substr,
                                 unsigned int* index, size_t* length)
{
    unsigned int len = strlen(str);
    unsigned int sublen = strlen(substr);
    unsigned int i;

    if (len && sublen) {
        for (i = 0; i < len; i++, str++) {
            if (0 == strnicmp(str, substr, sublen)) {
                if (index) *index = i;
                if (length) *length = sublen;
                return str;
            }
        }
    }
    return NULL;
}

/* FUN_0048c0c0: a copy of `string` in a zeroed block rounded up to an
 * even size. */
// FUNCTION: LEGORR 0x0048c0c0
char* lwo_string_duplicate(const char* string)
{
    unsigned int len = strlen(string) + 1;
    char* s;

    len += (len & 1);
    s = (char*)malloc(len);
    memset(s, 0, len);
    strcpy(s, string);
    return s;
}

/* The texture projection named by a CTEX or TTEX chunk: 0 planar,
 * 1 cylindrical, 2 spherical; planar when unknown (FUN_0048c120). */
// FUNCTION: LEGORR 0x0048c120
int lwo_texture_projection_mode_parse(const char* name)
{
    const char* names[3];
    unsigned int i;

    // STRING: LEGORR 0x004afa0c
    names[0] = "PLANAR";
    // STRING: LEGORR 0x004afa00
    names[1] = "CYLINDRICAL";
    // STRING: LEGORR 0x004af9f4
    names[2] = "SPHERICAL";
    for (i = 0; i < 3; i++) {
        if (0 == strnicmp(name, names[i], strlen(names[i]))) break;
    }
    if (i == 3) i = 0;
    return i;
}

/* Free a surface's name, texture path and texture layers, and the surface
 * itself when `freeSurf` is set (FUN_0048c180). */
// FUNCTION: LEGORR 0x0048c180
void owned_resource_tree_release(LWSURFACE* surf, int freeSurf)
{
    if (surf->srfName) free(surf->srfName);
    if (surf->srfPath) free(surf->srfPath);
    if (surf->srfNextTex) owned_resource_tree_release(surf->srfNextTex, 1);
    if (freeSurf) free(surf);
}

/* Read a big-endian float at *ptr and advance it (FUN_0048c1d0). */
// FUNCTION: LEGORR 0x0048c1d0
int lwo_read_big_endian_float(unsigned char** ptr, float* f)
{
    unsigned char* out = (unsigned char*)f;

    out[3] = **ptr;
    out[2] = *(++(*ptr));
    out[1] = *(++(*ptr));
    out[0] = *(++(*ptr));
    (*ptr)++;
    return 1;
}

/* PNTS: the vertex list (FUN_0048c210). */
// FUNCTION: LEGORR 0x0048c210
int lwo_points_read(File* file, LWSIZE* size, int csize, float** verts,
                    int verbose)
{
    /* Unused; the original zeroes it (shared with the chunk readers'
     * locals), so its stores are kept. Lwt.c's PNTSprc shape (OpenLRR):
     * the early return when verts is NULL, and the loop counting csize
     * down (finding 677). */
    char id[10] = {0};
    char str[256];
    float point[3];
    unsigned char* sp;
    unsigned char* cp;
    float* vptr;
    int i = 0;

    sp = (unsigned char*)malloc(csize + 16);
    cp = sp;
    file_read(sp, csize, 1, file);
    size->lwVertCount = csize / 12;
    if (!verts) {
        free(sp);
        return 1;
    }
    *verts = (float*)malloc(csize + 4);
    vptr = *verts;
    while (csize > 0) {
        lwo_read_big_endian_float(&cp, &point[0]);
        lwo_read_big_endian_float(&cp, &point[1]);
        lwo_read_big_endian_float(&cp, &point[2]);
        *vptr++ = point[0];
        *vptr++ = point[1];
        *vptr++ = point[2];
        if (verbose) {
            // STRING: LEGORR 0x004afa14
            snprintf(str, sizeof(str), "Point %d > %f %f %f\n", i, point[0], point[1], point[2]);
        }
        i++;
        csize -= 12;
    }
    free(sp);
    return 1;
}

/* CRVS: walked and dropped (FUN_0048c360). */
// FUNCTION: LEGORR 0x0048c360
int lwo_curves_read(File* file, int csize, int verbose)
{
    unsigned short* buff;
    unsigned short* ptr;
    unsigned short count;
    unsigned short i;

    buff = (unsigned short*)malloc(csize + 16);
    ptr = buff;
    file_read(buff, csize, 1, file);
    while (csize > 0) {
        count = LWO_SWAPWORD(*ptr);
        ptr++;
        csize -= 2;
        for (i = 0; i != count; i++) {
            ptr++;
            csize -= 2;
        }
        csize -= 4;
        ptr += 2;
    }
    free(buff);
    return 1;
}

/* POLS: the polygon list (FUN_0048c3e0), in Lwt.c's POLSprc shape as
 * OpenLRR records it: `ply += pc` into the grown array and a counted-down
 * `while (vcount)` (finding 677). `cp` is unused, as in the original. */
// FUNCTION: LEGORR 0x0048c3e0
int lwo_polygons_read(File* file, LWSIZE* size, LWPOLY** polys, int csize,
                      int verbose)
{
    unsigned short vcount, vcur, vsurf;
    LWPOLY* ply;
    unsigned short* plyd;
    char buff[256], buff2[64];
    unsigned char* sp;
    unsigned char* cp;
    unsigned short* swp;
    unsigned short pc;

    sp = (unsigned char*)malloc(csize + 16);
    cp = sp;
    swp = (unsigned short*)sp;
    file_read(sp, csize, 1, file);
    pc = 0;
    while (csize > 0) {
        vcount = LWO_SWAPWORD(*swp);
        swp++;
        csize -= 2;

        if (polys) {
            if (!pc) {
                ply = (LWPOLY*)malloc(sizeof(LWPOLY));
                *polys = ply;
            } else {
                ply = (LWPOLY*)malloc((pc + 1) * sizeof(LWPOLY));
                memcpy(ply, *polys, (pc * sizeof(LWPOLY)));
                free(*polys);
                *polys = ply;
                ply += pc;
            }
            plyd = (unsigned short*)malloc(vcount * sizeof(unsigned short));
            if (verbose) {
                // STRING: LEGORR 0x004afa40
                snprintf(buff, sizeof(buff), "Poly %d, Vertex Count %d > ", pc, vcount);
            }
            ply->plyCount = vcount;
            ply->plyData = plyd;
            while (vcount) {
                vcur = LWO_SWAPWORD(*swp);
                if (verbose) {
                    // STRING: LEGORR 0x004afa3c
                    snprintf(buff2, sizeof(buff2), "%d ", vcur);
                    strcat(buff, buff2);
                }
                *plyd++ = vcur;
                swp++;
                csize -= 2;
                vcount--;
            }
            vsurf = LWO_SWAPWORD(*swp);
            vsurf--;
            ply->srfIndex = vsurf;
            if (verbose) {
                // STRING: LEGORR 0x004afa2c
                snprintf(buff2, sizeof(buff2), "Surface :- %d\n", vsurf);
                strcat(buff, buff2);
            }
        }
        swp++;
        csize -= 2;
        pc++;
    }
    size->lwPolyCount = pc;
    free(sp);

    return 1;
}

/* SRFS: the surface names (FUN_0048c690). */
// FUNCTION: LEGORR 0x0048c690
int lwo_surface_names_read(File* file, LWSIZE* size, LWSURFLIST** list,
                           int csize, unsigned int* count, int verbose)
{
    /* Unused, as in lwo_points_read. */
    char id[10] = {0};
    char* buff;
    char* ptr;
    unsigned int len;
    unsigned int offset = 0;
    int num = 0;
    char* name;
    char** names;
    LWSURFLIST* surfList;
    char str[512];

    ptr = buff = (char*)malloc(csize + 16);
    file_read(buff, csize, 1, file);
    if (list) {
        surfList = (LWSURFLIST*)malloc(sizeof(LWSURFLIST));
        memset(surfList, 0, sizeof(LWSURFLIST));
    }
    while (csize > 0) {
        if (verbose) {
            // STRING: LEGORR 0x004afa5c
            snprintf(str, sizeof(str), "Surface %d > %s\n", num, ptr);
        }
        len = strlen(ptr) + 1;
        if (len & 1) len++;
        if (list) {
            name = (char*)malloc(len);
            /* The original grows the array by 4 bytes: one Win32 pointer. */
            names = (char**)malloc(offset + sizeof(char*));
            memcpy(name, ptr, len);
            if (offset) {
                memcpy(names, surfList->srfName, offset);
                free(surfList->srfName);
            }
            *(char**)((char*)names + offset) = name;
            surfList->srfName = names;
            *list = surfList;
            surfList->srfCount = num + 1;
        }
        num++;
        offset += sizeof(char*);
        *count = num;
        csize -= len;
        ptr += len;
    }
    free(buff);
    return 1;
}

/* SURF: one surface's settings (FUN_0048c820), in Lwt.c's SURFprc shape
 * as OpenLRR records it: the block-scoped locals of each chunk branch, the
 * flag-name tables as initialised arrays, `setVal` then `surf` tested
 * separately, and Lwt.h's LWSVALUE/LWSPVALUE swaps (finding 677); the
 * sub-chunk length is swapped as an unsigned short sum (finding 682). */
// FUNCTION: LEGORR 0x0048c820
int lwo_surface_settings_read(File* file, LWSIZE* size, LWSURFACE** surf,
                              int csize, int verbose)
{
    char *sp, *cp;
    char buff[512];
    char buff2[512];
    char cbuff[10] = {0};
    unsigned int len;
    LWSURFACE* srfb;
    int sc;
    unsigned int setVal;

    sp = (char*)malloc(csize + 16);
    cp = sp;
    file_read(sp, csize, 1, file);

    sc = size->lwSurfaceCount;

    if (surf) {
        if (!sc) {
            srfb = (LWSURFACE*)malloc(sizeof(LWSURFACE));
            *surf = srfb;
            memset(srfb, 0, sizeof(LWSURFACE));
            srfb->srfName = lwo_string_duplicate(cp);
        } else {
            srfb = (LWSURFACE*)malloc(sizeof(LWSURFACE) * (sc + 1));
            memcpy(srfb, *surf, sc * sizeof(LWSURFACE));
            free(*surf);
            *surf = srfb;
            srfb += sc;
            memset(srfb, 0, sizeof(LWSURFACE));
            srfb->srfName = lwo_string_duplicate(cp);
        }
    }

    if (verbose) {
        // STRING: LEGORR 0x004afc0c
        snprintf(buff, sizeof(buff), "Surface %d :- %s", sc, cp);
    }
    len = strlen(cp) + 1;
    if (len & 1)
        len++;
    cp += len;
    csize -= len;
    while (csize > 0) {
        strncpy(cbuff, cp, 4);
        cp += 4;
        len = (unsigned short)((*(unsigned short*)cp << 8) + (*(unsigned short*)cp >> 8));
        // STRING: LEGORR 0x004afc00
        snprintf(buff, sizeof(buff), " %s (%d)", cbuff, len);
        cp += 2;
        // STRING: LEGORR 0x004afbf8
        if (!strnicmp(cbuff, "TIMG", 4)) {
            strcat(buff, cp);
            if (surf) {
                unsigned int pos;
                size_t len;
                char cps[128];

                strcpy(cps, cp);
                // STRING: LEGORR 0x004afbec
                if (string_case_substring_find(cps, "(sequence)", &pos, &len)) {
                    *(cps + pos - 1) = 0;
                    srfb->srfTexFlags |= 0x80;
                }
                srfb->srfPath = lwo_string_duplicate(cps);
            }
        }
        // STRING: LEGORR 0x004afbe4
        else if (!strnicmp(cbuff, "COLR", 4)) {
            // STRING: LEGORR 0x004afbd0
            snprintf(buff2, sizeof(buff2), " RGBA %d,%d,%d,%d", (unsigned char)cp[0], (unsigned char)cp[1], (unsigned char)cp[2], (unsigned char)cp[3]);
            strcat(buff, buff2);
            if (surf) {
                srfb->srfCol[0] = cp[0];
                srfb->srfCol[1] = cp[1];
                srfb->srfCol[2] = cp[2];
                srfb->srfCol[3] = cp[3];
            }
        }
        // STRING: LEGORR 0x004afbc8
        else if (!strnicmp(cbuff, "CTEX", 4)) {
            strcat(buff, cp);
            if (surf) {
                srfb->srfTexMode = lwo_texture_projection_mode_parse(cp);
            }
        }
        // STRING: LEGORR 0x004afbc0
        else if (!strnicmp(cbuff, "TTEX", 4)) {
            strcat(buff, cp);
            if (surf) {
                LWSURFACE* srfbN;

                srfbN = (LWSURFACE*)malloc(sizeof(LWSURFACE));
                srfb->srfNextTex = srfbN;
                srfb = srfbN;
                memset(srfb, 0, sizeof(LWSURFACE));

                srfb->srfTexMode = lwo_texture_projection_mode_parse(cp);
            }
        }
        // STRING: LEGORR 0x004afbb8
        else if (!strnicmp(cbuff, "TFLG", 4)) {
            unsigned short sval, i, f = 0;
            if (surf) {
                const char* ttexName[] = {
                    // STRING: LEGORR 0x004afbb0
                    "Axis_x",
                    // STRING: LEGORR 0x004afba0
                    "Axis_y",
                    // STRING: LEGORR 0x004afb98
                    "Axis_z",
                    // STRING: LEGORR 0x004afb8c
                    "World Coord",
                    // STRING: LEGORR 0x004afb7c
                    "Negative Image",
                    // STRING: LEGORR 0x004afb6c
                    "Pixel Blending",
                    // STRING: LEGORR 0x004afb5c
                    "Anti Aliasing",
                };

                sval = LWSPVALUE((unsigned short*)cp);
                srfb->srfTexFlags |= sval;
                // STRING: LEGORR 0x004afba8
                snprintf(buff2, sizeof(buff2), "%xH - ", sval);
                for (i = 0; i < 8; i++) {
                    if (sval & (1 << i)) {
                        if (f)
                            strcat(buff2, ",");
                        f++;
                        strcat(buff2, ttexName[i]);
                    }
                }

                strcat(buff, buff2);
            }
        }
        // STRING: LEGORR 0x004afb54
        else if (!strnicmp(cbuff, "FLAG", 4)) {
            unsigned short sval, i, f = 0;
            if (surf) {
                const char* tName[] = {
                    // STRING: LEGORR 0x004afb48
                    "Luminous",
                    // STRING: LEGORR 0x004afb40
                    "Outline",
                    // STRING: LEGORR 0x004afb34
                    "Smoothing",
                    // STRING: LEGORR 0x004afb20
                    "Color Highlights",
                    // STRING: LEGORR 0x004afb10
                    "Color Filter",
                    // STRING: LEGORR 0x004afb04
                    "Opaque Edge",
                    // STRING: LEGORR 0x004afaf0
                    "Transparent Edge",
                    // STRING: LEGORR 0x004afadc
                    "Sharp Terminator",
                    // STRING: LEGORR 0x004afacc
                    "Double Sided",
                    // STRING: LEGORR 0x004afac0
                    "Additive"
                };

                sval = LWSPVALUE((unsigned short*)cp);
                srfb->srfFlags = sval;
                snprintf(buff2, sizeof(buff2), "%xH - ", sval);
                for (i = 0; i < 10; i++) {
                    if (sval & (1 << i)) {
                        if (f)
                            strcat(buff2, ",");
                        f++;
                        strcat(buff2, tName[i]);
                    }
                }

                strcat(buff, buff2);
            }
        }
        // STRING: LEGORR 0x004afab8
        else if (!strnicmp(cbuff, "TSIZ", 4)) {
            float td[3];
            if (surf) {
                unsigned char* ccp = (unsigned char*)cp;
                lwo_read_big_endian_float(&ccp, &td[0]);
                lwo_read_big_endian_float(&ccp, &td[1]);
                lwo_read_big_endian_float(&ccp, &td[2]);
                // STRING: LEGORR 0x004afaac
                snprintf(buff2, sizeof(buff2), " (%f,%f,%f)", td[0], td[1], td[2]);
                srfb->srfTSIZ[0] = td[0];
                srfb->srfTSIZ[1] = td[1];
                srfb->srfTSIZ[2] = td[2];
                strcat(buff, buff2);
            }
        }
        // STRING: LEGORR 0x004afaa4
        else if (!strnicmp(cbuff, "TCTR", 4)) {
            float td[3];
            if (surf) {
                unsigned char* ccp = (unsigned char*)cp;
                lwo_read_big_endian_float(&ccp, &td[0]);
                lwo_read_big_endian_float(&ccp, &td[1]);
                lwo_read_big_endian_float(&ccp, &td[2]);
                snprintf(buff2, sizeof(buff2), " (%f,%f,%f)", td[0], td[1], td[2]);
                srfb->srfTCTR[0] = td[0];
                srfb->srfTCTR[1] = td[1];
                srfb->srfTCTR[2] = td[2];
                strcat(buff, buff2);
            }
        }
        setVal = 0;
        // STRING: LEGORR 0x004afa9c
        if (!strnicmp(cbuff, "TRAN", 4))
            setVal = 1;
        // STRING: LEGORR 0x004afa94
        else if (!strnicmp(cbuff, "DIFF", 4))
            setVal = 2;
        // STRING: LEGORR 0x004afa8c
        else if (!strnicmp(cbuff, "LUMI", 4))
            setVal = 3;
        // STRING: LEGORR 0x004afa84
        else if (!strnicmp(cbuff, "REFL", 4))
            setVal = 4;
        // STRING: LEGORR 0x004afa7c
        else if (!strnicmp(cbuff, "SPEC", 4))
            setVal = 5;
        // STRING: LEGORR 0x004afa74
        else if (!strnicmp(cbuff, "GLOS", 4))
            setVal = 6;

        if (setVal) {
            unsigned short sval;
            float fval;
            if (surf) {
                sval = LWSPVALUE((unsigned short*)cp);
                fval = (float)sval / 256;
                switch (setVal) {
                case 1:
                    srfb->srfTransparent = fval;
                    break;
                case 2:
                    srfb->srfDiffuse = fval;
                    break;
                case 3:
                    srfb->srfLuminous = fval;
                    break;
                case 4:
                    srfb->srfReflect = fval;
                    break;
                case 5:
                    srfb->srfSpecular = fval;
                    break;
                case 6:
                    srfb->srfGlossy = fval;
                    break;
                }
                // STRING: LEGORR 0x004afa70
                snprintf(buff2, sizeof(buff2), "%f", fval);
                strcat(buff, buff2);
            }
        }
        cp += len;

        csize -= (len + 6);
    }
    size->lwSurfaceCount++;
    free(sp);
    return 1;
}

/* LoadLWO (FUN_0048d2c0): read "<fname>.lwo"; 0 when it is missing or not
 * an LWOB. */
// FUNCTION: LEGORR 0x0048d2c0
int lwo_load(const char* fname, LWSIZE* size, float** verts, LWPOLY** polys,
             LWSURFACE** surf, File** uvFile, int verbose)
{
    char id[10] = {0};
    unsigned int formSize;
    unsigned int chunkSize;
    unsigned int surfNameCount = 0;
    LWSURFLIST* surfList;
    unsigned int surfCount;
    unsigned int i;
    unsigned int j;
    int remaining;
    File* file;
    LWSIZE* sizeData;
    LWSURFACE* newSurf;
    char* name;
    char path[256];
    char str[1024];

    // STRING: LEGORR 0x004afc98
    snprintf(path, sizeof(path), "%s.lwo", fname);
    if ((file = file_open(path, "rb")) == NULL) return 0;

    sizeData = (LWSIZE*)malloc(sizeof(LWSIZE));
    sizeData->lwSurfaceCount = 0;
    sizeData->lwPolyCount = 0;
    sizeData->lwVertCount = 0;

    if (uvFile) {
        // STRING: LEGORR 0x004afc90
        snprintf(path, sizeof(path), "%s.uv", fname);
        if (file_exists(path)) *uvFile = file_open(path, "rb");
        else *uvFile = NULL;
    }

    file_read(id, 4, 1, file);
    // STRING: LEGORR 0x004afc88
    if (0 != _strcmpi(id, "FORM")) {
        free(sizeData);
        file_close(file);
        // STRING: LEGORR 0x004afc64
        snprintf(str, sizeof(str), "Error in Lightwave file %s", fname);
        return 0;
    }
    file_read(&formSize, 4, 1, file);
    formSize = LWO_SWAPLONG(formSize);
    file_read(id, 4, 1, file);
    // STRING: LEGORR 0x004afc80
    if (0 != _strcmpi(id, "LWOB")) {
        free(sizeData);
        file_close(file);
        snprintf(str, sizeof(str), "Error in Lightwave file %s", fname);
        return 0;
    }

    for (remaining = formSize - 4; remaining > 0; remaining -= 8 + chunkSize) {
        file_read(id, 4, 1, file);
        file_read(&chunkSize, 4, 1, file);
        chunkSize = LWO_SWAPLONG(chunkSize);
        if (verbose) {
            // STRING: LEGORR 0x004afc50
            snprintf(str, sizeof(str), "LW OBJECT %s %d\n", id, chunkSize);
        }
        // STRING: LEGORR 0x004afc48
        if (0 == _strcmpi(id, "PNTS")) lwo_points_read(file, sizeData, chunkSize, verts, verbose);
        // STRING: LEGORR 0x004afc40
        else if (0 == _strcmpi(id, "SRFS")) lwo_surface_names_read(file, sizeData, &surfList, chunkSize, &surfNameCount, verbose);
        // STRING: LEGORR 0x004afc38
        else if (0 == _strcmpi(id, "SURF")) lwo_surface_settings_read(file, sizeData, surf, chunkSize, verbose);
        // STRING: LEGORR 0x004afc30
        else if (0 == _strcmpi(id, "CRVS")) lwo_curves_read(file, chunkSize, verbose);
        // STRING: LEGORR 0x004afc28
        else if (0 == _strcmpi(id, "POLS")) lwo_polygons_read(file, sizeData, polys, chunkSize, verbose);
        else file_seek(file, chunkSize, SEEK_CUR);
    }

    surfCount = sizeData->lwSurfaceCount;
    if (surf && surfList && sizeData) {
        newSurf = (LWSURFACE*)malloc(sizeof(LWSURFACE) * surfNameCount);
        memset(newSurf, 0, sizeof(LWSURFACE) * surfNameCount);
        for (i = 0; i < surfNameCount; i++) {
            name = surfList->srfName[i];
            for (j = 0; j < surfCount; j++) {
                if (0 == _strcmpi((*surf)[j].srfName, name)) {
                    newSurf[i] = (*surf)[j];
                    break;
                }
            }
            if (j == surfCount) {
                // STRING: LEGORR 0x004afc20
                newSurf[i].srfName = lwo_string_duplicate("Default");
                newSurf[i].srfCol[0] = 0xff;
                newSurf[i].srfCol[1] = 0xff;
                newSurf[i].srfCol[2] = 0xff;
                surfCount = surfNameCount;
            }
        }
        free(*surf);
        *surf = newSurf;
    }
    sizeData->lwSurfaceCount = surfCount;
    if (size) *size = *sizeData;
    free(sizeData);
    if (surfNameCount) {
        for (i = 0; i < surfNameCount; i++) free(surfList->srfName[i]);
        free(surfList->srfName);
        free(surfList);
    }
    file_close(file);
    return 1;
}

/* LoadAppObj (FUN_0048d7c0): load "<fname>.lwo" into a new APPOBJ. */
// FUNCTION: LEGORR 0x0048d7c0
int lwo_app_obj_load(const char* fname, APPOBJ** ao, int verbose)
{
    char path[128];
    APPOBJ* obj;

    *ao = (APPOBJ*)malloc(sizeof(APPOBJ));
    memset(*ao, 0, sizeof(APPOBJ));
    _splitpath(fname, NULL, path, NULL, NULL);
    (*ao)->aoPath = lwo_string_duplicate(path);
    if (lwo_load(fname, &(*ao)->aoSize, &(*ao)->aoVerts, &(*ao)->aoPoly,
                 &(*ao)->aoSurface, &(*ao)->aoFileUV, verbose)) {
        return 1;
    }
    free((*ao)->aoPath);
    obj = *ao;
    free(obj);
    return 0;
}

/* FreeLWOB (FUN_0048d870). */
// FUNCTION: LEGORR 0x0048d870
int lwo_app_obj_free(APPOBJ* ao)
{
    unsigned int i;
    unsigned int count;
    LWSURFACE* surf;

    if (ao == NULL) return 0;
    if (ao->aoFileUV) file_close(ao->aoFileUV);
    if ((surf = ao->aoSurface) != NULL) {
        count = ao->aoSize.lwSurfaceCount;
        for (i = 0; i < count; i++) {
            owned_resource_tree_release(surf, 0);
            surf++;
        }
        free(ao->aoSurface);
    }
    if (ao->aoPoly) {
        count = ao->aoSize.lwPolyCount;
        for (i = 0; i < count; i++) {
            free(ao->aoPoly[i].plyData);
        }
        free(ao->aoPoly);
    }
    free(ao->aoVerts);
    free(ao->aoPath);
    free(ao);
    return 1;
}

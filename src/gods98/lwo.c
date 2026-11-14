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
     * locals), so its stores are kept. */
    char id[10] = {0};
    float point[3];
    unsigned char* ptr;
    unsigned char* buff;
    int i;
    int count = 0;
    float* v;
    char str[256];

    buff = ptr = (unsigned char*)malloc(csize + 16);
    file_read(buff, csize, 1, file);
    size->lwVertCount = csize / 12;
    if (verts) {
        v = *verts = (float*)malloc(csize + 4);
        for (i = 0; i < csize; i += 12) {
            lwo_read_big_endian_float(&ptr, &point[0]);
            lwo_read_big_endian_float(&ptr, &point[1]);
            lwo_read_big_endian_float(&ptr, &point[2]);
            *v++ = point[0];
            *v++ = point[1];
            *v++ = point[2];
            if (verbose) {
                // STRING: LEGORR 0x004afa14
                sprintf(str, "Point %d > %f %f %f\n", count, point[0], point[1], point[2]);
            }
            count++;
        }
    }
    free(buff);
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

/* POLS: the polygon list (FUN_0048c3e0). */
// FUNCTION: LEGORR 0x0048c3e0
int lwo_polygons_read(File* file, LWSIZE* size, LWPOLY** polys, int csize,
                      int verbose)
{
    unsigned short* buff;
    unsigned short* ptr;
    unsigned short count;
    unsigned short vertCount;
    unsigned short i;
    unsigned short vert;
    unsigned short* data;
    LWPOLY* poly;
    LWPOLY* newPolys;
    char tmp[64];
    char str[256];

    buff = (unsigned short*)malloc(csize + 16);
    file_read(buff, csize, 1, file);
    ptr = buff;
    count = 0;
    while (csize > 0) {
        vertCount = LWO_SWAPSHORT(*ptr);
        ptr++;
        csize -= 2;
        if (polys) {
            if (count == 0) {
                poly = *polys = (LWPOLY*)malloc(sizeof(LWPOLY));
            } else {
                newPolys = (LWPOLY*)malloc(sizeof(LWPOLY) * (count + 1));
                memcpy(newPolys, *polys, sizeof(LWPOLY) * count);
                free(*polys);
                *polys = newPolys;
                poly = &newPolys[count];
            }
            data = (unsigned short*)malloc(vertCount * 2);
            if (verbose) {
                // STRING: LEGORR 0x004afa40
                sprintf(str, "Poly %d, Vertex Count %d > ", count, vertCount);
            }
            poly->plyCount = vertCount;
            poly->plyData = data;
            for (i = 0; i != vertCount; i++) {
                vert = LWO_SWAPSHORT(*ptr);
                if (verbose) {
                    // STRING: LEGORR 0x004afa3c
                    sprintf(tmp, "%d ", vert);
                    strcat(str, tmp);
                }
                *data++ = vert;
                ptr++;
                csize -= 2;
            }
            poly->srfIndex = (unsigned short)(LWO_SWAPSHORT(*ptr) - 1);
            if (verbose) {
                // STRING: LEGORR 0x004afa2c
                sprintf(tmp, "Surface :- %d\n", poly->srfIndex);
                strcat(str, tmp);
            }
        }
        ptr++;
        csize -= 2;
        count++;
    }
    size->lwPolyCount = count;
    free(buff);
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
            sprintf(str, "Surface %d > %s\n", num, ptr);
        }
        len = strlen(ptr) + 1;
        if (len & 1) len++;
        if (list) {
            name = (char*)malloc(len);
            names = (char**)malloc(offset + 4);
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
        offset += 4;
        *count = num;
        csize -= len;
        ptr += len;
    }
    free(buff);
    return 1;
}

/* SURF: one surface's settings (FUN_0048c820). */
// FUNCTION: LEGORR 0x0048c820
int lwo_surface_settings_read(File* file, LWSIZE* size, LWSURFACE** surf,
                              int csize, int verbose)
{
    char id[10] = {0};
    char* buff;
    char* ptr;
    unsigned int len;
    unsigned int surfCount;
    LWSURFACE* cur;
    LWSURFACE* newSurf;
    unsigned int subSize;
    unsigned short flags;
    unsigned int i;
    unsigned short printed;
    int type;
    float value;
    unsigned char* fptr;
    unsigned int seqIndex;
    size_t seqLength;
    float tsiz[3];
    float tctr[3];
    const char* texFlagNames[7];
    const char* surfFlagNames[10];
    char tmp[512];
    char texName[128];
    char str[512];

    buff = (char*)malloc(csize + 16);
    file_read(buff, csize, 1, file);
    surfCount = size->lwSurfaceCount;
    if (surf) {
        if (surfCount == 0) {
            cur = *surf = (LWSURFACE*)malloc(sizeof(LWSURFACE));
            memset(cur, 0, sizeof(LWSURFACE));
            cur->srfName = lwo_string_duplicate(buff);
        } else {
            newSurf = (LWSURFACE*)malloc(sizeof(LWSURFACE) * (surfCount + 1));
            memcpy(newSurf, *surf, sizeof(LWSURFACE) * surfCount);
            free(*surf);
            *surf = newSurf;
            cur = &newSurf[surfCount];
            memset(cur, 0, sizeof(LWSURFACE));
            cur->srfName = lwo_string_duplicate(buff);
        }
    }
    if (verbose) {
        // STRING: LEGORR 0x004afc0c
        sprintf(str, "Surface %d :- %s", surfCount, buff);
    }
    ptr = buff;
    len = strlen(ptr) + 1;
    if (len & 1) len++;
    csize -= len;
    ptr += len;
    while (csize > 0) {
        strncpy(id, ptr, 4);
        ptr += 4;
        subSize = LWO_SWAPWORD(*(unsigned short*)ptr);
        // STRING: LEGORR 0x004afc00
        sprintf(str, " %s (%d)", id, subSize);
        ptr += 2;
        // STRING: LEGORR 0x004afbf8
        if (0 == strnicmp(id, "TIMG", 4)) {
            strcat(str, ptr);
            if (surf) {
                strcpy(texName, ptr);
                // STRING: LEGORR 0x004afbec
                if (string_case_substring_find(texName, "(sequence)", &seqIndex, &seqLength)) {
                    cur->srfTexFlags |= 0x80;
                    texName[seqIndex - 1] = '\0';
                }
                cur->srfPath = lwo_string_duplicate(texName);
            }
        // STRING: LEGORR 0x004afbe4
        } else if (0 == strnicmp(id, "COLR", 4)) {
            // STRING: LEGORR 0x004afbd0
            sprintf(tmp, " RGBA %d,%d,%d,%d", (unsigned char)ptr[0], (unsigned char)ptr[1], (unsigned char)ptr[2], (unsigned char)ptr[3]);
            strcat(str, tmp);
            if (surf) {
                cur->srfCol[0] = ptr[0];
                cur->srfCol[1] = ptr[1];
                cur->srfCol[2] = ptr[2];
                cur->srfCol[3] = ptr[3];
            }
        // STRING: LEGORR 0x004afbc8
        } else if (0 == strnicmp(id, "CTEX", 4)) {
            strcat(str, ptr);
            if (surf) cur->srfTexMode = lwo_texture_projection_mode_parse(ptr);
        // STRING: LEGORR 0x004afbc0
        } else if (0 == strnicmp(id, "TTEX", 4)) {
            strcat(str, ptr);
            if (surf) {
                cur = cur->srfNextTex = (LWSURFACE*)malloc(sizeof(LWSURFACE));
                memset(cur, 0, sizeof(LWSURFACE));
                cur->srfTexMode = lwo_texture_projection_mode_parse(ptr);
            }
        // STRING: LEGORR 0x004afbb8
        } else if (0 == strnicmp(id, "TFLG", 4)) {
            printed = 0;
            if (surf) {
                flags = LWO_SWAPWORD(*(unsigned short*)ptr);
                // STRING: LEGORR 0x004afbb0
                texFlagNames[0] = "Axis_x";
                cur->srfTexFlags |= flags;
                // STRING: LEGORR 0x004afba0
                texFlagNames[1] = "Axis_y";
                // STRING: LEGORR 0x004afb98
                texFlagNames[2] = "Axis_z";
                // STRING: LEGORR 0x004afb8c
                texFlagNames[3] = "World Coord";
                // STRING: LEGORR 0x004afb7c
                texFlagNames[4] = "Negative Image";
                // STRING: LEGORR 0x004afb6c
                texFlagNames[5] = "Pixel Blending";
                // STRING: LEGORR 0x004afb5c
                texFlagNames[6] = "Anti Aliasing";
                // STRING: LEGORR 0x004afba8
                sprintf(tmp, "%xH - ", flags);
                for (i = 0; i < 8; i++) {
                    if (flags & (1 << i)) {
                        if ((short)printed) strcat(tmp, ",");
                        printed++;
                        strcat(tmp, texFlagNames[i]);
                    }
                }
                strcat(str, tmp);
            }
        // STRING: LEGORR 0x004afb54
        } else if (0 == strnicmp(id, "FLAG", 4)) {
            printed = 0;
            if (surf) {
                flags = LWO_SWAPWORD(*(unsigned short*)ptr);
                // STRING: LEGORR 0x004afb48
                surfFlagNames[0] = "Luminous";
                // STRING: LEGORR 0x004afb40
                surfFlagNames[1] = "Outline";
                // STRING: LEGORR 0x004afb34
                surfFlagNames[2] = "Smoothing";
                // STRING: LEGORR 0x004afb20
                surfFlagNames[3] = "Color Highlights";
                // STRING: LEGORR 0x004afb10
                surfFlagNames[4] = "Color Filter";
                // STRING: LEGORR 0x004afb04
                surfFlagNames[5] = "Opaque Edge";
                // STRING: LEGORR 0x004afaf0
                surfFlagNames[6] = "Transparent Edge";
                // STRING: LEGORR 0x004afadc
                surfFlagNames[7] = "Sharp Terminator";
                // STRING: LEGORR 0x004afacc
                surfFlagNames[8] = "Double Sided";
                // STRING: LEGORR 0x004afac0
                surfFlagNames[9] = "Additive";
                cur->srfFlags = flags;
                sprintf(tmp, "%xH - ", flags);
                for (i = 0; i < 10; i++) {
                    if (flags & (1 << i)) {
                        if ((short)printed) strcat(tmp, ",");
                        printed++;
                        strcat(tmp, surfFlagNames[i]);
                    }
                }
                strcat(str, tmp);
            }
        // STRING: LEGORR 0x004afab8
        } else if (0 == strnicmp(id, "TSIZ", 4)) {
            if (surf) {
                fptr = (unsigned char*)ptr;
                lwo_read_big_endian_float(&fptr, &tsiz[0]);
                lwo_read_big_endian_float(&fptr, &tsiz[1]);
                lwo_read_big_endian_float(&fptr, &tsiz[2]);
                // STRING: LEGORR 0x004afaac
                sprintf(tmp, " (%f,%f,%f)", tsiz[0], tsiz[1], tsiz[2]);
                cur->srfTSIZ[0] = tsiz[0];
                cur->srfTSIZ[1] = tsiz[1];
                cur->srfTSIZ[2] = tsiz[2];
                strcat(str, tmp);
            }
        // STRING: LEGORR 0x004afaa4
        } else if (0 == strnicmp(id, "TCTR", 4)) {
            if (surf) {
                fptr = (unsigned char*)ptr;
                lwo_read_big_endian_float(&fptr, &tctr[0]);
                lwo_read_big_endian_float(&fptr, &tctr[1]);
                lwo_read_big_endian_float(&fptr, &tctr[2]);
                sprintf(tmp, " (%f,%f,%f)", tctr[0], tctr[1], tctr[2]);
                cur->srfTCTR[0] = tctr[0];
                cur->srfTCTR[1] = tctr[1];
                cur->srfTCTR[2] = tctr[2];
                strcat(str, tmp);
            }
        }

        type = 0;
        // STRING: LEGORR 0x004afa9c
        if (0 == strnicmp(id, "TRAN", 4)) type = 1;
        // STRING: LEGORR 0x004afa94
        else if (0 == strnicmp(id, "DIFF", 4)) type = 2;
        // STRING: LEGORR 0x004afa8c
        else if (0 == strnicmp(id, "LUMI", 4)) type = 3;
        // STRING: LEGORR 0x004afa84
        else if (0 == strnicmp(id, "REFL", 4)) type = 4;
        // STRING: LEGORR 0x004afa7c
        else if (0 == strnicmp(id, "SPEC", 4)) type = 5;
        // STRING: LEGORR 0x004afa74
        else if (0 == strnicmp(id, "GLOS", 4)) type = 6;
        if (type && surf) {
            value = (float)(int)LWO_SWAPSHORT(*(unsigned short*)ptr) / 256.0f;
            switch (type) {
            case 1: cur->srfTransparent = value; break;
            case 2: cur->srfDiffuse = value; break;
            case 3: cur->srfLuminous = value; break;
            case 4: cur->srfReflect = value; break;
            case 5: cur->srfSpecular = value; break;
            case 6: cur->srfGlossy = value; break;
            }
            // STRING: LEGORR 0x004afa70
            sprintf(tmp, "%f", value);
            strcat(str, tmp);
        }
        ptr += subSize;
        csize -= 6 + subSize;
    }
    size->lwSurfaceCount++;
    free(buff);
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
    sprintf(path, "%s.lwo", fname);
    if ((file = file_open(path, "rb")) == NULL) return 0;

    sizeData = (LWSIZE*)malloc(sizeof(LWSIZE));
    sizeData->lwSurfaceCount = 0;
    sizeData->lwPolyCount = 0;
    sizeData->lwVertCount = 0;

    if (uvFile) {
        // STRING: LEGORR 0x004afc90
        sprintf(path, "%s.uv", fname);
        if (file_exists(path)) *uvFile = file_open(path, "rb");
        else *uvFile = NULL;
    }

    file_read(id, 4, 1, file);
    // STRING: LEGORR 0x004afc88
    if (0 != _strcmpi(id, "FORM")) {
        free(sizeData);
        file_close(file);
        // STRING: LEGORR 0x004afc64
        sprintf(str, "Error in Lightwave file %s", fname);
        return 0;
    }
    file_read(&formSize, 4, 1, file);
    formSize = LWO_SWAPLONG(formSize);
    file_read(id, 4, 1, file);
    // STRING: LEGORR 0x004afc80
    if (0 != _strcmpi(id, "LWOB")) {
        free(sizeData);
        file_close(file);
        sprintf(str, "Error in Lightwave file %s", fname);
        return 0;
    }

    for (remaining = formSize - 4; remaining > 0; remaining -= 8 + chunkSize) {
        file_read(id, 4, 1, file);
        file_read(&chunkSize, 4, 1, file);
        chunkSize = LWO_SWAPLONG(chunkSize);
        if (verbose) {
            // STRING: LEGORR 0x004afc50
            sprintf(str, "LW OBJECT %s %d\n", id, chunkSize);
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

    *ao = (APPOBJ*)malloc(sizeof(APPOBJ));
    memset(*ao, 0, sizeof(APPOBJ));
    _splitpath(fname, NULL, path, NULL, NULL);
    (*ao)->aoPath = lwo_string_duplicate(path);
    if (lwo_load(fname, &(*ao)->aoSize, &(*ao)->aoVerts, &(*ao)->aoPoly,
                 &(*ao)->aoSurface, &(*ao)->aoFileUV, verbose)) {
        return 1;
    }
    free((*ao)->aoPath);
    free(*ao);
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

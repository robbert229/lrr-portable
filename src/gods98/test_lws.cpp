// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_lws — lws.c (G98-020) at a scene's last key (issue #943).
 *
 * Lws_Parse allocates one key more than the file lists. When the last key
 * is before the scene's last frame it fills that slot with a copy of the
 * last key and counts it; when the last key is on the last frame it leaves
 * the slot unwritten. Lws_FindPrevKey and Lws_Interpolate blend the last
 * key with the slot after it. In a Tutorial01 run that slot held all-ones
 * bytes (frame 65535, NaN floats), so a raider's nodes went NaN at the end
 * of an animation, and a power crystal carried on its carry null took a
 * NaN position.
 *
 * The test loads a synthetic scene through the real parser, with the units'
 * file reads served from memory and their allocations filled with all-ones
 * bytes and padded past the end. It sets each scene to its last frame and
 * checks that the null frame holds its last key. GNU ld's --wrap routes the
 * units' calls to the __wrap_ functions below. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <miniwin/d3drm.h>

#include "gods98/files_open.h"
#include "gods98/lws.h"
#include "gods98/main_globs.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* ---- Allocations: all-ones bytes, padded past the end ---- */

enum { kPad = 256 };

extern "C" void* __real_malloc(size_t size);

extern "C" void* __wrap_malloc(size_t size)
{
    unsigned char* p = (unsigned char*)__real_malloc(size + kPad);
    if (p) memset(p, 0xff, size + kPad);
    return p;
}

/* ---- The units' file reads, from memory ---- */

static const char* s_text = NULL;
static const char* s_cursor = NULL;
static int s_handle;

extern "C" struct GameFileHandle* __wrap_game_file_open_dispatch(const char* path,
                                                                 const char* mode)
{
    (void)path;
    (void)mode;
    if (!s_text) return NULL;
    s_cursor = s_text;
    return (struct GameFileHandle*)&s_handle;
}

extern "C" char* __wrap_game_file_read_crlf_line(char* buffer, int capacity,
                                                 struct GameFileHandle* handle)
{
    int len = 0;
    (void)handle;
    if (!s_cursor || *s_cursor == '\0') return NULL;
    while (*s_cursor != '\0' && *s_cursor != '\n') {
        if (len < capacity - 1) buffer[len++] = *s_cursor;
        s_cursor++;
    }
    if (*s_cursor == '\n') s_cursor++;
    buffer[len] = '\0';
    return buffer;
}

extern "C" int __wrap_game_file_close(struct GameFileHandle* handle)
{
    (void)handle;
    s_cursor = NULL;
    return 0;
}

/* ---- Scenes ---- */

/* One null whose keys run from (0, 0, 0) at frame 0 to (x, y, z) at
 * frame `lastKey`; the scene ends at frame 15. LightWave indents the key
 * lines, so the parser's first field on each is empty, and the parser
 * counts the object at its ShadowOptions line. */
static void scene_text(char* out, size_t size, int lastKey)
{
    snprintf(out, size,
             "LWSC\n"
             "1\n"
             "\n"
             "FirstFrame 0\n"
             "LastFrame 15\n"
             "FramesPerSecond 25\n"
             "\n"
             "AddNullObject Carry\n"
             "ObjectMotion (unnamed)\n"
             "  9\n"
             "  2\n"
             "  0 0 0 0 0 0 1 1 1\n"
             "  0 0 0 0 0\n"
             "  4 5 6 0 0 0 1 1 1\n"
             "  %d 0 0 0 0\n"
             "EndBehavior 1\n"
             "ShadowOptions 7\n",
             lastKey);
}

static int matrix_finite(D3DRMMATRIX4D m)
{
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            if (!isfinite(m[r][c])) return 0;
    return 1;
}

static IDirect3DRMFrame3* only_child(IDirect3DRMFrame3* parent)
{
    IDirect3DRMFrameArray* children = NULL;
    IDirect3DRMFrame* child = NULL;
    IDirect3DRMFrame3* child3 = NULL;
    if (parent->GetChildren(&children) != DD_OK || !children) return NULL;
    if (children->GetSize() == 1 && children->GetElement(0, &child) == DD_OK && child) {
        child->QueryInterface(IID_IDirect3DRMFrame3, (void**)&child3);
        child->Release();
    }
    children->Release();
    return child3;
}

/* Load the scene, set it to `time`, and check the null's frame is at
 * (x, y, z) with a finite transform. */
static void check_scene(IDirect3DRM3* d3drm, int lastKey, float time, float x, float y, float z,
                        const char* what)
{
    char text[1024];
    char message[256];
    IDirect3DRMFrame3* root = NULL;
    IDirect3DRMFrame3* carry = NULL;
    LwsInfo* scene;
    D3DRMMATRIX4D m;

    scene_text(text, sizeof(text), lastKey);
    s_text = text;
    scene = lws_parse("Scene", 0);
    s_text = NULL;
    snprintf(message, sizeof(message), "%s: the scene parses", what);
    CHECK(scene != NULL, message);
    if (!scene) return;

    CHECK(d3drm->CreateFrame(NULL, &root) == DD_OK && root, "root frame");
    if (!root) return;
    lws_meshes_load(scene, root);
    carry = only_child(root);
    snprintf(message, sizeof(message), "%s: one frame for the null", what);
    CHECK(carry != NULL, message);
    if (carry) {
        lws_time_set(scene, time);
        memset(m, 0, sizeof(m));
        CHECK(carry->GetTransform(root, m) == DD_OK, "the null's transform");
        snprintf(message, sizeof(message), "%s: the null's transform is finite", what);
        CHECK(matrix_finite(m), message);
        snprintf(message, sizeof(message), "%s: the null is at (%g, %g, %g)", what, m[3][0],
                 m[3][1], m[3][2]);
        CHECK(m[3][0] == x && m[3][1] == y && m[3][2] == z, message);
        carry->Release();
    }
    lws_free(scene);
    root->Release();
}

int main(void)
{
    IDirect3DRM* d3drm1 = NULL;
    IDirect3DRM3* d3drm = NULL;

    if (Direct3DRMCreate(&d3drm1) != DD_OK || !d3drm1 ||
        d3drm1->QueryInterface(IID_IDirect3DRM3, (void**)&d3drm) != DD_OK || !d3drm) {
        printf("FAIL: Direct3DRMCreate\n");
        return 1;
    }
    d3drm1->Release();
    s_main_d3drm = d3drm;

    /* The last key on the last frame: the parser leaves the spare slot
     * unwritten. */
    check_scene(d3drm, 15, 15.0f, 4.0f, 5.0f, 6.0f, "last key on the last frame");
    /* Past the last frame, a non-looping scene stops on it. */
    check_scene(d3drm, 15, 40.0f, 4.0f, 5.0f, 6.0f, "time past the last frame");
    /* The last key before the last frame: the parser copies it into the
     * spare slot and counts it, so the slot after is past the keys. */
    check_scene(d3drm, 10, 15.0f, 4.0f, 5.0f, 6.0f, "last key before the last frame");
    /* Between the keys, the blend is unchanged. */
    check_scene(d3drm, 15, 7.5f, 2.0f, 2.5f, 3.0f, "halfway between the keys");

    s_main_d3drm = NULL;
    d3drm->Release();

    if (s_failures) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("lws: all checks passed\n");
    return 0;
}

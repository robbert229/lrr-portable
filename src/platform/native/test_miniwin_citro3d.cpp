// SPDX-License-Identifier: LGPL-3.0-or-later
// The arithmetic of miniwin's Citro3D immediate mode (citro3dimmediate.h),
// which builds on any host; the backend itself runs only on a 3DS:
// - Direct3D clip space turned onto the sideways framebuffer, with the
//   reversed depth retained mode's projections give;
// - screen rectangles as the framebuffer's;
// - depth tests reversed to match;
// - texture stage 0 as PICA texture combiners, colour keys as an alpha test;
// - fog as a table indexed by depth.
#include "citro3dimmediate.h"

#include <math.h>
#include <stdio.h>

static int failures;

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) {                                                     \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__,         \
                    __LINE__, #expr);                                      \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

using namespace Citro3DImmediate;

static bool Near(float a, float b)
{
    return fabsf(a - b) < 1.0e-4f;
}

static DWORD Op(D3DTEXTUREOP op)
{
    return static_cast<DWORD>(op);
}

// Mtx_OrthoTilt turns x and y as (x, y) -> (y, -x); depth runs from window
// 1 at the near plane to 0 at the far one, PICA's z/w from -1 to 0.
static void test_clip()
{
    const float near[4] = {0.25f, -0.5f, 0.0f, 2.0f};
    float out[4];
    ClipFromD3D(near, out);
    CHECK(Near(out[0], -0.5f));
    CHECK(Near(out[1], -0.25f));
    CHECK(Near(out[2] / out[3], -1.0f));
    CHECK(Near(out[3], 2.0f));

    const float far[4] = {0.0f, 0.0f, 3.0f, 3.0f};
    ClipFromD3D(far, out);
    CHECK(Near(out[2] / out[3], 0.0f));
}

// The bottom screen is 320x240; the framebuffer under it is 240x320, its x
// along the screen's height from the bottom, its y along the screen's
// width from the right.
static void test_target_rect()
{
    SDL_Rect whole = TargetRect({0, 0, 320, 240}, 320, 240);
    CHECK(whole.x == 0 && whole.y == 0 && whole.w == 240 && whole.h == 320);

    SDL_Rect corner = TargetRect({0, 0, 10, 20}, 320, 240);
    CHECK(corner.x == 220 && corner.y == 310 && corner.w == 20 && corner.h == 10);

    SDL_Rect inner = TargetRect({100, 50, 60, 40}, 320, 240);
    CHECK(inner.x == 150 && inner.y == 160 && inner.w == 40 && inner.h == 60);
}

static void test_depth_func()
{
    CHECK(ReversedDepthFunc(D3DCMP_LESS) == D3DCMP_GREATER);
    CHECK(ReversedDepthFunc(D3DCMP_LESSEQUAL) == D3DCMP_GREATEREQUAL);
    CHECK(ReversedDepthFunc(D3DCMP_GREATER) == D3DCMP_LESS);
    CHECK(ReversedDepthFunc(D3DCMP_GREATEREQUAL) == D3DCMP_LESSEQUAL);
    CHECK(ReversedDepthFunc(D3DCMP_EQUAL) == D3DCMP_EQUAL);
    CHECK(ReversedDepthFunc(D3DCMP_NOTEQUAL) == D3DCMP_NOTEQUAL);
    CHECK(ReversedDepthFunc(D3DCMP_ALWAYS) == D3DCMP_ALWAYS);
    CHECK(ReversedDepthFunc(D3DCMP_NEVER) == D3DCMP_NEVER);
}

static StageState Modulate()
{
    StageState s;
    s.colorOp = Op(D3DTOP_MODULATE);
    s.colorArg1 = D3DTA_TEXTURE;
    s.colorArg2 = D3DTA_DIFFUSE;
    s.alphaOp = Op(D3DTOP_SELECTARG1);
    s.alphaArg1 = D3DTA_TEXTURE;
    s.alphaArg2 = D3DTA_DIFFUSE;
    return s;
}

static bool Is(const Combiner& c, Func func, Source s0, Operand o0)
{
    return c.func == func && c.scale == 1 && c.source[0] == s0 && c.operand[0] == o0;
}

static void test_combiners()
{
    // Untextured, or COLOROP DISABLE: the diffuse colour and alpha.
    Combiners plain = PlanCombiners(Modulate(), false, false, true);
    CHECK(plain.count == 1 && !plain.alphaTest);
    CHECK(Is(plain.stage[0].rgb, Func::Replace, Source::Primary, Operand::Color));
    CHECK(Is(plain.stage[0].alpha, Func::Replace, Source::Primary, Operand::Alpha));
    StageState disabled = Modulate();
    disabled.colorOp = Op(D3DTOP_DISABLE);
    CHECK(Is(PlanCombiners(disabled, true, false, false).stage[0].rgb, Func::Replace, Source::Primary, Operand::Color));

    // The game's stage: texture times diffuse, and the texture's alpha.
    Combiners textured = PlanCombiners(Modulate(), true, false, true);
    CHECK(textured.count == 1 && !textured.alphaTest);
    const Combiner& rgb = textured.stage[0].rgb;
    CHECK(Is(rgb, Func::Modulate, Source::Texture, Operand::Color));
    CHECK(rgb.source[1] == Source::Primary && rgb.operand[1] == Operand::Color);
    CHECK(Is(textured.stage[0].alpha, Func::Replace, Source::Texture, Operand::Alpha));

    // SELECTARG2 picks the second argument; MODULATE2X and 4X scale; ADD.
    StageState other = Modulate();
    other.colorOp = Op(D3DTOP_SELECTARG2);
    other.alphaOp = Op(D3DTOP_MODULATE4X);
    Combiners picked = PlanCombiners(other, true, false, false);
    CHECK(Is(picked.stage[0].rgb, Func::Replace, Source::Primary, Operand::Color));
    CHECK(picked.stage[0].alpha.func == Func::Modulate && picked.stage[0].alpha.scale == 4);
    other.colorOp = Op(D3DTOP_MODULATE2X);
    other.alphaOp = Op(D3DTOP_ADD);
    picked = PlanCombiners(other, true, false, false);
    CHECK(picked.stage[0].rgb.func == Func::Modulate && picked.stage[0].rgb.scale == 2);
    CHECK(picked.stage[0].alpha.func == Func::Add);

    // Complement and alpha replicate; TFACTOR reads white, SPECULAR black,
    // both from the white constant.
    other.colorOp = Op(D3DTOP_MODULATE);
    other.colorArg1 = D3DTA_TEXTURE | D3DTA_COMPLEMENT;
    other.colorArg2 = D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE;
    picked = PlanCombiners(other, true, false, false);
    CHECK(picked.stage[0].rgb.operand[0] == Operand::OneMinusColor);
    CHECK(picked.stage[0].rgb.operand[1] == Operand::Alpha);
    other.colorArg1 = D3DTA_TFACTOR;
    other.colorArg2 = D3DTA_SPECULAR;
    picked = PlanCombiners(other, true, false, false);
    CHECK(picked.stage[0].rgb.source[0] == Source::Constant && picked.stage[0].rgb.operand[0] == Operand::Color);
    CHECK(picked.stage[0].rgb.source[1] == Source::Constant);
    CHECK(picked.stage[0].rgb.operand[1] == Operand::OneMinusColor);

    // A keyed texture's texels are opaque: its alpha reads white. With
    // COLORKEYENABLE a second stage multiplies in the texel's alpha, which
    // is 0 on the key, for the alpha test to drop.
    Combiners keyed = PlanCombiners(Modulate(), true, true, false);
    CHECK(keyed.count == 1 && !keyed.alphaTest);
    CHECK(Is(keyed.stage[0].alpha, Func::Replace, Source::Constant, Operand::Alpha));
    CHECK(keyed.stage[0].rgb.source[0] == Source::Texture);
    Combiners keyedOut = PlanCombiners(Modulate(), true, true, true);
    CHECK(keyedOut.count == 2 && keyedOut.alphaTest);
    CHECK(Is(keyedOut.stage[1].rgb, Func::Replace, Source::Previous, Operand::Color));
    CHECK(Is(keyedOut.stage[1].alpha, Func::Modulate, Source::Previous, Operand::Alpha));
    CHECK(keyedOut.stage[1].alpha.source[1] == Source::Texture);
    CHECK(keyedOut.stage[1].alpha.operand[1] == Operand::Alpha);

    // The test drops texels under half covered, of the draw's alpha.
    CHECK(KeyAlphaTestRef(255) == 128);
    CHECK(KeyAlphaTestRef(1) == 1);
}

// A D3D perspective projection, near 1 and far 101, as the viewport's.
static void Perspective(D3DRMMATRIX4D p, float nearZ, float farZ)
{
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            p[r][c] = 0.0f;
        }
    }
    p[0][0] = 1.5f;
    p[1][1] = 2.0f;
    p[2][2] = farZ / (farZ - nearZ);
    p[2][3] = 1.0f;
    p[3][2] = -nearZ * farZ / (farZ - nearZ);
}

static void test_fog()
{
    D3DRMMATRIX4D p;
    Perspective(p, 1.0f, 101.0f);
    CHECK(Near(FogViewZ(p, 1.0f), 1.0f));
    CHECK(Near(FogViewZ(p, 0.0f), 101.0f));
    // Halfway in window depth is the harmonic, not linear, middle.
    const float z = FogViewZ(p, 0.5f);
    float clip[4] = {0.0f, 0.0f, p[2][2] * z + p[3][2], z};
    float out[4];
    ClipFromD3D(clip, out);
    CHECK(Near(-out[2] / out[3], 0.5f));

    // FogLut_FromArray's layout: 128 values from far (depth 0), then the
    // 128 steps between them and the next.
    float data[256];
    FogTable(p, [](float viewZ) { return viewZ < 51.0f ? 1.0f : 0.0f; }, data);
    CHECK(data[0] == 0.0f);
    CHECK(data[127] == 1.0f);
    for (int i = 0; i < 127; ++i) {
        CHECK(Near(data[128 + i], data[i + 1] - data[i]));
    }
    CHECK(Near(data[255], 1.0f - data[127]));
}

int main()
{
    test_clip();
    test_target_rect();
    test_depth_func();
    test_combiners();
    test_fog();
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("miniwin citro3d immediate: ok\n");
    return 0;
}

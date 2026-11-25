// SPDX-License-Identifier: LGPL-3.0-or-later
/* Standalone FLIC decoding and playback over original, tiny colored grids. */
#include <array>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <vector>

#include "platform/native/test_fixture.h"
#include <miniwin/ddraw.h>
#include "gods98/directdraw.h"
#include "gods98/files.h"
#include "gods98/flic_link_names.h"
#include "gods98/flic_unit.h"
#include "gods98/images.h"
#include "test/lrr_test.h"

namespace {
using Bytes = std::vector<unsigned char>;
constexpr unsigned width = 5, height = 3;
using Grid = std::array<uint16_t, width * height>;
constexpr uint16_t red = 0xf800, green = 0x07e0, blue = 0x001f;
constexpr uint16_t red15 = 0x7c00, green15 = 0x03e0, blue15 = 0x001f;
constexpr uint16_t green16 = 0x07c0; // 5-bit source green expanded to the high five bits.
constexpr uint16_t paletteMagic = 0xaf12, wordMagic = 0xaf43;
constexpr uint16_t colour256 = 4, deltaFlc = 7, byteRun = 15, wordRun = 25, wordDelta = 27;
bool completed;

void le16(Bytes& out, uint16_t value)
{
    out.push_back(static_cast<unsigned char>(value));
    out.push_back(static_cast<unsigned char>(value >> 8));
}

void le32(Bytes& out, uint32_t value)
{
    le16(out, static_cast<uint16_t>(value));
    le16(out, static_cast<uint16_t>(value >> 16));
}

Bytes chunk(uint16_t type, Bytes payload)
{
    if (payload.size() % 2) payload.push_back(0);
    Bytes out;
    le32(out, static_cast<uint32_t>(6 + payload.size()));
    le16(out, type);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

Bytes frame(std::initializer_list<Bytes> chunks)
{
    unsigned size = 16;
    for (const auto& bytes : chunks) size += static_cast<unsigned>(bytes.size());
    Bytes out;
    le32(out, size);
    le16(out, 0xf1fa);
    le16(out, static_cast<uint16_t>(chunks.size()));
    out.resize(16, 0);
    for (const auto& bytes : chunks) out.insert(out.end(), bytes.begin(), bytes.end());
    return out;
}

Bytes palette()
{
    // One full 256-colour packet; only the first four entries have values.
    Bytes payload = {1, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255};
    payload.resize(4 + 256 * 3, 0);
    return chunk(colour256, payload);
}

Bytes indexed_run()
{
    // Three rows: a run followed by literals, one run, then all literals.
    return chunk(byteRun, {
        2, 2, 1, 253, 2, 3, 0,
        1, 5, 2,
        1, 251, 3, 1, 2, 0, 3
    });
}

Bytes indexed_delta()
{
    // Two changed rows. Row 0 has a repeated pair and an odd final pixel;
    // skip row 1, then replace two pixels starting at x=1 in row 2.
    Bytes payload;
    le16(payload, 2);
    le16(payload, 0x8002);
    le16(payload, 1);
    payload.insert(payload.end(), {0, 254, 3, 1});
    le16(payload, 0xffff);
    le16(payload, 1);
    payload.insert(payload.end(), {1, 1, 0, 2});
    return chunk(deltaFlc, payload);
}

Bytes word_run()
{
    Bytes payload = {2, 2};
    le16(payload, red15);
    payload.push_back(253);
    for (uint16_t p : {green15, blue15, uint16_t(0)}) le16(payload, p);
    payload.insert(payload.end(), {1, 5});
    le16(payload, green15);
    payload.insert(payload.end(), {1, 251});
    for (uint16_t p : {blue15, red15, green15, uint16_t(0), blue15}) le16(payload, p);
    return chunk(wordRun, payload);
}

Bytes word_delta()
{
    Bytes payload;
    le16(payload, 2);
    le16(payload, 2);
    payload.insert(payload.end(), {0, 254});
    le16(payload, blue15);
    payload.insert(payload.end(), {1, 2});
    le16(payload, red15);
    le16(payload, green15);
    le16(payload, 0xffff);
    le16(payload, 1);
    payload.insert(payload.end(), {1, 1});
    le16(payload, 0);
    return chunk(wordDelta, payload);
}

Grid initial(uint16_t r, uint16_t g, uint16_t b)
{
    return {r, r, g, b, 0, g, g, g, g, g, b, r, g, 0, b};
}

Grid changed(bool words, uint16_t r, uint16_t g, uint16_t b)
{
    if (words) return {b, b, g, r, g, g, g, g, g, g, b, 0, g, 0, b};
    return {b, r, b, r, g, g, g, g, g, g, b, 0, g, 0, b};
}

void check_grid(const uint16_t* pixels, unsigned stride, const Grid& expected)
{
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x)
            CHECK_EQ(pixels[y * stride + x], expected[y * width + x]);
}

void check_decoder(bool words, bool output15)
{
    constexpr unsigned stride = 8;
    constexpr uint16_t padding = 0x1234;
    std::array<uint16_t, stride * height> pixels;
    pixels.fill(padding);
    FLICSTRUCT flic = {};
    flic.fsXsize = width;
    flic.fsYsize = height;
    flic.fsPitch = stride * sizeof(uint16_t);
    flic.fsSPtr = pixels.data();
    flic.fsDisplayMode = FLICMODE_HICOLOR;
    flic.fsHeader.depth = words ? 16 : 8;
    flic.fsHeader.magic = words ? wordMagic : paletteMagic;
    flic.is15bit = output15;
    auto colours = palette();
    if (!words) {
        flic.fsSource = colours.data();
        CHECK_EQ(Flic_DoChunk(&flic), 1);
        CHECK_EQ(Flic_GetPaletteColour(0, &flic), 0u);
        CHECK_EQ(Flic_GetPaletteColour(255, &flic), 0u);
    }
    auto run = words ? word_run() : indexed_run();
    auto delta = words ? word_delta() : indexed_delta();
    const uint16_t r = output15 ? red15 : red;
    const uint16_t g = output15 ? green15 : words ? green16 : green;
    flic.fsSource = run.data();
    CHECK_EQ(Flic_DoChunk(&flic), 1);
    check_grid(pixels.data(), stride, initial(r, g, blue));
    flic.fsSource = delta.data();
    CHECK_EQ(Flic_DoChunk(&flic), 1);
    check_grid(pixels.data(), stride, changed(words, r, g, blue));
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = width; x < stride; ++x)
            CHECK_EQ(pixels[y * stride + x], padding);
}

bool write_animation(const char* path, bool words)
{
    const Bytes first = words ? frame({word_run()}) : frame({palette(), indexed_run()});
    const Bytes second = words ? frame({word_delta()}) : frame({indexed_delta()});
    // Full-color ring frame returns to the initial grid.
    const Bytes ring = words ? frame({word_run()}) : frame({indexed_run()});
    Bytes out;
    le32(out, static_cast<uint32_t>(128 + first.size() + second.size() + ring.size()));
    le16(out, words ? wordMagic : paletteMagic);
    le16(out, 2);
    le16(out, width);
    le16(out, height);
    le16(out, words ? 16 : 8);
    le16(out, 0);
    le32(out, 40);
    out.resize(128, 0);
    for (const auto* bytes : {&first, &second, &ring})
        out.insert(out.end(), bytes->begin(), bytes->end());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(out.data()), out.size());
    file.close();
    return file.good();
}

void check_surface(IDirectDrawSurface4* surface, const Grid& expected)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    if (!CHECK_EQ(surface->Lock(nullptr, &desc, DDLOCK_WAIT, nullptr), DD_OK)) return;
    CHECK_EQ(desc.ddpfPixelFormat.dwRGBBitCount, 16u);
    CHECK_EQ(desc.ddpfPixelFormat.dwGBitMask, 0x07e0u);
    check_grid(static_cast<const uint16_t*>(desc.lpSurface), desc.lPitch / sizeof(uint16_t), expected);
    CHECK_EQ(surface->Unlock(nullptr), DD_OK);
}

void check_playback(const char* name, bool words, unsigned flags)
{
    LPFLICSTRUCT flic = nullptr;
    if (!CHECK(Flic_Setup(name, &flic, flags) && flic, "open generated animation")) return;
    CHECK_EQ(Flic_GetWidth(flic), static_cast<int>(width));
    CHECK_EQ(Flic_GetHeight(flic), static_cast<int>(height));
    CHECK_EQ(flic->fsHeader.depth, words ? 16u : 8u);
    const Area2F dest = {0, 0, width, height};
    const auto first = initial(red, words ? green16 : green, blue);
    const auto second = changed(words, red, words ? green16 : green, blue);
    // The first display forces a decode even when advance is false.
    CHECK_EQ(Flic_Animate(flic, &dest, 0, 0), 1);
    CHECK_EQ(flic->currentframe, 1);
    check_surface(flic->fsSurface, first);
    check_surface(directDrawGlobs.bSurf, first);
    // A paused draw still blits, with source colour zero transparent.
    constexpr uint16_t background = 0x39e7;
    DDBLTFX fill = {};
    fill.dwSize = sizeof(fill);
    fill.dwFillColor = background;
    CHECK_EQ(directDrawGlobs.bSurf->Blt(nullptr, nullptr, nullptr,
                                       DDBLT_COLORFILL | DDBLT_WAIT, &fill), DD_OK);
    Grid transparent = first;
    for (auto& pixel : transparent) {
        if (pixel == 0) pixel = background;
    }
    CHECK_EQ(Flic_Animate(flic, &dest, 0, 1), 1);
    CHECK_EQ(flic->overallframe, 1);
    check_surface(flic->fsSurface, first);
    check_surface(directDrawGlobs.bSurf, transparent);
    CHECK_EQ(Flic_Animate(flic, &dest, 1, 0), 1);
    CHECK_EQ(flic->currentframe, 2);
    check_surface(flic->fsSurface, second);
    check_surface(directDrawGlobs.bSurf, second);
    const bool looping = (flags & FLICLOOPINGON) != 0;
    CHECK_EQ(Flic_Animate(flic, &dest, 1, 0), looping ? 1 : 0);
    CHECK_EQ(flic->overallframe, 3);
    check_surface(flic->fsSurface, first);
    if (looping) {
        CHECK_EQ(Flic_Animate(flic, &dest, 1, 0), 1);
        check_surface(flic->fsSurface, (flags & FLICMEMORY) ? second : first);
        CHECK_EQ(flic->overallframe, 4);
    }
    CHECK_EQ(Flic_Close(flic), 1);
    CHECK_EQ(flic->fsLoadBuffer, nullptr);
    // Close releases the file and decode buffers; this caller owns the surface and record.
    flic->fsSurface->Release();
    std::free(flic);
}

void exit_guard()
{
    if (!completed) {
        std::fputs("FAIL: media test exited before completion\n", stderr);
        std::fflush(stderr);
        std::_Exit(1);
    }
}

void playback()
{
    if (!CHECK(fixture_enter("flic_media_fixture") && fixture_mkdir("Data"),
               "isolated media directory")) return;
    if (!CHECK(write_animation("Data/indexed.flc", false) &&
               write_animation("Data/words.flc", true), "write generated animations")) return;
    file_initialise("FlicMediaTest", 0, "");
    if (!CHECK(SDL_Init(SDL_INIT_VIDEO), SDL_GetError())) return;
    IDirectDraw* dd1 = nullptr;
    IDirectDraw4* dd = nullptr;
    IDirectDrawSurface4* back = nullptr;
    if (CHECK_EQ(DirectDrawCreate(nullptr, &dd1, nullptr), DD_OK) &&
        CHECK_EQ(dd1->QueryInterface(IID_IDirectDraw4, reinterpret_cast<void**>(&dd)), DD_OK)) {
        DDSURFACEDESC2 desc = {};
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        desc.dwWidth = width;
        desc.dwHeight = height;
        if (CHECK_EQ(dd->CreateSurface(&desc, &back, nullptr), DD_OK)) {
            directDrawGlobs.lpDirectDraw = dd;
            directDrawGlobs.bSurf = back;
            const unsigned modes[] = {0, FLICMEMORY, FLICMEMORY | FLICLOOPINGON, FLICLOOPINGON};
            for (unsigned flags : modes) {
                check_playback("indexed.flc", false, flags);
                check_playback("words.flc", true, flags);
            }
            directDrawGlobs.lpDirectDraw = nullptr;
            directDrawGlobs.bSurf = nullptr;
            back->Release();
        }
        dd->Release();
    }
    if (dd1) dd1->Release();
    SDL_Quit();
}
} // namespace

int main(int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "decode";
    if (!CHECK(mode == "decode" || mode == "playback", "known test mode")) return lrr_test_result();
    std::atexit(exit_guard);
    if (mode == "decode") {
        check_decoder(false, false);
        check_decoder(true, false);
        check_decoder(true, true);
    } else playback();
    completed = true;
    return lrr_test_result();
}

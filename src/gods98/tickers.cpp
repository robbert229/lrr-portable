// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstddef>

#include "tickers.hpp"

/* ------------------------------------------------------------------ *
 * Per-ticker stubs. Each `ticker_<hex>` corresponds to the corpus's
 * `FUN_<hex>`; fill the bodies in from engine-region.c one at a time.
 * The table order below is the observed EngineTick call order and must
 * stay stable (finding 03 §4; corpus lines 93-119).
 * ------------------------------------------------------------------ */

static void ticker_00425d90(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_00425d90 */
}

static void ticker_00402220(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_00402220 */
}

static void ticker_00424600(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_00424600(state, scale) */
}

static void ticker_0040aa10(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0040aa10 */
}

static void ticker_0043b270(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0043b270 */
}

static void ticker_0046f550(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0046f550 */
}

static void ticker_0040eab0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0040eab0 */
}

static void ticker_004313b0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_004313b0(state, scale) */
}

static void ticker_004520d0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_004520d0 (selection root) */
}

static void ticker_00435f5c(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_00435f5c — one of the three view-context ticks
     * (DAT_005560e4 / e8 / ec); the table lists the entry three times
     * because the corpus calls it three times, once per view. */
}

static void ticker_004091e0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_004091e0 */
}

static void ticker_0041ba10(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0041ba10 */
}

static void ticker_004291a0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_004291a0(dt, 0) — render pass entry */
}

static void ticker_00449c00(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_00449c00 */
}

static void ticker_0040d450(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0040d450 */
}

static void ticker_00426520(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_00426520(dt, raw-dt, sim_out1..3) — consumes the
     * MainSimStep outs; the unison signature flattens them for now. */
}

static void ticker_0044eff0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0044eff0(state+4, scale) */
}

static void ticker_0046e390(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0046e390(state, scale) */
}

static void ticker_0047aba0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0047aba0 */
}

static void ticker_00465b00(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_00465b00 */
}

static void ticker_004665c0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_004665c0 */
}

static void ticker_0044cfa0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0044cfa0 */
}

static void ticker_0041a2c0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* TODO: corpus FUN_0041a2c0 */
}

typedef struct TickerEntry {
    const char*  id;        /* corpus FUN_<hex> */
    void (*fn)(EngineContext*, float, float);
    uint32_t     skip_when; /* skip when (ctx->state & mask) — 0 = always run */
} TickerEntry;

static const TickerEntry TICKERS[] = {
    { "FUN_00425d90", ticker_00425d90, ENGINE_STATE_TIME_FROZEN },
    { "FUN_00402220", ticker_00402220, 0 },
    { "FUN_00424600", ticker_00424600, 0 },
    { "FUN_0040aa10", ticker_0040aa10, 0 },
    { "FUN_0043b270", ticker_0043b270, 0 },
    { "FUN_0046f550", ticker_0046f550, 0 },
    { "FUN_0040eab0", ticker_0040eab0, 0 },
    { "FUN_004313b0", ticker_004313b0, 0 },
    { "FUN_004520d0", ticker_004520d0, 0 },
    { "FUN_00435f5c", ticker_00435f5c, 0 }, /* view A (DAT_005560e4) */
    { "FUN_00435f5c", ticker_00435f5c, 0 }, /* view B (DAT_005560e8) */
    { "FUN_00435f5c", ticker_00435f5c, 0 }, /* view C (DAT_005560ec) */
    { "FUN_004091e0", ticker_004091e0, 0 },
    { "FUN_0041ba10", ticker_0041ba10, 0 },
    { "FUN_004291a0", ticker_004291a0, 0 },
    { "FUN_00449c00", ticker_00449c00, 0 },
    { "FUN_0040d450", ticker_0040d450, 0 },
    { "FUN_00426520", ticker_00426520, 0 },
    { "FUN_0044eff0", ticker_0044eff0, 0 },
    { "FUN_0046e390", ticker_0046e390, 0 },
    { "FUN_0047aba0", ticker_0047aba0, 0 },
    { "FUN_00465b00", ticker_00465b00, 0 },
    { "FUN_004665c0", ticker_004665c0, 0 },
    { "FUN_0044cfa0", ticker_0044cfa0, 0 },
    { "FUN_0041a2c0", ticker_0041a2c0, 0 },
};

/* The ordered EngineTick subsystem sequence (finding 03 §4; corpus 93-119). */
void EngineSubsystemTickers(EngineContext* ctx, float dt, float scale)
{
    for (int i = 0; i < (int)(sizeof(TICKERS) / sizeof(TICKERS[0])); ++i) {
        if ((TICKERS[i].skip_when != 0) && (ctx->state & TICKERS[i].skip_when)) {
            continue;
        }
        TICKERS[i].fn(ctx, dt, scale);
        ++ctx->tickers_ran;
    }
}

/* Camera / move cluster at the top of EngineTick — corpus lines 52-88:
 *   FUN_0046a730, FUN_00435a50, FUN_00431560, FUN_0042c440, FUN_0042cb70,
 *   FUN_004261c0, then the screen->world projection solving the camera
 *   position and the force-scale one-shot (ENGINE_STATE_FORCE_SCALE_1).
 */
void EngineCameraCluster(EngineContext* ctx, float dt, float scale)
{
    (void)dt; (void)scale;
    if ((ctx->state & ENGINE_STATE_FORCE_SCALE_1) != 0) {
        ctx->state &= ~ENGINE_STATE_FORCE_SCALE_1;
        /* TODO(engine): one-shot "scale = 1.0 this frame". */
    }
    /* TODO(engine): the six camera functions above, once mapped. */
    (void)ctx;
}

/* Indexed manager dispatch — corpus ManagerTick FUN_0045a730. Under
 * ENGINE_STATE_INDEXED_SUBSYS the corpus calls it with idx
 *   1, 2, 4, 9, 6, 8, 0xb, 5, 7, 10
 * interleaved with the named manager tickers FUN_0045d6d0, FUN_0040e7f0,
 * FUN_0041b490, FUN_0041b930, FUN_0041a080, FUN_00419f30. Once an index is
 * identified (AI, particles, FX…), give it a name here.
 */
static const int MANAGER_ORDER[] = { 1, 2, 4, 9, 6, 8, 0xb, 5, 7, 10 };

static void ManagerTick(EngineContext* ctx, int idx, float dt)
{
    (void)ctx; (void)dt;
    /* TODO(game): identified manager for index %d — see corpus FUN_0045a730. */
    (void)idx;
}

void EngineManagerDispatch(EngineContext* ctx, float dt)
{
    for (size_t i = 0; i < sizeof(MANAGER_ORDER) / sizeof(MANAGER_ORDER[0]); ++i) {
        ManagerTick(ctx, MANAGER_ORDER[i], dt);
        ++ctx->tickers_ran;
    }
}

/* Render begin/end pairs + world draw vs editor pass — corpus lines 130-215:
 * surface lock/unlock (FUN_00473b40), sprite/animation draw, FUN_00477150
 * (render target, dt), the in-game world drawers FUN_00435300 / FUN_00454600,
 * and the &2/&0x2000-gated second render pass (FUN_004291a0(scale,1) etc.).
 */
void EngineRenderFrame(EngineContext* ctx, float dt, float scale, uint32_t* sim_out1)
{
    (void)ctx; (void)dt; (void)scale; (void)sim_out1;
    /* TODO(engine): begin/end pairs once surfaces exist (miniwin DDraw). */
}

/* Air-supply beat mechanic — corpus EngineTick tail. When the air supply
 * crosses under the threshold, fire the air-low / air-empty notifications
 * (corpus FUN_00419b80 ids 0x10..0x13) and drive the looped SND_AirBeat
 * sound; air_prev (+0x204) is written back from air_beat (+0x1fc).
 */
void EngineAirBeat(EngineContext* ctx, float scale)
{
    (void)scale;
    EngineState* s = ctx->state_block;
    if (s != NULL) {
        s->air_prev = s->air_beat;
    }
    /* TODO(engine): threshold check + notification dispatch + sound hook. */
}
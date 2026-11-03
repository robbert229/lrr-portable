// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — subsystem ticker dispatch scaffold.
 *
 * The ordered list in tickers.cpp reproduces the EngineTick subsystem call
 * sequence (finding 03 §4; corpus lines 93-119). Every entry is a named stub
 * symbol (`ticker_<hex>` = the corpus's FUN_<hex>) whose body is filled in
 * from the corpus one at a time. The order must stay stable — it is the
 * observed call order, not a suggestion.
 *
 * This file is part of the engine core (src/gods98/); no OS-facing headers.
 */
#ifndef LEGORRR_TICKERS_HPP
#define LEGORRR_TICKERS_HPP

#include "audio.hpp"
#include "engine.hpp"

struct EntitySlot;

/* EngineTick's two positioned-entity calls need the view's +0x24 render-list
 * pointer. Keep that owner explicit until the portable view exposes it. */
typedef void* (*EngineRenderSceneListGetHook)(void* context,
                                               EngineContext* engine);
void engine_render_scene_list_get_hook_set(EngineRenderSceneListGetHook hook,
                                           void* context);
void engine_render_poly_ranges_set(float medium_range, float high_range);

/* Run the ordered EngineTick subsystem sequence (finding 03 §4). */
void EngineSubsystemTickers(EngineContext* ctx, float dt, float scale);
/* Production view dispatch at its native place before4091e0/4291a0.
 * Camera frame units are supplied by EngineTick, independently of AI time. */
int EngineSubsystemTickersWithViews(EngineContext* ctx, float dt, float scale,
                                     float camera_dt, float camera_scale);

/* Air-beat sound hook (finding 03 §7): point the loop at an AudioPlayer +
 * SND_AirBeat resource name; NULL player disables audio (messages still
 * dispatch). airbeat_loop_active() reports the engaged state. */
void airbeat_set_loop(AudioPlayer* p, const char* sound);
int  airbeat_loop_active(void);

/* Indexed manager dispatch (corpus ManagerTick FUN_0045a730). */
void EngineManagerDispatch(EngineContext* ctx, float dt);

/* Render begin/end + world-draw vs editor pass (corpus lines 130-215). */
void EngineRenderFrame(EngineContext* ctx, float dt, float scale, uint32_t* sim_out1);

/* Air-supply beat mechanic (corpus EngineTick tail). */
void EngineAirBeat(EngineContext* ctx, float scale);

#endif /* LEGORRR_TICKERS_HPP */

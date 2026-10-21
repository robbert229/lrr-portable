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

#include "engine.hpp"

/* Run the ordered EngineTick subsystem sequence (finding 03 §4). */
void EngineSubsystemTickers(EngineContext* ctx, float dt, float scale);

/* Camera / move cluster at the top of EngineTick (corpus lines 52-88). */
void EngineCameraCluster(EngineContext* ctx, float dt, float scale);

/* Indexed manager dispatch (corpus ManagerTick FUN_0045a730). */
void EngineManagerDispatch(EngineContext* ctx, float dt);

/* Render begin/end + world-draw vs editor pass (corpus lines 130-215). */
void EngineRenderFrame(EngineContext* ctx, float dt, float scale, uint32_t* sim_out1);

/* Air-supply beat mechanic (corpus EngineTick tail). */
void EngineAirBeat(EngineContext* ctx, float scale);

#endif /* LEGORRR_TICKERS_HPP */
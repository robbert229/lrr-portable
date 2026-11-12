// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <string.h>

#include "mainsim.hpp"
#include "message.hpp"
#include "view.hpp"
#include "game/entity.hpp"

/* --- boundaries ----------------------------------------------------------
 *  Selecting objects / menus / rename touch: FUN_004525c0 (SelectedEntity),
 *  FUN_00437880 (EntityRename) -> entity layer not landed; skipped (stub).
 *  QuitPoll corner-dwell: FUN_004589c0 -> input_quit_dwell (input.c) is the
 *  gesture model; MainSimStep is driven by the named quit key here.
 */

void main_hud_text(EngineContext* ctx, const char* s)
{
    (void)ctx;
    fprintf(stderr, "hud: %s\n", s != NULL ? s : "");
}

int main_sim_step(EngineContext* ctx, float scale, float dt,
                  uint32_t* o1, uint32_t* o2, uint32_t* o3)
{
    (void)dt;
    return main_sim_step(ctx, scale, o1, o2, o3);
}

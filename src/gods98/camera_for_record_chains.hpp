// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what record_chains.cpp sees from
 * camera.cpp. A verbatim snapshot of camera_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_CAMERA_FOR_RECORD_CHAINS_HPP
#define LRR_GODS98_CAMERA_FOR_RECORD_CHAINS_HPP

extern CameraMovePostCallback s_move_post_callback;
extern void* s_move_post_user;

#endif /* LRR_GODS98_CAMERA_FOR_RECORD_CHAINS_HPP */

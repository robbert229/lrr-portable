// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from message.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_MESSAGE_FOR_LEGO_HPP
#define LRR_GODS98_MESSAGE_FOR_LEGO_HPP

extern char* s_info_message_sound_names[MESSAGE_MAX_ID];

#endif /* LRR_GODS98_MESSAGE_FOR_LEGO_HPP */

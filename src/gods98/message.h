// SPDX-License-Identifier: LGPL-3.0-or-later
/* Text.c's message lookup for units rebuilt as C.
 *
 * C boundary header (ADR 0005) for gods98/message.hpp, whose matching
 * declaration carries extern "C"; src/game/test_c_boundaries.cpp includes
 * both. Shared C++ headers do not include this file (finding 506). */
#ifndef LEGORRR_GODS98_MESSAGE_H
#define LEGORRR_GODS98_MESSAGE_H


#ifdef __cplusplus
extern "C" {
#endif

/* Text_GetTextType (FUN_0046aad0). */
int ui_message_label_index_find(const char* label, int* out_index);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MESSAGE_H */

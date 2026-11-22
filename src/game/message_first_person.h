// SPDX-License-Identifier: LGPL-3.0-or-later
/* Message_EnterFirstPersonView's test of whether a unit can take the
 * first-person camera: its flags3 0x10 and, unless it is a minifigure, a
 * driven vehicle. An inline helper: with the test open-coded twice, MSVC 5.0
 * gives the unit loop's not-found exit its own epilogue, 4 bytes the
 * original does not have; through the helper the loop leaves by `jae` to the
 * shared `return 0`, as the original does.
 *
 * Only message_unit.c includes it, just before its caller, the unit's last
 * function, so no other function's symbol numbering changes (finding 538).
 * In a header it adds no Reccmp lint debt. */
#ifndef LEGORRR_GAME_MESSAGE_FIRST_PERSON_H
#define LEGORRR_GAME_MESSAGE_FIRST_PERSON_H

static __inline int message_object_can_first_person(Lego_Object* obj)
{
    return (obj->flags3 & 0x10) && (obj->type == 2 || obj->driveObject);
}

#endif /* LEGORRR_GAME_MESSAGE_FIRST_PERSON_H */

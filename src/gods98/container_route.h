// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native-only forwarders for the Container calls of the Creature unit
 * (game/animation_object.c, GAME-004). Each takes the same arguments as the
 * gods98 function it stands for, answers for a native Container
 * (container_native.hpp) and otherwise calls that function unchanged.
 *
 * gods98/container.h routes GAME-004's calls here in MINIWIN builds, so the
 * annotated Container functions (view.cpp, texture.cpp, resource.cpp,
 * record.cpp) and container_load (entity.cpp) carry no native code
 * (finding 555). The matching build never sees this file. */
#ifndef LEGORRR_GODS98_CONTAINER_ROUTE_H
#define LEGORRR_GODS98_CONTAINER_ROUTE_H

#endif /* LEGORRR_GODS98_CONTAINER_ROUTE_H */

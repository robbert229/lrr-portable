// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LRR_SYNTHETIC_INITIALIZER_FIXTURE_H
#define LRR_SYNTHETIC_INITIALIZER_FIXTURE_H

#include <string>
namespace synthetic_fixture {
extern const char CATALOG[];
bool prepare_catalog(const char* root = "Fixture");
std::string misc_resource_block();
// Optional additional abstract object stats for tests creating live objects.
bool initialise_full_game(const char* directory, const char* programName,
                          const char* extraStats = nullptr);
}
#endif

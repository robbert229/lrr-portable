# Finding 496 — Help-Window and Cell-Action State

`0x004DB8C4` marks that the menu overlay surface/text update ran. The overlay helper sets it after drawing the relevant state-specific resources; the menu loop resets it on entry.

`0x004DB978` and `0x004DC0F8` begin help-window info-line string tables. The loader fills 16 strings per configured entry, and the building-upgrade event dispatcher selects from these tables when updating the help panel.

`0x004DC648` stores the index of the help-window button selected by hit testing. `0x004DC658` begins the incident-message name table, including identifiers such as CrystalFound, UnderAttack, and AirLow; name lookup maps those strings to catalog indices.

`0x004DDD34` begins a 300-dword cell-action permission matrix. Level-state reset clears the matrix, and the permission getter indexes it by action category plus cell-kind index times 15 when a system-state flag enables the check.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).

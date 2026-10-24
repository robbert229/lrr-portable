# Finding 500 — Panel Names and Priority-Image Tables

`0x005000E0` begins twelve UI panel-name strings, including Radar, Messages, TopPanel, Information, PriorityList, and InfoDock. Case-insensitive lookup maps a name to its panel index.

`0x00500350` begins the panel-button name matrix. Initialization stores button identifiers, and lookup scans 24 entries in the row selected by panel index, with a `0x60`-byte row stride. `0x0050034C` is a shared UI panel and screen-effect state word; callers check bits 1, 2, and 4 for different behaviors, but the complete bit meanings are unresolved.

`0x0050080C` is a value that gates panel-resource position updates. The setter stores it, and the update helper requires it to be nonzero along with a separate interaction check.

`0x00500F00` begins the command-kind priority-image resource table. `PriorityImagesConfigLoad` stores image handles by command-kind index and the region interaction path draws the selected image. `0x005010B0` begins the corresponding sound-alias table used for selection feedback.

Complete pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).

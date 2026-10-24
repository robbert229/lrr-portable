# Finding 441 — Incident and Information Message Queue

This finding maps the configured incident-message catalog, its per-ID event queues, and the frame path that updates the scrolling information panel. The strongest evidence is the canonical pseudocode in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c); the `EngineTick` call order is preserved in [`LEGORR.supplement.c`](../decompiles/full/LEGORR.supplement.c). Related context is in [Finding 22](22-research-crumbs.md), [Finding 47](47-entity-events-feed-incident-messages.md), [Finding 57](57-incident-message-advisor-consumer.md), [Finding 246](246-entity-move-route-command-and-object-clone-helpers.md), [Finding 256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), and [Finding 372](372-incident-messages-options-and-resource-helpers.md).

## Catalog and config records

`IncidentMessageCatalogInit @ 0x00419230` installs 40 hardcoded `Info_*` names in order at `DAT_004dc658` through `DAT_004dc6f4`, stores its argument in `DAT_004dcd3c`, and ORs `0x18` into `DAT_004dcd48`. `GameConfigInit @ 0x0041fb50` calls it at startup and then loads the `InfoMessages` config section through `IncidentMessageConfigLoad @ 0x00434c10` (Finding 22 traces the startup callback registration). `IncidentMessageConfigLookup @ 0x004193e0` compares a supplied name case-insensitively against those 40 names and returns its slot through an output pointer. The catalog order and names are recorded in Finding 22.

Each per-ID config record has a `0x14`-byte stride beginning at `DAT_004dc6f8`. Direct stores identify the first dword as the compiled display template, the next as a loaded sound handle, the next as a secondary sound value, followed by a float timer and flags. `IncidentMessageConfigLoad` reads each entry name, resolves its catalog slot, parses the display string, loads the sound resource, optionally stores a secondary sound, and sets the per-entry `0x10000` and `0x20000` flag bits from its config options. `InfoMessageAlwaysPlayFlagSet @ 0x00419470` only sets or clears `0x10000`; the exact playback policy is controlled by its caller and the state gates in `MessageDispatch`.

`IncidentMessageTemplateReplace @ 0x004194b0` frees the slot's prior compiled template, then replaces it with the result of `IncidentMessageTemplateCompile @ 0x00419530`. The compiler changes underscores to spaces and `\\n` pairs to line breaks. It parses `$...$` spans through a resource lookup and, when that lookup fails, through the message-name catalog; successful spans become `%b` markers plus a compact trailing byte table. The exact authored meaning of every `$...$` token and the consumer-side contract for that trailing table are not established by this function alone. `InfoMessageSoundLoad @ 0x00419650` loads and retains a resource handle in the record's `+4` field; `InfoMessageSecondarySoundSet @ 0x00419680` stores its supplied value at `+8` without interpreting it. `IncidentMessageTextCatalogGet @ 0x00419420` returns the dword at `DAT_004dc6fc + index * 0x14`, which is the same `+4` field populated by `InfoMessageSoundLoad`; despite its current name, the checked-in body does not return the display-template field. Its precise token-compilation role remains an open question.

## Dispatch and per-message event queues

`MessageDispatch @ 0x00419b80` accepts a message ID and three payload arguments. It returns without queueing when the global message-enable bit `0x4` is clear or `SystemStateFlags @ 0x00456240` has bit `0x100` set. With global bit `0x8` set, it has two per-ID timer paths: when the `0x20000` entry flag is set, a due timer is reset to `25.0` and the secondary-sound helper is called with operation `0` before the precheck; when the flag is clear, the same timer/sound action occurs after the precheck and only if the configured template exists. Certain IDs reset a group of four timer fields in those paths. This describes the calls and state writes, not a guaranteed audible result. The dispatcher then invokes `MessagePreCheck @ 0x004199f0`; IDs `10`, `11`, `12`, and `0x26` bypass that check. For other IDs, the precheck searches an existing same-ID entry and dispatches a callback over its nodes; the callback's complete duplicate criterion is not recovered here.

If the configured display template exists and the precheck admits the event, the dispatcher finds or creates an active per-ID record, subject to the 40-record limit at `DAT_004dcd38`. A record has five dwords: linked-list head, node count, message ID, a float used as a display offset, and a float used by the update path. `IncidentMessageNodeCreate @ 0x00419a80` allocates a 20-byte node, copies two dwords from the first payload pointer or initializes them to `-1`, stores the second payload, compiles the optional third payload as a text override, and leaves the link field for the append helper. The consumers show the first two fields are used as event-specific coordinates/data, but not all producers use them the same way. The text consumer prefers the node's compiled override when present and otherwise uses the configured template.

`IncidentMessageQueueAppend @ 0x004196f0` appends the node to the selected record's linked list and increments its count. When traversal reaches its ninth-node condition it removes the head before appending, keeping the observed list bounded at about nine nodes. `IncidentMessageQueueRemoveAt @ 0x004197b0` bounds-checks an index, unlinks that node, updates the head when removing index zero, decrements the count, and returns the detached pointer; the helper does not free it. `MessageSubscriberDispatch @ 0x004196a0` is the shared list walker: it invokes a caller-supplied callback for each node and stops on the first nonzero result. `InfoMessageQueueFindId @ 0x00419b20` returns an active record index for a matching ID or `-1`; `FUN_00419b00` checks whether the ID's compiled template pointer is nonzero.

`IncidentMessageFlagSet @ 0x00419ae0` sets or clears the global `DAT_004dcd48` bit `0x4` tested by the dispatcher. `IncidentMessagePanelRefreshIfActive @ 0x00419da0` examines the first active record and calls a panel update helper when its node list and leading record fields meet the observed conditions. These operations establish the global gate and a first-record refresh path; they do not establish that every dispatched event is displayed or played.

## Frame update and information-panel text

The `EngineTick @ 0x004232e0` snapshot calls `InfoMessageTimerDecay @ 0x0041a2c0` before the in-game render/update branch. That helper walks all 40 per-ID timer fields at `DAT_004dc704` in `0x14`-byte steps and subtracts the frame delta from positive values. `KindSixRandomMessageDispatchSweep @ 0x0040f0e0` runs when its global gate is active: after its countdown expires it samples up to 100 random grid coordinates, accepts one through `FUN_0040f190`, dispatches message ID `6` with that coordinate pointer, and rearms the countdown to a value in the observed `750`–`799` range. Finding 256 captures this producer.

During the in-game branch, `EngineTick` calls `IncidentMessageTextConsumer @ 0x0041a080` and then `InfoMessageQueueTick @ 0x00419f30`. The text consumer checks global queue/display state; when the active display-update bit is set, it resets the text widget, chooses the head node's override string or the configured template for the leading message ID, appends the formatted text, updates the scroll position, and clears that update bit. If the queue is inactive it requests the corresponding panel transition. Finding 57 traces this consumer into the widget's ring buffer and records the limitations of the caller inventory.

`InfoMessageQueueTick` advances each active record's two float fields using the frame delta, clamps the first against `333`, updates the associated configured handle through `FUN_0047dcb0`, and updates a state-selected object through `IncidentMessageStateGet @ 0x0041a2f0` and `FUN_0047a180`. If the computed record value becomes negative, it updates the global handle at `DAT_004dcd40` and returns. `RemainingTimerDurationGet @ 0x00419b50` starts at `333` and subtracts the dword at offset `+8` of each active ID's configured handle. The relationship between these numeric fields, the rendered timing, and the game-facing duration is not fully identified by the pseudocode; those expressions are retained literally rather than assigned a stronger semantic label.

## Adjacent callback and timed-state helpers

`CatalogCallbackByIdDispatch @ 0x0041b680` is a separate catalog traversal helper used by advisor/input callers (see Finding 246). IDs `0x13`, `0x14`, and `0x15` enumerate fixed entity sets with distinct flag gates; other IDs walk the configured ID list at `DAT_004ddc04` and dispatch each value, resolving `0x22`/`0x23` through `SelectionEntityCodeResolve`. `CatalogSlotFlagUpdate @ 0x0041c800` handles catalog categories `1` and `4`, consulting per-slot flag arrays, global flag `0x80`, and eligibility helpers before clearing or setting slot bit `1`; its category meaning is unresolved. These helpers share callback/catalog infrastructure but are not the same queue as `DAT_004dca18`.

`TimedStatusAndEventTick @ 0x0041b930` decrements a timer, toggles `DAT_004ddd1c` and resets that timer to 15 seconds when it expires, calls `EventTimerCountdownAdvance @ 0x0041eac0`, and raises bit `0x800` in `DAT_004de1f8` once `_DAT_004de1ec` is no longer positive. It runs in `EngineTick`'s periodic in-game block. This is adjacent simulation status state; no direct call from it to the incident-message dispatcher is present in the checked-in body.

## Assigned-function map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `0x0040f0e0` | `KindSixRandomMessageDispatchSweep` | Samples a valid grid coordinate on timer expiry and dispatches message ID `6`. |
| `0x00419230` | `IncidentMessageCatalogInit` | Installs the 40 hardcoded message names and sets global flag bits `0x18`. |
| `0x004193e0` | `IncidentMessageConfigLookup` | Case-insensitive lookup of a message name in the 40-name catalog. |
| `0x00419420` | `IncidentMessageTextCatalogGet` | Returns the per-ID config dword at offset `+4`, populated by the sound loader; the current name may be misleading. |
| `0x00419470` | `InfoMessageAlwaysPlayFlagSet` | Sets or clears per-ID config flag `0x10000`. |
| `0x004194b0` | `IncidentMessageTemplateReplace` | Frees the previous compiled template and installs a newly compiled one. |
| `0x00419530` | `IncidentMessageTemplateCompile` | Converts underscores/newline escapes and compiles `$...$` spans into format markers and trailing bytes. |
| `0x00419650` | `InfoMessageSoundLoad` | Loads and retains a handle in the per-ID record's `+4` field. |
| `0x00419680` | `InfoMessageSecondarySoundSet` | Stores a supplied secondary-sound value at per-ID offset `+8`. |
| `0x004196a0` | `MessageSubscriberDispatch` | Visits a per-ID node list and stops when a callback returns nonzero. |
| `0x004196f0` | `IncidentMessageQueueAppend` | Appends an event node and removes the head at the observed ninth-node threshold. |
| `0x004197b0` | `IncidentMessageQueueRemoveAt` | Unlinks and returns a node by index without freeing it. |
| `0x004199f0` | `MessagePreCheck` | Applies the same-ID callback check, except for IDs `10`–`12` and `0x26`. |
| `0x00419a80` | `IncidentMessageNodeCreate` | Allocates a 20-byte node with payload fields and an optional compiled text override. |
| `0x00419ae0` | `IncidentMessageFlagSet` | Sets or clears global message-enable bit `0x4`. |
| `0x00419b00` | `FUN_00419b00` | Tests whether the selected per-ID display-template pointer is nonzero. |
| `0x00419b20` | `InfoMessageQueueFindId` | Returns the active record index for an ID or `-1`. |
| `0x00419b50` | `RemainingTimerDurationGet` | Starts at `333` and subtracts offset `+8` values from active IDs' configured handles. |
| `0x00419b80` | `MessageDispatch` | Gates, prechecks, and queues an event under a configured message ID. |
| `0x00419da0` | `IncidentMessagePanelRefreshIfActive` | Requests a panel refresh from the leading active record when its guards pass. |
| `0x00419f30` | `InfoMessageQueueTick` | Advances active-record timing/position state and updates associated handles. |
| `0x0041a080` | `IncidentMessageTextConsumer` | Renders the leading event's override or configured template into the scrolling widget. |
| `0x0041a2c0` | `InfoMessageTimerDecay` | Decrements positive per-ID timers by the frame delta. |
| `0x0041a2f0` | `IncidentMessageStateGet` | Returns the global state object used by the queue update path. |
| `0x0041b680` | `CatalogCallbackByIdDispatch` | Enumerates configured entity/catalog values and invokes a callback with category gates. |
| `0x0041b930` | `TimedStatusAndEventTick` | Advances a periodic state timer and raises event bit `0x800` after a second countdown. |
| `0x0041c800` | `CatalogSlotFlagUpdate` | Updates slot flag bit `1` for catalog categories `1` and `4` under eligibility gates. |
| `0x00434c10` | `IncidentMessageConfigLoad` | Parses the `InfoMessages` section into per-ID template, sound, timer, and flag records. |

## Open questions and follow-up slices

- Recover the callback predicate at `LAB_004199a0` and establish exactly which message payloads it treats as duplicates, including the special-ID exceptions.
- Validate the `$...$` template syntax and the trailing `%b` data against shipped `InfoMessages` entries; resolve why `IncidentMessageTextCatalogGet` reads the sound-handle field.
- Trace the resource and display effects driven by the queue's float fields and `FUN_0047dcb0`, including the ownership/freeing of nodes returned by `IncidentMessageQueueRemoveAt`.
- Identify the meanings of `DAT_004dcd48` bits beyond the direct setter/test relationships documented here, and determine when the leading queue record is reordered or removed.
- Confirm which `CatalogCallbackByIdDispatch` categories and `CatalogSlotFlagUpdate` arrays belong to this investigation's message path; current callers point to adjacent advisor/input catalog logic instead.

This is static analysis of the checked-in canonical pseudocode and its supplemental `EngineTick` snapshot. No runtime execution or authored-config parsing was performed for this finding.

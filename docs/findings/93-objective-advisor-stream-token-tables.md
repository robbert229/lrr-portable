# 93 — Objective advisor stream builds separate resource and sound token tables

The level objective/advisor stream loader (`FUN_00452e70`) separates marked
records from ordinary text records. A leading `:` record is loaded through
`FUN_0047d9d0` and stored in the resource-name table at `DAT_00555db0` with its
handle; a leading `$` record is loaded through `FUN_0047a870` and stored in a
second name/handle table at `DAT_00555db8`. Other nonempty records are added to
the indexed text array exposed by `FUN_00453200`. The loader also changes `_`
to a space while walking the input buffer. The level-load function
`FUN_00429890` calls this loader.

This is the objective/advisor stream resource grammar. It is separate from the
`InfoMessages` template compiler in finding 56, which handles incident-message
configuration. Static evidence establishes the table construction and lookup
data shape; it does not establish the contents of a particular level's stream
or the visible meaning of each resource token.

Evidence: `ev_b85e67b084f3e4f58104808358264bbf35d8a22b04e4917f254dcf99157f38da`
in [`legorr-objective-advisor-inline-markup.bundle`](../evidence/legorr-objective-advisor-inline-markup.bundle).

| Claim | State |
| --- | --- |
| Colon and dollar records populate distinct name/handle tables via separate loaders | observed (`FUN_00452e70`) |
| Ordinary records populate an indexed text array | observed (`FUN_00452e70`, `FUN_00453200`) |
| Exact runtime stream contents and token meanings | unknown; no level-specific stream was observed |

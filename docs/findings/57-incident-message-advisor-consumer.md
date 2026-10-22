# 57 — Queued incident messages feed the scrolling advisor text widget

Finding 47 documents the entity-state producer for shared ID `0x26` and the
type-selected IDs `0x0a`–`0x0c` (`FUN_0043bc40`, `ev_cb67dd44b4a1a803dee82f4abe7eb06a9d4e5aad251bbfab9e77bcd31969d456`). The hardcoded catalog places
`Info_LegoManDeath`, `Info_VehicleDeath`, and `Info_BuildingDeath` at those
three consecutive slots (`FUN_00419230`, `ev_f63affba6f717b3403a0ec7f529f5985850cddf6e753c2bba389a08b2c0ebab2`).
`FUN_00419b80` admits a message into the active queue after its precheck and
stores the selected ID in the queue record; IDs 10, 11, 12, and `0x26` bypass
the generic per-ID callback precheck
(`ev_24fe8bd23007105d0b67bd182c0b6ee7854ae1c1f69179b262926c163cece399`,
`ev_caa0f98d050fa72d87fbba5de2795eff2611f44364a1511776dedb831d39fcfd`).

`FUN_0041a080` is the downstream text-consumer path. When its state gates
allow a queued message and the display-update flag is set, it resets the
text widget, selects an entry-specific string from the queued record when
present or falls back to the configured string at
`DAT_004dc6f8 + message_id * 0x14`, appends that string through
`FUN_0045a590`, adjusts the scroll position, and clears the update flag.
`FUN_0045a590` delegates to `FUN_00485250`, which formats the supplied text
and appends its bytes to a widget ring buffer; reaching the buffer end wraps
the cursor and sets a widget flag. Evidence:
`ev_97f5ee0491bf31f413170c472613d04d79b8ef94dabcaa3a7b26bf48d5d78a29`,
`ev_e5ef82d21a28c48ab517a01d5a7a0171cd5d3f16366bb828feac61811e9c80ee`,
`ev_72d54379e492457a25f0e5d6f86da3b895476bc8a6a8f6dfaf49cf05c87fd48d`.

Together with finding 56, this connects the incident producer IDs to
configured display templates and the text widget. It is a static path
observation: the available caller inventory for `FUN_0041a080` is partial and
omits one incoming call reference, so this evidence does not establish the
precise runtime scheduling point or prove that every submitted message is
shown. [Caller coverage:
`ev_65b93699e20bdb18d6ae5857e861d34d0823f1540b6d6a43832871ef84dd188d`]
All cited records are retained in
[`legorr-incident-message-rendering.bundle`](../evidence/legorr-incident-message-rendering.bundle).

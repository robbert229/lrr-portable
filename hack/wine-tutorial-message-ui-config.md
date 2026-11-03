# Shipped tutorial message UI configuration

Static asset/source support for the isolated NEXT task, not runtime acceptance.
Source is the previously extracted LegoRR1.wad Lego.cfg, SHA256
`20cbf383bc672b0c330fe45e5fc509371de93d0b46958af82ac1b71f10fccb4f`,
under `/var/tmp/wine-tutorial-gate-audit/`. No live reference file or setting
was changed. The keys are under the authored `Lego* { Main { ... } }` block;
use actual root-qualified config traversal rather than the prior unqualified
lookup pattern that caused the Stats fault.

| Key | Authored value |
| --- | --- |
|NextButton640x480|Interface\MessagePanel\OKButton.bmp|
|NextButtonPos640x480|380,447|
|BackButton640x480|Interface\MessagePanel\repeat.bmp|
|BackButtonPos640x480|380,462|
|BackArrow|Interface\MessagePanel\BackButton.bmp|
|Panel_Messages (panel definitions)|Interface\MessagePanel\MsgPanel.bmp; comment says its listed positions are irrelevant here|
|Panel_MessagesSide|Interface\MessagePanel\MsgSidePanel.bmp; position0,409|

GameConfigInit's checked decompile at LEGORR.full.c23669–23714 formats display-
size-qualified Next/Back keys, loads the configured image, and reads its position
pair. The main interaction path at25761–25793 tests the real image dimensions
plus those stored positions. The NEXT path gates on debug-word bit0x40, supplies
feedback0x20, and calls0x453920; Back gates on bit0x80 and calls0x453910. Broader
admission and the exact meaning of its click/local booleans require the full
caller, so this slice does not independently establish DOWN/UP or physical
button policy. Coordinates are authored logical-frame data, not permission to
hardcode a screenshot hit rectangle.

The same config TextMessages::Text_Walk, Text_Drill, Text_Reinforce and
Text_PickupCrystal explicitly instruct a left mouse click for their actions.
Text_SelectionHelp says to click an object. This is stronger source support for
trying ordinary left-click at a permitted tutorial step, but is not observation
of configured runtime button/gesture behavior, right-button parity, drag policy,
or a substitute for the ordinary reference replay.

M01 source still disallows world actions in its introduction; follow real NEXT
and scripted arrivals before validating actor selection and a chosen destination.

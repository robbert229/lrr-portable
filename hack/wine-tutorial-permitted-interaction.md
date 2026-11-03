# Moving Rock Raiders: permitted interaction evidence

Source/asset inspection only; no game launch or runtime acceptance in this
investigation. The first red-X reference click was made during an instruction
that intentionally disallows world interaction. It cannot establish the select,
move, release, or cancel gesture policy.

I inspected the already-saved immutable reference backup, not the live reference
tree or prefix. The extractor used O_NOATIME and checked unchanged archive
atime/mtime/size before and after. Extractor, payloads, and provenance manifest:
`/var/tmp/wine-tutorial-gate-audit/`. No authored assets were changed or copied
into source directories.

Asset identities:

- LegoRR0.wad `Levels\TutorialLevels\MoveTuto_01\M01.nrn`: SHA256
  `3f3e5ef83b791ea2a8e39493c7ff0dfa4bbca8bf81317a834ae74c2733705910`.
- Corresponding compiled M01.npl: SHA256
  `ec59c67c043f20fd514921d221ee11e5420cb7f461f9b885b6d6541c725fdf6f`.
- LegoRR1.wad localized M01.txt: SHA256
  `230f825a10c9c2e2600451a00f26d4e1791f333a0077380c2d3b5346581b7473`.

The shipped .nrn is a source companion. Its correspondence with the .npl at
runtime has not been independently decoded/stepped in this investigation; the
candidate loader installs the configured binary .npl. Script-source conclusions
below remain separate from executed interpreter behavior.

## Source-supported progression

The script begins by checking GetObjectiveSwitch and GetObjectiveShowing;
it skips the body while the objective is showing. Intro sets DisallowAll and
asks for the flashing instruction arrow. Thus the objective/briefing ownership
repair is relevant to tutorial progression as well as reward/render/pause state.

R3 selects the major stage, R5 the local message progression. These are script
register names, not addresses to reuse across builds. Resolve any read-only
capture from the current exact PDB. Never write them to jump to a test stage.

| Stage in shipped script | Interaction and evidence |
| --- | --- |
| Intro, then TopMan/MidMan/BotMan | NEXT instructions introduce the three actors and different ground surfaces. Intro explicitly disallows world actions. |
| Watch/WaitForFast, Mid, SlowFinish | Camera locks and actor arrivals demonstrate scripted movement. Wait stages check GetRecordObjectAtTutorial and temporarily set game speed0 while waiting for instruction advancement. This is not user-commanded movement acceptance. |
| TellSecret, R3=10 | After its messages, enables ClickOnlyObjects and points at tutorial group4. |
| SelectRaider, R3=11, message16 | Requests an ordinary click on the visible Raider near the arrow. Stops until GetSelectedRecordObject is nonzero, then disallows other actions while acknowledging the selection. The following message describes a green selection frame. |
| ClickRubble, R3=12, message18 | Prior transition sets ClickOnlyMap and tutorial pointer8. Stops until the tutorial block-click count changes. This is the first explicitly permitted commanded destination, followed by a walking/shoveling instruction. |
| SelectRaider2 then ClickFloor, R3=13/14, messages20/21 | A second ordinary actor selection precedes a different ground destination. Arrival has its own subsequent stage. |

The localized text instructs two mouse clicks: one to select the Raider and one
to give the destination. It does not itself distinguish physical left/right
buttons or DOWN versus UP. Observe these details under Wine before claiming
reference gesture parity or changing the candidate policy.

## Next runtime comparison

Use normal menu selection and briefing acknowledgement. Follow visible flashing
NEXT arrows and wait for actual scripted arrivals until the instruction explicitly
asks for actor selection. Do not assume a fixed number of clicks or fixed screen
coordinates: camera changes and arrival gates are part of this sequence.

Record the relevant instruction, visible actor/arrow, exact physical button and
press/release, persistent green selection feedback, subsequent destination
instruction, and actual commanded displacement/arrival. Capture the second
selection and distinct floor destination as available. Distinguish scripted
movement, a block-click counter, accepted queue submission, and actual world
movement; none alone proves the complete entity milestone.

If the candidate cannot advance the real message arrow or scripted prerequisite,
record that first divergence and investigate its production owner. Do not clear
script gates, inject a selected actor, write registers, teleport, or substitute
a direct-level/debug flow. The current candidate has not reached this permitted
interaction stage in runtime evidence.

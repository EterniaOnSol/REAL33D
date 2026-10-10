# Presentation continuation after preservation commit - 2026-10-09

Task: UNREAL-WORLD-PRESENTATION-POLISH-001. State: IMPLEMENTED_UNVERIFIED.
Branch: milestone/unreal-world-presentation-polish-001.
Preservation commit: ce2e02db4c7c1cc4010db59169d9357532640301.
Certified base remains fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf.
Operator requested "keep going" after the commit instruction. No push/main merge.

## Water correction now proven live

Normal client (default renderer), live-20261009T140845, PID10844.
The operator explicitly confirmed the water now looks good. Manual screenshot
2026-10-09T20:11:28.952Z at32097,32205,8 shows continuous banks and water with
the former triangular wall fragments absent. Camera orbit changed to yaw284.50,
pitch-51.26, distance1320; unobstructed, one cutaway, upper floors hidden.
Protocol residual/unsupported/anomalies all0. Runtime geometry shows corrected
wall heights218. This closes the named floor8/9 water overlap BUG for ce2e02d;
it does not certify every floor/art case or later untested changes.

Main remained clean. Four recorded frozen-checkout evidence hashes match the
October7 recovery manifest; none was edited. Server status again verified the
existing querymanager830/game845/login1067 before launch; no duplicate startup.

## Operator-rejected raised cave walls

The same capture shows tall, vertically smeared cave pieces. Operator says the
raised pieces look horrible and asks for help improving them.

Source audit: catalog marks the affected357..384 cave-wall identities as
PROVISIONAL_SPRITE_RELIEF. Their actual source bounds are1x1 metre horizontally
and only0.045 metre high. The existing runtime's name-only wall-height rule
stretches these4.5uu reliefs to218uu (48.44x), causing giant columns with stretched
side texture. This is a runtime scale BUG compounded by incomplete volumetric
V08 art, not a reason to replace source assets.

Candidate correction in AssetRegistry::TryResolveExperimental: exclude a
PROVISIONAL_SPRITE_RELIEF of height<=10uu from wall-height normalization. Preserve
its authored proportions, original mesh/material and identity. Actual volumetric
walls still use the218uu room-height presentation policy. Twenty catalog entries
are affected; flat-wall-reliefs.json records IDs and source bounds only.
Both live tiles and WideWorld share this resolution. No map/authority changes.

Actual-catalog regression tests require seven cave variants to retain native
4.5uu thickness and full authored extents, while volumetric1294 stays218uu high.
The earlier lower-wall/water-plane tests remain. Pending build/live results are
recorded below when obtained; this candidate is not a visual PASS yet.

The expected visual compromise is explicit: these caves use low source reliefs
until proper volumetric V08 cave-wall art exists. No invented wall texture,
replacement mesh, source-art redesign, catalog rewrite or visual editor is used.
Other retained art issues remain in the recovery report.

## Remaining work

Compare the relief correction live at the same cave and obtain operator review.
Then finish camera clearance/return, wall occlusion, speech, Use With, Follow/Stop
and minimap-click/sector coexistence. WORLD_SPEECH/NPC_SPEECH remain
IMPLEMENTED_UNVERIFIED until a captured received-speech frame proves visibility.
No acceptance is inferred merely from source, compilation or aggregate counters.

Compact: unreal-world-presentation-polish-continuation-20261009/.
Raw: build/unreal-world-presentation-polish-001/recovery-20261009/ and live dirs.

## Executed correction checks

Unreal build PASS (exit0, four actions, Result Succeeded). All three final
automation cases PASS with errors0, including real relief proportions and
volumetric-wall height. ClientCore source is unchanged; the fresh nine-suite
PASS from this run remains valid. Live comparison of the relief correction is
still pending. The prior client was closed with a non-forced taskkill request
to its exact launched PID; normal EndPlay and engine exit were verified. That
session ended with36 accepted walks,7 refused,0 unanswered and protocol errors0.

## Subsequent session and checkpoint

Session live-20261009T142059 closed normally at 20:28:25 UTC. EndPlay records
881 commands, 16 requested/16 accepted walks, one rejected, zero unanswered,
six external relocations, eight Use requests, one attack and one item move.
Protocol residual/unsupported/anomalies are all zero; viewport synchronized,
duplicate/orphan events zero. WideWorld loads/unloads were 69/7, minimap and
local position agree at floor 7. These counters are bounded observations;
they do not prove the missing interaction/navigation scenarios.

No manual or speech frame was captured in that session. Consequently the relief
correction remains IMPLEMENTED_UNVERIFIED visually. Final staged secret_check
PASS is preserved alongside the EndPlay evidence. Source/build/automation
results above remain applicable; no production assets or gameplay were changed.

At 19:13 local the same three server services again passed identity/start/port
checks. Remote main is still fa3adcfd3fa85ec2bcf2514fc5df46c45e1de0cf and remote
polish remains 4305d23738454c2530007b16e759cbcaeddbca55. The operator requested
continued work; a fresh normal QA launch and cave/Say capture request are in
progress. No push, merge or milestone PASS is claimed.

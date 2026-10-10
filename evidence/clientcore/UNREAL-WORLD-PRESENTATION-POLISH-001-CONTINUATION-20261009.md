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

## Live visual review at 55e468b - 19:18 to 19:20 local

The operator provided two F9 captures and asked for their review, then closed
normally. Cave frame 01:18:37.442 UTC is at the same player position as the
before capture (32097,32205,8), with different camera distance/yaw. It shows:
- Continuous water/banks without the former triangular overlaps.
- The stretched dirt columns are gone. Live bounds confirm 379/383/384 are
  4.5uu high with full native horizontal size; 373/377 remain 218uu high.
- Player/rat labels and health are readable; upper floors are hidden and one
  cutaway is active. Blackness beyond the received underground scene remains.
- The mixture of thin cave reliefs and tall stone walls still looks visually
  inconsistent. This is not an overall cave-art approval.

Return frame 01:18:44.026 UTC shows floor 7 restored, player visible, upper
floors restored, no cutaway, camera distance equals requested 1653.50uu. The
minimap follows both floor states. A real Look response is retained in chat.
This proves the observed floor round trip and current water/relief correction,
not the complete camera collision or minimap-click navigation matrix.

Normal EndPlay at 01:20:40.019 UTC, engine exit at 01:20:44.966 UTC: 426 commands,
22 requested/accepted walks, zero rejected/unanswered; two Uses, one Look, zero
Say/Follow/attack. Protocol residual/unsupported/anomalies all zero; synchronized
viewport, no duplicate/orphan events. WideWorld loads/unloads 87/31. No speech
capture exists; world/NPC speech remain IMPLEMENTED_UNVERIFIED.

### Remaining wall classification

BUG (fixed and live observed): 4.5uu provisional cave reliefs stretched almost
49x into columns. Native proportions now preserved in the same imported V08 meshes.
BUG (fixed and live observed): lower-floor normalized wall caps overlapping water.

V08_ART_DEFECT: inconsistent source construction within the cave wall family.
The remaining raised walls are IDs 373 and 377, FAMILY_RECIPE, with source height
85.907936uu and original straight/corner footprints matching live imported bounds.
Source GLB SHA256s match the manifest. Their source nodes explicitly describe a
semantic family recipe, pilot pending review, with interpreted hidden surfaces.
Read-only binary inspection finds all 4592/8080 triangles respectively use a
constant UV coordinate per triangle. Thus the faceted color layout is already
present in the source; it is not evidence of a runtime material replacement.
Catalog assembly is unresolved and art approval is NO. Existing room-height
normalization scales Z by 2.5376, which accentuates the source facets. It is not
an unexplained import-unit mismatch. No orientation error is established here.

The bounded runtime repair does not supply missing volumetric cave continuity.
Changing these assets or their authored construction needs a separate V08 art
workstream. No source asset was modified, substituted, flattened or redesigned
in response to these two captures. Source audit and compact live bounds summary
are preserved; no raw geometry or map dump is committed.

### Current acceptance delta

FLOOR_READABILITY = PASS for the observed water/relief/floor-return scene.
The new evidence replaces the pending live status of that bounded correction.
ROOF_HANDLING and desktop WIDEWORLD_COMPATIBILITY remain PASS for exercised cases.
The full milestone remains IMPLEMENTED_UNVERIFIED: camera collision/occlusion,
complete Use/Use With feedback, fresh Follow/Stop, visible local/NPC speech,
map-click arrival and the full gameplay regression matrix are still incomplete.
Art completeness is not being used to block runtime acceptance.

Client is closed normally. Next work remains this same milestone's missing live
matrix. After PASS, exact next milestone is
UNREAL-WORLD-PRESENTATION-POLISH-001-INDEPENDENT-CERTIFICATION.

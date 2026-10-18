# Live observations — independent skills certification

Fresh session live-20261017T051912, candidate63a62dd plus the retained read-only
SkillsCert diagnostic. Existing Fusion32 service and accountB; operator controls
all input. No injected state/commands or modifications to server rules/data.

- 11:19:32.487Z: actual Slate text before receipt is -- for all14 rows; all9bars0.
- 11:19:49.518Z: server event copies arrive, before the next HUD refresh. Observer
  records new state plus previous frame's unknown widgets; this is a transition,
  not a fabricated value. At11:19:50.925Z, actual widgets match received data.
- 11:20:13.158Z operator F9: all7named skills visible, no clipping at1280x720.
  Level4/17%, XP471, HP64, Mana15, Soul100, Capacity285, Magic0/0%; Fist11/32%,
  Club13/9%, Sword11/67%, Axe10/0%, Distance10/0%, Shielding10/0%, Fishing10/0%.
  Actual 100px bar paint widths:17,0,32,9,67,0,0,0,0. Matches packet event copies.
  Minimap player marker/world position32097,32204,7 agree; inventory equipment,
  vitals panel, Dixi battle entry and local chat visible. Protocol counters0.
- Operator opened backpack normally at11:20:18.721Z (Use type2854, body slot3).

Continuation inspected at closeout (the client had already exited normally):

- 12:06:38.870Z: ordinary world right-click attack, creature1073743164. Sword
  progresses67->68->69->70->71%, and actual painted widths match. Experience
  changes471->476 after combat. A second world attack follows at12:07:07.510Z.
- F9 at12:07:16.152Z shows all seven skills and the longer Sword bar, XP481,
  HP54, Mana15, Magic0; rats in the world and Battle List; open dead-rat and
  backpack containers, stack counts gold26/worms14, shovel and rope; inventory,
  minimap on floor8 and server combat messages in local chat. No clipped skill
  label or missing row is visible. Compared with the earlier ground-floor F9,
  the HUD retains its columns, panels and controls during gameplay.
- Three ordinary world right-click attacks total. By12:11:16.136Z experience486
  and Sword11/83%; actual paint83%. Skill level remains11, correctly. No skill
  level increment is claimed. At12:11:34.415Z final observed values still match.
- EndPlay12:11:39.155Z:3128.8seconds,3073frames,4295commands; residual bytes,
  unsupported opcodes and anomalies all0; normal LogExit at12:11:48.908Z.
  Eight retained protocol snapshots all have the same three zero counters.

`verify_live.py` replays50 observer records:46 fully matching observations and
four event/HUD pre-refresh transitions. All17 received skill vectors have a
matching actual-text/paint observation, including the final83%. Unknown states
show dashes; server-sent Magic0 shows0. No local progression is computed.

This closeout reviewed the retained independent session and its screenshots;
it did not launch another gameplay session or claim the candidate author's
different04:46 session as independent evidence. Live button toggles are not
claimed: the six real HUD panels' registry operations were independently
exercised by CertificationSkillsWidgets, repeated at23:53:30Z with6/6 tests
Success/errors0. The temporary observer is preserved as source evidence and
retired from production Source for the final exact-candidate build.

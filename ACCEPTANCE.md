# FreeTTS - in-game acceptance checklist

**0.4.0 (2026-09-04): voice picker, needs a run.** DLL change (voice
enumeration, `SetVoice`) plus a Voice settings group. Import
`dist/FreeTTS-0.4.0.zip`, confirm the deployed DLL hash matches the staged
one. **Steps 36-39 are new**; steps 5, 17, and 28 are the regression set
for this build.

**0.3.0 result (2026-09-04): map pins and districts spoke on the first
run** once the right build was deployed (the first attempt ran with 0.2.0
still deployed, which is why the metro menu talked and the map did not).
Log observations, not defects: district changes arrive as two events on
the same frame with one stale half ("City Center, Little China" then
"Watson, Little China"); the second purges the first, so the right one
is heard. The grouped fast travel pin reads its whole description
sentence.

**0.2.0 result (2026-09-04): every step passed** on the first in-game run,
covering quickhacks, RAM readout, dialogue choices, and the settings page.

Where a step asks for a log line, the line is the result; what you heard is
extra.

The log is `red4ext/logs/freetts-<date>.log` in the game folder. Lines from
this mod carry `[FreeTTS]`. A spoken phrase logs as
`speaking (rate <n>, voice <v>): <phrase>`; `<n>` is the rate slider for
that list and `<v>` the Voice setting (0 unless changed). The step quotes
below leave `, voice <v>` out for brevity.

## Before launching

1. Windows volume up, game audio does not matter. Speech goes to the
   default output device.
2. Deployed `red4ext/plugins/FreeTTS/FreeTTS.dll` hash equals
   `dist/FreeTTS/red4ext/plugins/FreeTTS/FreeTTS.dll`.

## Load

3. Log has `FreeTTS loaded`, then `SAPI voice ready`. No `HRESULT` line
   between them.
   - Missing `FreeTTS loaded`: runtime mismatch or wrong depth; nothing else
     below can pass.
   - `HRESULT` line: voice creation failed; nothing will be spoken. Stop
     here and report the line.
4. `r6/logs/redscript_r*.log` from this launch has no error naming
   `FreeTTS.reds`.

## Quickhack list, combat

5. Open the quickhack list on an enemy. The log gains one
   `speaking (rate 0): RAM <current> of <max>, <name>, <n> RAM` line and
   that whole phrase is spoken: your RAM first, then the first row. The
   two numbers match the panel's own RAM readout; if the current value
   is off by exactly one, the spoken number is the truer one (the panel
   rounds a percentage), note it, not a defect.
   - Only the row, no RAM prefix, or `RAM ...` on its own line first: a
     defect in the open-order handling. Record which shape you saw.
6. Move the highlight down one row. One new `speaking:` line, the new row
   spoken, the old one cut off if it was still talking.
7. Move up and down quickly through four or five rows. One `speaking:` line
   per row, and the voice keeps up: it says the current row, not a backlog.
8. Stay on a row you can already afford while RAM regenerates for a few
   seconds. **No** new `speaking:` line for that row. (This is the dedup; a
   repeat here is a defect. A row that was locked for RAM and becomes
   affordable is a different phrase and is meant to re-speak.)
9. Highlight a hack you cannot afford. Spoken as
   `<name>, <n> RAM, locked, <reason>`; the log line matches.
10. Close the list, reopen it on the same enemy. RAM and the first row
    are spoken again (one new `speaking` line), even though it is the
    same hack as before. If you spent RAM in between, the number is the
    new one.
11. Switch to a different enemy with the list open. The first row for the
    new target is spoken. Open question for this run: if the new target's
    first row is word-for-word the same phrase and the panel never hid in
    between, it may stay silent. Note whether that happened; it is a
    design choice to make, not a defect.

## Quickhack list, out of combat

12. Scan a camera or door and open its hack list. Rows are spoken the same
    way. This is expected: the same UI controller serves both.
13. Open the list on something with no hacks available. The "no
    quickhacks" row is spoken by its title alone, with no RAM cost.

## Settings page

14. Settings > Mods lists FreeTTS with two groups: Quickhacks (Speak
    quickhacks, Quickhack speech rate) and Dialogue (Speak dialogue
    choices, Dialogue speech rate). If Mod Configuration Menu is open
    instead, the same four entries show there.
    - No FreeTTS entry: the settings system did not attach or Mod
      Settings is missing. `redscript_r*.log` is the place to look.
15. Turn "Speak quickhacks" off, apply, open the quickhack list and move
    through it. Silence, and **no** `speaking` line. Turn it back on:
    the next highlight change speaks.
16. Set "Quickhack speech rate" to 8, apply, cycle the list. The log line
    says `speaking (rate 8):` and the voice is clearly faster. Set it back
    to 0 afterwards, or leave it where you like it.
    - The rate is greyed out while its switch is off; that is the
      dependency working, not a defect.

## Dialogue choices

17. Start a conversation with two or more choices. The log gains one
    `speaking (rate <n>): <choice>` line when the list appears and the
    highlighted choice is spoken. A choice shown as `[Leave]` is spoken
    as "Leave"; a tagged line like `[Corpo] I know the drill` is spoken
    "Corpo, I know the drill".
18. Move the highlight to another choice. One new line, the new choice
    spoken, the previous cut off if still talking.
19. Move back to the first choice. It is spoken again (a different key,
    so no dedup).
20. An option you cannot pick (a failed attribute check, or one the game
    marks inactive) is spoken with ", unavailable" on the end. A line you
    have already heard is dimmed but still selectable, so it is spoken
    plainly.
21. A timed choice (progress bar under the list) is spoken the moment it
    appears. Judge whether the Dialogue rate needs to be higher than the
    quickhack one; that is what the second slider is for.
22. A single-option prompt (just `[Leave]` or `[Continue]`, nothing to
    pick between). Open question for this run: the game runs these with
    no active hub, and the mod treats a lone hub as the target. Record
    whether it spoke. Silence here is a design gap to fix, not a crash.
23. Pick a choice. The list closes; nothing more is spoken for it. Open
    the next choice list in the same conversation: its highlighted choice
    is spoken, even if the text matches the last one (different hub id).
    Two `speaking` lines back to back here, the first cut off at once, is
    the hub and the index arriving on separate frames; not a defect
    unless the wrong one is what you end up hearing.
24. Turn "Speak dialogue choices" off, apply, open a choice list. Silence
    and no `speaking` line; quickhacks still speak if their switch is on.

## World map

27. Open the map. Settings > Mods > FreeTTS now has a third group, Map
    (Speak map, Map speech rate).
28. Hover the mouse over a fast travel pin. One `speaking (rate <n>):
    <name>, Fast travel` line (or the point's name alone if the tooltip
    shows no second line) and the pin is spoken.
29. Move to a different pin: a job, a shop, a vehicle for sale. Each is
    spoken as its tooltip title, with the description line after a comma
    when the tooltip has one. A pin whose tooltip title is a raw key like
    `UI-MappinTypes-Gig` instead of words is a defect; report the text.
30. Hover off a pin and back onto the same one. It is spoken again (the
    hide clears the dedup).
31. With a gamepad, move the map cursor across pins. Same behaviour as
    hover: one line and one utterance per pin.
32. Zoom out until district names show, then drag the cursor across the
    city. Each district change is spoken as `<district>, <subdistrict>`
    ("Watson, Kabuki"); staying inside one subdistrict speaks nothing
    more. Dogtown is spoken as Dogtown. Off the edge of the city, nothing.
33. Turn "Speak map" off, apply, hover a pin and cross a district. Silence
    and no `speaking` line; quickhacks and dialogue still speak.

## Voice

36. The log, just before `SAPI voice ready`, has one `voice n: <name>`
    line per installed voice, in order from 1. The Windows default is
    among them.
37. Settings > Mods > FreeTTS has a fourth group, Voice, with one
    number entry (0 to 20). Set it to a number that is in the log's
    list and not the default, apply, open the quickhack list. The log
    line is `speaking (rate 0, voice n):` (full form here) followed by `voice set: <name>`,
    and the row is spoken in that voice. Every later phrase, dialogue and
    map included, uses it.
38. Set it back to 0, apply, speak something. `voice set:` names the
    default again and the voice reverts.
39. Set it to a number past the end of the list (20), apply, speak
    something. One `voice 20 is not installed (n available)` line, the
    default voice speaks, and no further complaint on later phrases.

## Stability

34. Play for a while with the mod on. No crash, no hitch when the list
    opens. If the game crashes, run
    `python ..\cp2077-tooling\analyze-crash.py` and look for
    `FreeTTS.dll` frames; the symbols are in `dist/symbols/`.
35. Quit to desktop cleanly. The game process exits; it does not hang on
    a worker thread.

## Result

**2026-09-04, 0.2.0: reported working across the board** (quickhacks, RAM
readout on open, dialogue choices, settings page). Note from the run: the
stock Windows voice copes less well with dialogue wording than with
quickhack names; a voice or rate change may help, the list itself is right.
Next feature requested: the world map (highlighted mappins, and districts
when zoomed out).

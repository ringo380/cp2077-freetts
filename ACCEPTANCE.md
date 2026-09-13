# FreeTTS - in-game acceptance checklist

**0.5.0 result (2026-09-05): passed.** The 16:20 log shows the preview
`speaking (rate 0, voice 4): Voice 4, Microsoft David Desktop` followed by
`voice set: Microsoft David Desktop ...`, then the same pair for Voice 3
(Ava), so the field is written before the change callback fires. Voice 3
was still in use after the 17:43 relaunch. No `not installed` line was
logged, so step 39's past-the-end slot was not exercised this run.
Steps 40-42 were new; 5, 17, 28 and 37 were the regression set.

**0.4.0 result (2026-09-05): the voice list and `SetVoice` work.** The
13:09 log listed nine voices and the setting was in use at 8 (Jenny).

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
that list and `<v>` the voice in use (0 unless changed; from 0.6.0 the
position the remembered name resolved to at launch, which can differ from
the menu's number). The step quotes below leave `, voice <v>` out for
brevity.

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
    (0.5.0: the setting is a list; use Voice 12 for this step and expect
    "Voice 12 is not installed" spoken as the preview.)
40. 0.5.0: the Voice entry is a list, Windows default then Voice 1 to
    Voice 12, not a number. On first run after upgrading it reads Windows
    default (the old number is not carried over).
41. Pick Voice 3, apply. Straight away the mod says "Voice 3, <name>" in
    that voice, where <name> is the log's `voice 3:` name without the
    language tail; the log has `speaking (rate 0, voice 3): Voice 3, ...`
    then `voice set: <name>`. Apply again without changing it: silence.
    If the preview names the voice you had before instead, Mod Settings
    fired the change callback before writing the field; report it.
42. Pick Windows default, apply: "Windows default, <name>" in the default
    voice.

### Remembered by name (0.6.0)

Before 0.6.0 the setting was a bare position, so any change to the
Windows voice list (a voice added, removed, or made the default) silently
rebound it to a different voice; on 2026-09-13 a saved Voice 3 went from
Ava to Mark that way. Now the plugin keeps the chosen voice's logged name
in `%LOCALAPPDATA%\FreeTTS\voice.txt` and every list asks the plugin,
not the setting, which voice to use.

43. First launch with no `voice.txt`: the log, just before `SAPI voice
    ready`, says `saved voice: none, using the Windows default`. The
    menu still shows whatever number it had; the default voice speaks.
44. Pick a numbered voice, apply. The log has `voice choice saved:
    <name> (voice n)` before the preview's `speaking` line, and
    `voice.txt` now holds that name on one line. Speak something: it uses
    that voice. Relaunch: the log says `saved voice: <name> (voice n)`
    and the first phrase spoken is in that voice, with no apply needed.
45. The renumbering case, simulated. Quit. Edit `voice.txt` to the exact
    logged name of a voice at a different position (say the `voice 1:`
    name). Relaunch: the log resolves it, `saved voice: <name> (voice 1)`,
    and the game speaks in that voice, even though the menu still shows
    the old number. Choosing that stale number again does nothing (no
    change); stepping to another entry and back applies and re-saves.
46. Pick Windows default, apply: `voice choice cleared, using the Windows
    default`, `voice.txt` is gone, the default speaks. Relaunch: `saved
    voice: none`.
47. Missing voice. Quit, put a made-up name in `voice.txt`, relaunch: one
    `saved voice "<made-up>" is not installed, using the Windows default`
    line, the default speaks, nothing else complains. Pick a real voice to
    overwrite the file.
48. Stale menu, unrelated apply. In the step 45 state (voice.txt names a
    voice at a position other than the menu's number), change a rate
    slider only and apply. Expect no `voice choice saved` line, `voice.txt`
    unchanged, and the next phrase still in the remembered voice. If a
    `voice choice saved` line for the menu's number does appear, the
    change callback compared against a baseline taken before Mod Settings
    wrote the stored value; report it, the fix is to key the callback on
    `OnModVariableChangeAccepted` with `varName == n"voice"` instead.

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

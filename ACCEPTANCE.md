# FreeTTS - in-game acceptance checklist

**0.2.0 (2026-09-04): dialogue choices and a settings page, needs a run.**
0.1.0 (quickhacks only) was never run in-game, so this run covers both.
Import `dist/FreeTTS-0.2.0.zip`, confirm the deployed DLL hash matches the
staged one, then work down the list. Where a step asks for a log line, the
line is the result; what you heard is extra.

The log is `red4ext/logs/freetts-<date>.log` in the game folder. Lines from
this mod carry `[FreeTTS]`. A spoken phrase logs as
`speaking (rate <n>): <phrase>`; `<n>` is the rate slider for that list.

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
   two numbers match the panel's own RAM readout.
   - Only the row, no RAM prefix: the panel was already visible when the
     row was selected (a target switch, see 11) or the RAM readout ran
     before the panel showed. Note which.
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

## Stability

25. Play for a while with the mod on. No crash, no hitch when the list
    opens. If the game crashes, run
    `python ..\cp2077-tooling\analyze-crash.py` and look for
    `FreeTTS.dll` frames; the symbols are in `dist/symbols/`.
26. Quit to desktop cleanly. The game process exits; it does not hang on
    a worker thread.

## Result

(Record the date, build, and which steps passed or failed here.)

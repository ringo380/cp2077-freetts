# FreeTTS - in-game acceptance checklist

**0.1.0 (2026-09-02): first build, needs a run.** Proof of concept: the
quickhack list only. Import `dist/FreeTTS-0.1.0.zip`, confirm the deployed
DLL hash matches the staged one, then work down the list. Where a step asks
for a log line, the line is the result; what you heard is extra.

The log is `red4ext/logs/freetts-<date>.log` in the game folder. Lines from
this mod carry `[FreeTTS]`.

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
   `speaking: <name>, <n> RAM` line and the first row is spoken.
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
10. Close the list, reopen it on the same enemy. The first row is spoken
    again (one new `speaking:` line), even though it is the same hack as
    before.
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

## Stability

14. Play for a while with the mod on. No crash, no hitch when the list
    opens. If the game crashes, run
    `python ..\cp2077-tooling\analyze-crash.py` and look for
    `FreeTTS.dll` frames; the symbols are in `dist/symbols/`.
15. Quit to desktop cleanly. The game process exits; it does not hang on
    a worker thread.

## Result

(Record the date, build, and which steps passed or failed here.)

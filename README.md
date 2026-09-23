# FreeTTS

An accessibility mod for Cyberpunk 2077. It speaks the currently highlighted
quickhack, dialogue choice, or map pin aloud, through the Windows
text-to-speech voice, as you move through the list or across the map. No
more squinting at the quickhack wheel mid-fight, at a timed dialogue
prompt, or at a tiny map label.

## Why this exists

The quickhack list appears briefly, in small text, while the game is slowed
but not paused. A player who relies on screen magnification cannot read it at
combat pace, and resizing the UI is not practical. Hearing the highlighted
item removes the need to read it at all.

## What it does

- When the quickhack list is open (combat, scanner, or a device), every time
  the highlight moves, the mod speaks the new row: its name, its RAM cost,
  and whether it is locked and why. For example: "Overheat, 6 RAM" or
  "Short Circuit, 8 RAM, locked, not enough RAM".
- Cycling quickly cuts the previous item off; you always hear the one that
  is highlighted now.
- Opening the list announces your RAM and then the first row, as one
  phrase: "RAM 8 of 12, Overheat, 6 RAM". Changes the game makes on its
  own (RAM regenerating, a cooldown ending) do not repeat the row you are
  already on.
- When a dialogue choice list is open, every time the highlight moves, the
  mod speaks the new choice the way it is shown: the tag first if it has
  one, then the line, and "unavailable" for a greyed-out option. For
  example: "Leave" or "Corpo, I know how these deals work" or "Body, force
  the door, unavailable". Opening the list announces the highlighted
  choice.
- On the world map, whenever a pin is highlighted (mouse hover or the
  gamepad cursor), the mod speaks what its tooltip shows: the title and,
  when there is one, the description line. For example: "Kabuki Market,
  Fast travel" or "The Heist, Main job" or "Undiscovered". Zoomed out far
  enough for district names to appear, moving across the map speaks the
  district and subdistrict under the cursor: "Watson, Kabuki".
- Quickhacks, dialogue, and the map each have their own on/off switch and
  speech rate, so timed dialogue can be read faster than the quickhack
  list. See Settings below.
- It uses the voice Windows already has, or your screen reader: when NVDA
  is running, the text goes to NVDA instead (your NVDA voice and speed,
  and your braille display). Nothing is downloaded, nothing leaves your
  machine.

Phone and text-message choices, the radio, the weapon wheel, and the map's
filter bar and tracked-quest panel are not spoken yet.

## Requirements

- Cyberpunk 2077 2.x. Tested on **patch 2.31** (game file version
  `3.0.80.51928`). The plugin is not tied to one game version, so it keeps
  loading after a game update; if an update ever changes something it
  relies on, remove the mod until a fixed version is out.
- **RED4ext** for your game version.
- **redscript** (the script compiler that runs at game launch).
- Windows 10 or 11 with at least one text-to-speech voice installed. Every
  stock install has Microsoft David and Microsoft Zira. Linux and Steam
  Deck (Proton) are untested; Windows speech may be missing there, in
  which case the log says so and nothing is spoken.
- Optional: **NVDA**, if you use a screen reader. See Screen readers.
- Optional: **Mod Settings** (the `mod_settings` RED4ext plugin) for the
  in-game settings page. Without it everything is on and every rate is 0.

Cyber Engine Tweaks is not required.

## Install

FreeTTS has two parts, a native RED4ext plugin and a redscript file. Copy
them into your game folder at these paths:

```
red4ext/plugins/FreeTTS/FreeTTS.dll
r6/scripts/FreeTTS/FreeTTS.reds
r6/scripts/FreeTTS/FreeTTSMap.reds
r6/scripts/FreeTTS/FreeTTSSettings.reds
r6/scripts/FreeTTS/FreeTTSSpatial.reds
r6/scripts/FreeTTS/FreeTTSText.reds
red4ext/plugins/FreeTTS/nvdaControllerClient.dll
red4ext/plugins/FreeTTS/nvdaControllerClient-LICENSE.txt
```

`nvdaControllerClient.dll` is NV Access's own library for talking to NVDA,
unchanged, under the GNU LGPL 2.1 (the license file beside it). Without it
FreeTTS still works, just never through NVDA.

With Vortex, import the release zip; it already has this layout. Vortex
reads no name or version from a local archive, so type these in the mod
details pane by hand:

- Name: FreeTTS
- Version: 1.0.0
- Author: ringo

## Settings

With Mod Settings installed, FreeTTS appears under Settings > Mods, and in
Mod Configuration Menu if you use that. Four groups:

- **Quickhacks**: Speak quickhacks (on/off), Quickhack speech rate.
- **Dialogue**: Speak dialogue choices (on/off), Dialogue speech rate.
- **Map**: Speak map (on/off), Map speech rate.
- **Voice**: Voice, a list: Windows default, Voice 1, Voice 2, ... up to
  Voice 12. Voice n is the nth voice in the plugin's startup log, see
  Voices below. When you apply a change the mod says the chosen voice's
  name in that voice, so you can pick by ear, and remembers the voice by
  its name, so it stays the same voice even when Windows renumbers the
  list. The name is always said in the Windows voice, even with NVDA
  running, since that is the voice being picked. Output: Automatic (NVDA
  when it is running, otherwise the Windows voice) or Windows voice (never
  NVDA).

FreeTTS follows the game's Text Language setting. With Voice at Windows
default it speaks with a voice for that language when Windows has one
installed (the Windows default voice itself when it qualifies), and the
few words it adds ("locked", "RAM 8 of 12", "unavailable") are in that
language too. The game's own text is already translated by the game. The
non-English wording of those added words is best-effort; corrections are
welcome, and they all live in `r6/scripts/FreeTTS/FreeTTSText.reds`. The
settings page labels are English.

A rate runs from -10 (slowest) to 10 (fastest); 0 is the voice's own speed.
Rates and Voice apply to the Windows voice only; NVDA uses its own.
The rate for a group is greyed out while that group is off. Changes apply
the next time something is spoken; no restart.

## Screen readers

With Output at Automatic (the default, and what you get without Mod
Settings), FreeTTS checks for NVDA before every phrase. When NVDA is
running the phrase goes to NVDA, interrupting whatever NVDA was saying, and
to your braille display if one is connected; the Windows voice stays
silent, so the two never talk over each other. Start or quit NVDA at any
time; the next phrase follows. The log says `NVDA is running, speaking
through it` or `NVDA is not running, speaking through the Windows voice`
when that changes.

Only NVDA is supported. JAWS, Narrator and other screen readers get the
Windows voice.

## Voices

With Voice at Windows default the mod speaks with the Windows default
text-to-speech voice, which is set in the old Speech control panel (run
`sapi.cpl`, or Control Panel > Speech Recognition > Text to Speech), or,
when the game's text language differs from that voice's, with the first
installed voice for the game's language. To use a different voice in the
game without changing the Windows default, pick Voice n, where n is that
voice's number in the plugin's startup list. Apply and the mod says
"Voice n, <name>" in that voice; step through the list until you hear the
one you want. The plugin lists every voice it can see at startup in
`red4ext/logs/freetts-<date>.log`, for example:

```
voice 1: Microsoft Zira Desktop - English (United States)
voice 2: Microsoft David Desktop - English (United States)
voice 3: Microsoft Mark - English (United States)
```

The number is that voice's position in the list; the log's order is the
one that counts, and it can differ from what other programs show. A slot
past the end of the list says "Voice n is not installed" and falls back
to the Windows default.

The number is only how you pick. What the mod keeps is the voice's name,
in `%LOCALAPPDATA%\FreeTTS\voice.txt`, and at every launch it finds that
name in the fresh list: the log says `saved voice: <name> (voice n)`.
Windows renumbers the list whenever a voice is added, removed, or made the
default, so the voice you picked stays and only the number shown in the
menu can go stale. When that happens, choosing the shown entry again does
nothing (no change to apply): step to another entry and back. To forget
the choice, pick Windows default, or delete the file.

A stock English Windows shows only the two "Desktop" voices, David and
Zira; other Windows display languages come with a voice for their own
language. Ways to get more:

- **Voices Windows already has but hides.** Windows 10 and 11 ship more
  voices for the newer speech stack (Microsoft Mark, and the voices
  added when you install a language's speech pack under Settings > Time
  & language > Speech), hidden from the older one the mod uses. Copying a
  voice's registry entry across makes it visible. In an administrator
  PowerShell, for Mark:

  ```
  reg copy HKLM\SOFTWARE\Microsoft\Speech_OneCore\Voices\Tokens\MSTTS_V110_enUS_MarkM HKLM\SOFTWARE\Microsoft\Speech\Voices\Tokens\MSTTS_V110_enUS_MarkM /s /f
  ```

  To undo it: `reg delete HKLM\SOFTWARE\Microsoft\Speech\Voices\Tokens\MSTTS_V110_enUS_MarkM /f`.
  The same works for any voice listed under `Speech_OneCore\Voices\Tokens`.
- **Natural voices (much better).** Windows 11's Narrator has neural
  voices (Aria, Guy, Jenny, Sonia, and more). A free bridge,
  [NaturalVoiceSAPIAdapter](https://github.com/gexgd0419/NaturalVoiceSAPIAdapter),
  registers them as ordinary Windows voices, and the mod then lists them
  like any other. Install the x64 build (the one the game needs) and
  follow its own documentation; its wiki explains which Narrator voice
  packages it can open.

Quit and relaunch the game after adding a voice; the list is read once at
startup.

## Troubleshooting

Open `red4ext/logs/freetts-<date>.log` in the game folder.

- No `freetts-<date>.log` file for this launch at all: the plugin was not
  loaded. Check that RED4ext itself loads (its own `red4ext-<date>.log`),
  and that the DLL is at exactly the path above (one folder deep under
  `red4ext/plugins`).
- `FreeTTS loaded` but no `SAPI voice ready`: Windows could not create a
  voice. The line after it names the failing call. Confirm a voice exists
  under Settings > Time & Language > Speech.
- `speaking (rate n, voice v): ...` lines appear but you hear nothing: the voice is
  working and the game is calling it. Check the Windows volume mixer; the
  speech plays through the default output device, not the game's.
- Neither `speaking` nor any warning when you cycle the list: the script
  did not compile. Look in `r6/logs/redscript_r*.log`.
- The voice is wrong or ignored: the log's `voice n:` lines are the list
  the number refers to, and a `voice n is not installed` line means the
  number is past the end of it. The `saved voice:` line just before
  `SAPI voice ready` names the voice actually in use; if it differs from
  the menu's number, Windows has renumbered the list (see Voices) and the
  voice heard is still the one you chose.
- One list is silent and the other is not: check its switch under
  Settings > Mods > FreeTTS.
- No FreeTTS entry under Settings > Mods: Mod Settings is not installed.
  The mod still works with its defaults.

## Building from source

`red4ext/plugins/FreeTTS/` is a CMake project that fetches the RED4ext SDK
at a pinned commit. From that folder:

```
cmake -B build
cmake --build build --config Release
```

Needs Visual Studio 2022 Build Tools (MSVC x64) and a Windows SDK. The
Release build also emits `FreeTTS.pdb` and `FreeTTS.map` next to the DLL;
keep them if you ever need to read a crash dump.

## License

MIT, see `LICENSE`.

# FreeTTS

An accessibility mod for Cyberpunk 2077. It speaks the currently highlighted
quickhack or dialogue choice aloud, through the Windows text-to-speech
voice, as you move through the list. No more squinting at the quickhack
wheel mid-fight, or at a timed dialogue prompt.

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
- Quickhacks and dialogue each have their own on/off switch and speech
  rate, so timed dialogue can be read faster than the quickhack list. See
  Settings below.
- It uses the voice Windows already has. Nothing is downloaded, nothing
  leaves your machine.

Phone and text-message choices, the radio, and the weapon wheel are not
spoken yet.

## Requirements

- Cyberpunk 2077 **patch 2.31** (game file version `3.0.80.51928`). The
  plugin declares this exact runtime to RED4ext. On any other game version
  RED4ext silently refuses to load it: nothing crashes, the mod just does
  nothing.
- **RED4ext** matching that game version.
- **redscript** (the script compiler that runs at game launch).
- Windows 10 or 11 with at least one text-to-speech voice installed. Every
  stock install has Microsoft David and Microsoft Zira.
- Optional: **Mod Settings** (the `mod_settings` RED4ext plugin) for the
  in-game settings page. Without it everything is on and both rates are 0.

Cyber Engine Tweaks is not required.

## Install

FreeTTS has two parts, a native RED4ext plugin and a redscript file. Copy
them into your game folder at these paths:

```
red4ext/plugins/FreeTTS/FreeTTS.dll
r6/scripts/FreeTTS/FreeTTS.reds
r6/scripts/FreeTTS/FreeTTSSettings.reds
```

With Vortex, import the release zip; it already has this layout. Vortex
reads no name or version from a local archive, so type these in the mod
details pane by hand:

- Name: FreeTTS
- Version: 0.2.0
- Author: ringo

## Settings

With Mod Settings installed, FreeTTS appears under Settings > Mods, and in
Mod Configuration Menu if you use that. Two groups, two entries each:

- **Quickhacks**: Speak quickhacks (on/off), Quickhack speech rate.
- **Dialogue**: Speak dialogue choices (on/off), Dialogue speech rate.

A rate runs from -10 (slowest) to 10 (fastest); 0 is the voice's own speed.
The rate for a group is greyed out while that group is off. Changes apply
the next time something is spoken; no restart.

Which voice speaks is whatever Windows is set to. Change it under
Settings > Time & Language > Speech.

## Troubleshooting

Open `red4ext/logs/freetts-<date>.log` in the game folder.

- No `freetts-<date>.log` file for this launch at all: the plugin was not
  loaded. Check the game version against Requirements, and that the DLL is
  at exactly the path above (one folder deep under `red4ext/plugins`).
- `FreeTTS loaded` but no `SAPI voice ready`: Windows could not create a
  voice. The line after it names the failing call. Confirm a voice exists
  under Settings > Time & Language > Speech.
- `speaking (rate n): ...` lines appear but you hear nothing: the voice is
  working and the game is calling it. Check the Windows volume mixer; the
  speech plays through the default output device, not the game's.
- Neither `speaking` nor any warning when you cycle the list: the script
  did not compile. Look in `r6/logs/redscript_r*.log`.
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

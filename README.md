# FreeTTS

An accessibility mod for Cyberpunk 2077. It speaks the currently highlighted
quickhack aloud, through the Windows text-to-speech voice, as you cycle
through the list. No more squinting at the quickhack wheel mid-fight.

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
- Opening the list announces the first row. Changes the game makes on its
  own (RAM regenerating, a cooldown ending) do not repeat the row you are
  already on.
- It uses the voice Windows already has. Nothing is downloaded, nothing
  leaves your machine.

Nothing else is spoken yet. Other briefly shown lists are planned.

## Requirements

- Cyberpunk 2077 **patch 2.31** (game file version `3.0.80.51928`). The
  plugin declares this exact runtime to RED4ext. On any other game version
  RED4ext silently refuses to load it: nothing crashes, the mod just does
  nothing.
- **RED4ext** matching that game version.
- **redscript** (the script compiler that runs at game launch).
- Windows 10 or 11 with at least one text-to-speech voice installed. Every
  stock install has Microsoft David and Microsoft Zira.

Cyber Engine Tweaks is not required.

## Install

FreeTTS has two parts, a native RED4ext plugin and a redscript file. Copy
them into your game folder at these paths:

```
red4ext/plugins/FreeTTS/FreeTTS.dll
r6/scripts/FreeTTS/FreeTTS.reds
```

With Vortex, import the release zip; it already has this layout. Vortex
reads no name or version from a local archive, so type these in the mod
details pane by hand:

- Name: FreeTTS
- Version: 0.1.0
- Author: ringo

Which voice speaks, and how fast, is whatever Windows is set to. Change it
under Settings > Time & Language > Speech.

## Troubleshooting

Open `red4ext/logs/freetts-<date>.log` in the game folder.

- No `freetts-<date>.log` file for this launch at all: the plugin was not
  loaded. Check the game version against Requirements, and that the DLL is
  at exactly the path above (one folder deep under `red4ext/plugins`).
- `FreeTTS loaded` but no `SAPI voice ready`: Windows could not create a
  voice. The line after it names the failing call. Confirm a voice exists
  under Settings > Time & Language > Speech.
- `speaking: ...` lines appear but you hear nothing: the voice is working
  and the game is calling it. Check the Windows volume mixer; the speech
  plays through the default output device, not the game's.
- Neither `speaking:` nor any warning when you cycle the list: the script
  did not compile. Look in `r6/logs/redscript_r*.log`.

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

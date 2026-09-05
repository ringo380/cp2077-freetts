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
- It uses the voice Windows already has. Nothing is downloaded, nothing
  leaves your machine.

Phone and text-message choices, the radio, the weapon wheel, and the map's
filter bar and tracked-quest panel are not spoken yet.

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
```

With Vortex, import the release zip; it already has this layout. Vortex
reads no name or version from a local archive, so type these in the mod
details pane by hand:

- Name: FreeTTS
- Version: 0.4.0
- Author: ringo

## Settings

With Mod Settings installed, FreeTTS appears under Settings > Mods, and in
Mod Configuration Menu if you use that. Four groups:

- **Quickhacks**: Speak quickhacks (on/off), Quickhack speech rate.
- **Dialogue**: Speak dialogue choices (on/off), Dialogue speech rate.
- **Map**: Speak map (on/off), Map speech rate.
- **Voice**: Voice, a number. 0 is the Windows default voice; 1 and up
  pick one voice for the whole mod, see Voices below.

A rate runs from -10 (slowest) to 10 (fastest); 0 is the voice's own speed.
The rate for a group is greyed out while that group is off. Changes apply
the next time something is spoken; no restart.

## Voices

With Voice at 0 the mod speaks with the Windows default text-to-speech
voice, which is set in the old Speech control panel (run `sapi.cpl`, or
Control Panel > Speech Recognition > Text to Speech). To use a different
voice in the game without changing the Windows default, set Voice to that
voice's number. The plugin lists every voice it can see at startup in
`red4ext/logs/freetts-<date>.log`, for example:

```
voice 1: Microsoft Zira Desktop - English (United States)
voice 2: Microsoft David Desktop - English (United States)
voice 3: Microsoft Mark - English (United States)
```

The number is that voice's position in the list; the log's order is the
one that counts, and it can differ from what other programs show. The change applies to
the next thing spoken; the log then says `voice set: <name>`. A number
past the end of the list falls back to the Windows default and logs one
line saying so.

A stock Windows install shows only the two "Desktop" voices, David and
Zira. Two ways to get more:

- **The hidden Mark voice.** Windows 10 and 11 ship Microsoft Mark (and
  newer copies of David and Zira) for the newer speech stack, hidden from
  the older one the mod uses. Copying its registry entry across makes it
  visible. In an administrator PowerShell:

  ```
  reg copy HKLM\SOFTWARE\Microsoft\Speech_OneCore\Voices\Tokens\MSTTS_V110_enUS_MarkM HKLM\SOFTWARE\Microsoft\Speech\Voices\Tokens\MSTTS_V110_enUS_MarkM /s /f
  ```

  To undo it: `reg delete HKLM\SOFTWARE\Microsoft\Speech\Voices\Tokens\MSTTS_V110_enUS_MarkM /f`.
  The same works for any voice listed under `Speech_OneCore\Voices\Tokens`,
  such as the ones Windows adds when you install another display language
  with its speech pack.
- **Natural voices (much better).** Windows 11's Narrator has neural
  voices (Aria, Guy, Jenny, Sonia, and more). They are Narrator-only
  until a bridge is installed: [NaturalVoiceSAPIAdapter](https://github.com/gexgd0419/NaturalVoiceSAPIAdapter)
  (MIT) registers them as ordinary Windows voices, and the mod then lists
  them like any other. Run the adapter's installer (x64 is the one the
  game needs) and untick both online voice options so only local voices
  are offered. Uninstall from the same installer.

  One catch, as of September 2026: the voice packages the Microsoft Store
  installs today (Settings > Accessibility > Narrator > Add natural
  voices) use a newer encryption the adapter cannot open. They show up in
  the list but fail with `EMBEDDED_TTS_ERROR_INVALID_LICENSE` the moment
  they speak through the adapter's automatic voice list, and the HD
  voices (Ava HD, Andrew HD, and friends) exist only in that newer form.
  Two ways round it.

  The simple one: the adapter's wiki page "Narrator natural voice download
  links" points at the older package versions that still work (Aria,
  Guy, Jenny, Ryan, Sonia among them). Download one, check its signature
  is Microsoft's (`Get-AuthenticodeSignature <file>.Msix` in PowerShell,
  expect `Valid` and `O=Microsoft Corporation`), unzip it (it is a zip)
  into its own subfolder of a folder you keep, and set that folder as the
  adapter's "Local voice path" (the installer field, or the registry
  value `NarratorVoicePath` under
  `HKCU\Software\NaturalVoiceSAPIAdapter\Enumerator`). The adapter's own
  log, `%LOCALAPPDATA%\NaturalVoiceSAPIAdapter\log.txt`, says which voice
  it could not initialize and why.

  The other, for a current Store voice such as Ava HD: the new packages
  are not encrypted at all; each model file starts with a plaintext
  license paragraph, and the adapter's engine accepts a voice token that
  carries that paragraph as a `License` value in place of its built-in
  key. Copy the package folder out of `C:\Program Files\WindowsApps`, and
  write a token under `HKLM\SOFTWARE\Microsoft\Speech\Voices\Tokens` with
  the adapter engine's CLSID, an `Attributes` subkey (Name, Gender, Age,
  Language, Locale, Vendor), and a `NaturalVoiceConfig` subkey holding
  `Path` (the copied folder) and `License` (the paragraph, up to "for
  others to use.", without the 16-character marker after it). The shape
  of the token is the one the adapter itself writes, visible in its
  source (`MakeLocalVoiceToken`). Whether using a Narrator voice this way
  is within its license is a question for you, not for this mod.

Either way, quit and relaunch the game after adding a voice; the list is
read once at startup.

## Troubleshooting

Open `red4ext/logs/freetts-<date>.log` in the game folder.

- No `freetts-<date>.log` file for this launch at all: the plugin was not
  loaded. Check the game version against Requirements, and that the DLL is
  at exactly the path above (one folder deep under `red4ext/plugins`).
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
  number is past the end of it.
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

#pragma once

#include <string>

// Output through a running screen reader instead of the Windows voice. NVDA
// only, through NV Access's nvdaControllerClient.dll shipped beside
// FreeTTS.dll and loaded at runtime, so a missing DLL just means no screen
// reader output. Every call belongs to the speaker's worker thread: the
// controller client talks to NVDA over RPC, which can stall, and the game's
// script thread must never wait on it.
namespace freetts::reader
{
using LogFn = void (*)(const char* aMessage);

// Loads the controller client from FreeTTS.dll's own folder. Logs one line
// either way. Idempotent.
void Load(LogFn aInfo, LogFn aError);

// Frees the controller client.
void Unload();

// Speaks aText through NVDA, interrupting whatever NVDA was saying, and shows
// it on a braille display if one is connected. Returns false, having sent
// nothing, when the DLL is missing or NVDA is not running; the caller then
// uses the Windows voice. Empty text only cancels. Logs when NVDA appears or
// goes away, not on every call.
bool Speak(const std::wstring& aText);
} // namespace freetts::reader

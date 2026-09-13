#pragma once

#include <string>

// A single Windows SAPI voice owned by a worker thread. The game's script VM
// calls Say() and never touches COM; the worker initialises COM for itself,
// creates the voice, and speaks whatever the latest request is.
namespace freetts::speaker
{
using LogFn = void (*)(const char* aMessage);

// Starts the worker. Idempotent. Logging goes through the two callbacks, which
// must be safe to call from a thread other than the game's. The worker logs
// every installed voice as `voice <n>: <name>`; <n> is what Say's aVoice means.
void Start(LogFn aInfo, LogFn aError);

// Stops and joins the worker, cancelling any speech in progress. Idempotent.
void Stop();

// True once the worker has a usable voice. False before Start, after Stop, and
// forever if voice creation failed.
bool IsReady();

// Replaces whatever is pending or currently being spoken with aUtf8 (empty
// text just cancels), spoken at aRate on SAPI's -10..10 scale (0 is the
// voice's own speed; out-of-range values are clamped) by voice aVoice (0 is
// the Windows default, 1.. is the position in the logged voice list; a number
// past the end falls back to the default). Returns IsReady(); a false return
// means nothing will be spoken.
bool Say(const std::string& aUtf8, int aRate, int aVoice);

// The logged display name of voice aVoice (1.. is the position in the voice
// list, 0 the Windows default voice), or "" when there is no such voice or
// the list has not been read yet. Safe from any thread.
std::string VoiceName(int aVoice);

// Remembers voice aVoice (1.. is the position in the voice list; 0 forgets)
// by its logged name in %LOCALAPPDATA%\FreeTTS\voice.txt, so the choice
// survives Windows reordering the list, and makes it what SavedVoice reports.
// Returns false when there is no such voice or the list has not been read
// yet; a file that cannot be written is logged and the choice still holds
// for this session. Safe from any thread.
bool SelectVoice(int aVoice);

// The position of the remembered voice in this session's list, or 0 when
// nothing is remembered, the remembered name is no longer installed, or the
// list has not been read yet. This is the voice every Say should pass.
int SavedVoice();
} // namespace freetts::speaker

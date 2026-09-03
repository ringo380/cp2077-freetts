#pragma once

#include <string>

// A single Windows SAPI voice owned by a worker thread. The game's script VM
// calls Say() and never touches COM; the worker initialises COM for itself,
// creates the voice, and speaks whatever the latest request is.
namespace freetts::speaker
{
using LogFn = void (*)(const char* aMessage);

// Starts the worker. Idempotent. Logging goes through the two callbacks, which
// must be safe to call from a thread other than the game's.
void Start(LogFn aInfo, LogFn aError);

// Stops and joins the worker, cancelling any speech in progress. Idempotent.
void Stop();

// True once the worker has a usable voice. False before Start, after Stop, and
// forever if voice creation failed.
bool IsReady();

// Replaces whatever is pending or currently being spoken with aUtf8 (empty
// text just cancels). Returns IsReady(); a false return means nothing will be
// spoken.
bool Say(const std::string& aUtf8);
} // namespace freetts::speaker

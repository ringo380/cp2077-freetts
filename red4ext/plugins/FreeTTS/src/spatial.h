#pragma once

#include <string>

// Positional WAV playback for lines said by someone standing in the world.
// A worker thread owns XAudio2 and X3DAudio; the script thread only queues
// requests and reads which handles are still playing, so no call here
// blocks on audio or disk.
namespace freetts::spatial
{
using LogFn = void (*)(const char* aMessage);

// Starts the worker. Idempotent. The audio engine itself is created on the
// first play, so a game that never plays a positional line opens no device.
void Start(LogFn aInfo, LogFn aError);

// Stops every voice, releases the engine, joins the worker. Idempotent.
void Stop();

// Queues aUtf8Path (RIFF, PCM 16-bit, mono) to play from world position
// (aX, aY, aZ) in game coordinates (metres, Z up). Returns a handle > 0, or
// 0 when the engine has already failed for good. A file that cannot be read
// is logged by the worker and its handle simply stops playing.
int PlayAt(const std::string& aUtf8Path, float aX, float aY, float aZ);

// The listener in game coordinates: position, forward and up (unit vectors).
// Every playing voice is re-panned and re-attenuated against it.
void SetListener(float aPx, float aPy, float aPz, float aFx, float aFy, float aFz, float aUx, float aUy, float aUz);

// True while the handle is queued or playing.
bool IsPlaying(int aHandle);

// Stops the handle now (no-op when it already finished).
void StopSound(int aHandle);
} // namespace freetts::spatial

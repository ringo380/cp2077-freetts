#include "spatial.h"

#include <Windows.h>
#include <objbase.h>
#include <x3daudio.h>
#include <xaudio2.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <iterator>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace
{
struct Vec
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct PlayRequest
{
    int          handle = 0;
    std::wstring path;
    std::string  utf8Path; // for log lines
    Vec          pos;
};

struct Listener
{
    Vec  pos;
    Vec  forward{0.0f, 1.0f, 0.0f};
    Vec  up{0.0f, 0.0f, 1.0f};
    bool set = false;
};

// Shared with the script thread, all under s_mutex.
std::mutex              s_mutex;
std::condition_variable s_wake;
std::thread             s_thread;
bool                    s_stop       = false;
bool                    s_started    = false;
int                     s_nextHandle = 1;
std::deque<PlayRequest> s_plays;
std::vector<int>        s_stops;
std::set<int>           s_live; // queued or playing
Listener                s_listener;
// Set once the engine could not be created; PlayAt then refuses at once.
std::atomic<bool> s_failed{false};

freetts::spatial::LogFn s_info  = nullptr;
freetts::spatial::LogFn s_error = nullptr;

void Info(const char* aMessage)
{
    if (s_info)
        s_info(aMessage);
}

void Error(const char* aMessage)
{
    if (s_error)
        s_error(aMessage);
}

void ErrorHr(const char* aWhat, HRESULT aHr)
{
    char buffer[200];
    std::snprintf(buffer, sizeof(buffer), "spatial: %s failed, HRESULT 0x%08lX", aWhat, static_cast<unsigned long>(aHr));
    Error(buffer);
}

std::wstring ToWide(const std::string& aUtf8)
{
    if (aUtf8.empty())
        return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, aUtf8.data(), static_cast<int>(aUtf8.size()), nullptr, 0);
    if (length <= 0)
        return {};
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, aUtf8.data(), static_cast<int>(aUtf8.size()), wide.data(), length);
    return wide;
}

void Forget(int aHandle)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_live.erase(aHandle);
}

// The game's world is right-handed with Z up; X3DAudio is left-handed with
// Y up. Swapping Y and Z does both at once (a reflection flips handedness).
// If left and right come out reversed in game, this is the one place to fix.
X3DAUDIO_VECTOR ToX3D(const Vec& aGame)
{
    return X3DAUDIO_VECTOR{aGame.x, aGame.z, aGame.y};
}

// Full volume within 2 m, then linear to silence at 35 m. X3DAudio curve
// distances are normalised by the emitter's CurveDistanceScaler (35 m).
constexpr float               kMaxMeters     = 35.0f;
X3DAUDIO_DISTANCE_CURVE_POINT s_curvePoints[] = {{0.0f, 1.0f}, {2.0f / kMaxMeters, 1.0f}, {1.0f, 0.0f}};
X3DAUDIO_DISTANCE_CURVE       s_curve         = {s_curvePoints, 3};

struct Wav
{
    WAVEFORMATEX      format{};
    std::vector<BYTE> data;
};

uint32_t ReadU32(const BYTE* aAt)
{
    uint32_t v;
    std::memcpy(&v, aAt, 4);
    return v;
}

uint16_t ReadU16(const BYTE* aAt)
{
    uint16_t v;
    std::memcpy(&v, aAt, 2);
    return v;
}

// RIFF / WAVE, PCM 16-bit mono (what the voice sidecar writes). Anything
// else fills aWhy and returns false.
bool LoadWav(const std::wstring& aPath, Wav& aOut, std::string& aWhy)
{
    std::ifstream file(aPath, std::ios::binary);
    if (!file)
    {
        aWhy = "cannot open the file";
        return false;
    }
    std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0)
    {
        aWhy = "not a RIFF WAVE file";
        return false;
    }

    bool   haveFormat = false;
    bool   haveData   = false;
    size_t at         = 12;
    while (at + 8 <= bytes.size())
    {
        const BYTE*    chunk = bytes.data() + at;
        const uint32_t size  = ReadU32(chunk + 4);
        const size_t   body  = at + 8;
        if (body + size > bytes.size())
            break;
        if (std::memcmp(chunk, "fmt ", 4) == 0 && size >= 16)
        {
            const BYTE* f               = bytes.data() + body;
            aOut.format.wFormatTag      = ReadU16(f);
            aOut.format.nChannels       = ReadU16(f + 2);
            aOut.format.nSamplesPerSec  = ReadU32(f + 4);
            aOut.format.nAvgBytesPerSec = ReadU32(f + 8);
            aOut.format.nBlockAlign     = ReadU16(f + 12);
            aOut.format.wBitsPerSample  = ReadU16(f + 14);
            aOut.format.cbSize          = 0;
            haveFormat                  = true;
        }
        else if (std::memcmp(chunk, "data", 4) == 0)
        {
            aOut.data.assign(bytes.begin() + body, bytes.begin() + body + size);
            haveData = true;
        }
        at = body + size + (size & 1); // chunks are padded to an even size
    }

    if (!haveFormat || !haveData)
    {
        aWhy = haveFormat ? "no data chunk" : "no fmt chunk";
        return false;
    }
    if (aOut.format.wFormatTag != WAVE_FORMAT_PCM || aOut.format.wBitsPerSample != 16 || aOut.format.nChannels != 1)
    {
        char buffer[120];
        std::snprintf(buffer, sizeof(buffer), "format %u, %u bit, %u channel(s); only PCM 16-bit mono plays",
                      aOut.format.wFormatTag, aOut.format.wBitsPerSample, aOut.format.nChannels);
        aWhy = buffer;
        return false;
    }
    if (aOut.data.empty() || aOut.format.nAvgBytesPerSec == 0)
    {
        aWhy = "no samples";
        return false;
    }
    return true;
}

struct Voice
{
    int                  handle = 0;
    IXAudio2SourceVoice* voice  = nullptr;
    std::vector<BYTE>    data; // must outlive the voice: XAudio2 reads it in place
    Vec                  pos;
};

struct Engine
{
    IXAudio2*               xaudio   = nullptr;
    IXAudio2MasteringVoice* master   = nullptr;
    X3DAUDIO_HANDLE         x3d      = {};
    UINT32                  channels = 0;
    bool                    tried    = false;

    // Created on the first play, once. A failure is logged and final.
    bool Ensure()
    {
        if (xaudio)
            return true;
        if (tried)
            return false;
        tried = true;

        HRESULT hr = XAudio2Create(&xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr))
        {
            ErrorHr("XAudio2Create", hr);
            xaudio = nullptr;
            return false;
        }
        hr = xaudio->CreateMasteringVoice(&master);
        if (FAILED(hr))
        {
            ErrorHr("CreateMasteringVoice", hr);
            xaudio->Release();
            xaudio = nullptr;
            return false;
        }
        DWORD mask = 0;
        master->GetChannelMask(&mask);
        XAUDIO2_VOICE_DETAILS details{};
        master->GetVoiceDetails(&details);
        channels = details.InputChannels;
        hr       = X3DAudioInitialize(mask, X3DAUDIO_SPEED_OF_SOUND, x3d);
        if (FAILED(hr))
        {
            ErrorHr("X3DAudioInitialize", hr);
            master->DestroyVoice();
            master = nullptr;
            xaudio->Release();
            xaudio = nullptr;
            return false;
        }
        char buffer[120];
        std::snprintf(buffer, sizeof(buffer), "spatial: engine ready, %u output channel(s), mask 0x%lX", channels,
                      static_cast<unsigned long>(mask));
        Info(buffer);
        return true;
    }

    void Release()
    {
        if (master)
            master->DestroyVoice();
        master = nullptr;
        if (xaudio)
            xaudio->Release();
        xaudio = nullptr;
    }
};

void Apply3D(Engine& aEngine, Voice& aVoice, const Listener& aListener)
{
    X3DAUDIO_LISTENER listener{};
    listener.OrientFront = ToX3D(aListener.forward);
    listener.OrientTop   = ToX3D(aListener.up);
    listener.Position    = ToX3D(aListener.pos);

    X3DAUDIO_EMITTER emitter{};
    emitter.OrientFront         = X3DAUDIO_VECTOR{0.0f, 0.0f, 1.0f};
    emitter.OrientTop           = X3DAUDIO_VECTOR{0.0f, 1.0f, 0.0f};
    emitter.Position            = ToX3D(aVoice.pos);
    emitter.ChannelCount        = 1;
    emitter.CurveDistanceScaler = kMaxMeters;
    emitter.DopplerScaler       = 0.0f;
    emitter.pVolumeCurve        = &s_curve;
    // Within half a metre the pan eases toward the centre instead of
    // snapping hard left or right as V brushes past her.
    emitter.InnerRadius      = 0.5f;
    emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;

    float                 matrix[XAUDIO2_MAX_AUDIO_CHANNELS] = {};
    X3DAUDIO_DSP_SETTINGS dsp{};
    dsp.SrcChannelCount     = 1;
    dsp.DstChannelCount     = aEngine.channels;
    dsp.pMatrixCoefficients = matrix;
    X3DAudioCalculate(aEngine.x3d, &listener, &emitter, X3DAUDIO_CALCULATE_MATRIX, &dsp);
    aVoice.voice->SetOutputMatrix(aEngine.master, 1, aEngine.channels, matrix);
}

void StartVoice(Engine& aEngine, std::vector<Voice>& aVoices, PlayRequest& aPlay, const Listener& aListener)
{
    Wav         wav;
    std::string why;
    if (!LoadWav(aPlay.path, wav, why))
    {
        Error(("spatial: " + std::to_string(aPlay.handle) + " not played, " + why + ": " + aPlay.utf8Path).c_str());
        Forget(aPlay.handle);
        return;
    }

    Voice v;
    v.handle = aPlay.handle;
    v.pos    = aPlay.pos;
    v.data   = std::move(wav.data);

    HRESULT hr = aEngine.xaudio->CreateSourceVoice(&v.voice, &wav.format);
    if (FAILED(hr))
    {
        ErrorHr("CreateSourceVoice", hr);
        Forget(aPlay.handle);
        return;
    }

    XAUDIO2_BUFFER buffer{};
    buffer.Flags      = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = static_cast<UINT32>(v.data.size());
    buffer.pAudioData = v.data.data();
    hr                = v.voice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr))
    {
        ErrorHr("SubmitSourceBuffer", hr);
        v.voice->DestroyVoice();
        Forget(aPlay.handle);
        return;
    }

    if (aListener.set)
        Apply3D(aEngine, v, aListener);
    hr = v.voice->Start(0);
    if (FAILED(hr))
    {
        ErrorHr("Start", hr);
        v.voice->DestroyVoice();
        Forget(aPlay.handle);
        return;
    }

    const double seconds = static_cast<double>(v.data.size()) / wav.format.nAvgBytesPerSec;
    char         line[160];
    std::snprintf(line, sizeof(line), "spatial: %d plays %.1f s at %.1f, %.1f, %.1f", v.handle, seconds, v.pos.x,
                  v.pos.y, v.pos.z);
    Info(line);
    aVoices.push_back(std::move(v));
}

void Worker()
{
    const HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    Engine             engine;
    std::vector<Voice> voices;

    for (;;)
    {
        std::deque<PlayRequest> plays;
        std::vector<int>        stops;
        Listener                listener;
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            const auto pending = [] { return s_stop || !s_plays.empty() || !s_stops.empty(); };
            // Idle, the worker sleeps until a request; while anything plays
            // it wakes every 20 ms to re-pan against the latest listener.
            if (voices.empty())
                s_wake.wait(lock, pending);
            else
                s_wake.wait_for(lock, std::chrono::milliseconds(20), pending);
            if (s_stop)
                break;
            plays.swap(s_plays);
            stops.swap(s_stops);
            listener = s_listener;
        }

        for (auto& play : plays)
        {
            if (!engine.Ensure())
            {
                s_failed = true;
                Forget(play.handle);
                continue;
            }
            StartVoice(engine, voices, play, listener);
        }

        for (auto it = voices.begin(); it != voices.end();)
        {
            bool stop = false;
            for (int h : stops)
                stop = stop || h == it->handle;
            XAUDIO2_VOICE_STATE state{};
            it->voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            if (stop || state.BuffersQueued == 0)
            {
                it->voice->DestroyVoice();
                Forget(it->handle);
                it = voices.erase(it);
            }
            else
            {
                if (listener.set)
                    Apply3D(engine, *it, listener);
                ++it;
            }
        }
    }

    for (auto& v : voices)
        v.voice->DestroyVoice();
    voices.clear();
    engine.Release();
    if (SUCCEEDED(co))
        CoUninitialize();
}
} // namespace

namespace freetts::spatial
{
void Start(LogFn aInfo, LogFn aError)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_started)
        return;
    s_info    = aInfo;
    s_error   = aError;
    s_stop    = false;
    s_started = true;
    s_thread  = std::thread(&Worker);
}

void Stop()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_started)
            return;
        s_stop = true;
    }
    s_wake.notify_all();
    if (s_thread.joinable())
        s_thread.join();
    std::lock_guard<std::mutex> lock(s_mutex);
    s_started = false;
    s_plays.clear();
    s_stops.clear();
    s_live.clear();
}

int PlayAt(const std::string& aUtf8Path, float aX, float aY, float aZ)
{
    if (s_failed)
        return 0;
    int handle = 0;
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_started)
            return 0;
        handle = s_nextHandle++;
        PlayRequest play;
        play.handle   = handle;
        play.path     = ToWide(aUtf8Path);
        play.utf8Path = aUtf8Path;
        play.pos      = Vec{aX, aY, aZ};
        s_plays.push_back(std::move(play));
        s_live.insert(handle);
    }
    s_wake.notify_all();
    return handle;
}

void SetListener(float aPx, float aPy, float aPz, float aFx, float aFy, float aFz, float aUx, float aUy, float aUz)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_listener.pos     = Vec{aPx, aPy, aPz};
    s_listener.forward = Vec{aFx, aFy, aFz};
    s_listener.up      = Vec{aUx, aUy, aUz};
    s_listener.set     = true;
}

bool IsPlaying(int aHandle)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_live.count(aHandle) > 0;
}

void StopSound(int aHandle)
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_live.count(aHandle))
            return;
        s_stops.push_back(aHandle);
    }
    s_wake.notify_all();
}
} // namespace freetts::spatial

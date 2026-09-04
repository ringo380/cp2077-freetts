#include "speaker.h"

#include <Windows.h>
#include <objbase.h>
#include <sapi.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <optional>
#include <thread>

namespace
{
std::mutex                  s_mutex;
std::condition_variable     s_wake;
std::thread                 s_thread;
// One utterance: what to say and how fast. Rate travels with the text so the
// worker, the only thread that touches the voice, is also the only one that
// calls SetRate.
struct Request
{
    std::wstring text;
    long         rate = 0;
};

std::optional<Request> s_pending; // single slot: a new request replaces an unspoken one
bool                        s_stop    = false;
bool                        s_started = false;
std::atomic<bool>           s_ready{false};

freetts::speaker::LogFn s_info  = nullptr;
freetts::speaker::LogFn s_error = nullptr;

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
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "%s failed, HRESULT 0x%08lX", aWhat, static_cast<unsigned long>(aHr));
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

void Worker()
{
    // The worker is the only thread that ever touches the voice, so it owns its
    // own apartment. Doing this on the script thread instead would collide with
    // whatever COM state the game already established there.
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initHr))
    {
        ErrorHr("CoInitializeEx", initHr);
        return;
    }

    ISpVoice* voice = nullptr;
    const HRESULT createHr =
        CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, reinterpret_cast<void**>(&voice));
    if (FAILED(createHr) || !voice)
    {
        ErrorHr("CoCreateInstance(SpVoice)", createHr);
        CoUninitialize();
        return;
    }

    s_ready = true;
    Info("SAPI voice ready");

    long currentRate = 0;

    for (;;)
    {
        std::optional<Request> request;
        {
            std::unique_lock lock(s_mutex);
            s_wake.wait(lock, [] { return s_stop || s_pending.has_value(); });
            if (s_stop)
                break;
            request = std::move(s_pending);
            s_pending.reset();
        }

        // SetRate applies to the next Speak, so it goes first. A failure is
        // logged and the text is still spoken at whatever rate the voice has.
        if (request->rate != currentRate)
        {
            const HRESULT rateHr = voice->SetRate(request->rate);
            if (FAILED(rateHr))
                ErrorHr("ISpVoice::SetRate", rateHr);
            else
                currentRate = request->rate;
        }

        // Purge first so cycling quickly through a list cuts the previous item
        // off instead of queueing it; async so this loop is free to accept the
        // next request while the voice is still talking.
        const HRESULT speakHr = voice->Speak(request->text.empty() ? nullptr : request->text.c_str(),
                                             SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
        if (FAILED(speakHr))
            ErrorHr("ISpVoice::Speak", speakHr);
    }

    s_ready = false;
    voice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
    voice->Release();
    CoUninitialize();
}
} // namespace

namespace freetts::speaker
{
void Start(LogFn aInfo, LogFn aError)
{
    std::lock_guard lock(s_mutex);
    if (s_started)
        return;

    s_info    = aInfo;
    s_error   = aError;
    s_stop    = false;
    s_started = true;
    s_thread  = std::thread(Worker);
}

void Stop()
{
    {
        std::lock_guard lock(s_mutex);
        if (!s_started)
            return;
        s_stop = true;
        s_pending.reset();
    }
    s_wake.notify_all();

    if (s_thread.joinable())
        s_thread.join();

    std::lock_guard lock(s_mutex);
    s_started = false;
    s_ready   = false;
}

bool IsReady()
{
    return s_ready.load();
}

bool Say(const std::string& aUtf8, int aRate)
{
    if (!s_ready.load())
        return false;

    {
        std::lock_guard lock(s_mutex);
        s_pending = Request{ToWide(aUtf8), static_cast<long>(std::clamp(aRate, -10, 10))};
    }
    s_wake.notify_one();
    return true;
}
} // namespace freetts::speaker

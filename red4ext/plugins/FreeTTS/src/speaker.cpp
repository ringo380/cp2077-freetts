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
#include <string>
#include <thread>
#include <vector>

namespace
{
std::mutex                  s_mutex;
std::condition_variable     s_wake;
std::thread                 s_thread;
// One utterance: what to say, how fast, and with which voice. Rate and voice
// travel with the text so the worker, the only thread that touches the voice,
// is also the only one that calls SetRate and SetVoice.
struct Request
{
    std::wstring text;
    long         rate  = 0;
    int          voice = 0; // 0 = Windows default, 1.. = position in the SAPI voice list
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

std::string ToUtf8(const wchar_t* aWide)
{
    if (!aWide || !*aWide)
        return {};

    const int length = WideCharToMultiByte(CP_UTF8, 0, aWide, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1)
        return {};

    std::string utf8(static_cast<std::size_t>(length - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, aWide, -1, utf8.data(), length, nullptr, nullptr);
    return utf8;
}

// The display name of a voice token ("Microsoft Zira Desktop - English
// (United States)"), or "?" when the token will not say.
std::string TokenName(ISpObjectToken* aToken)
{
    LPWSTR description = nullptr;
    if (!aToken || FAILED(aToken->GetStringValue(nullptr, &description)) || !description)
        return "?";
    std::string name = ToUtf8(description);
    CoTaskMemFree(description);
    return name.empty() ? "?" : name;
}

// Every installed SAPI voice, in Windows' own order, logged one per line so the
// settings page's voice number can be matched to a name. The tokens are
// AddRef'd; the caller releases them.
std::vector<ISpObjectToken*> EnumerateVoices()
{
    std::vector<ISpObjectToken*> voices;

    ISpObjectTokenCategory* category = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL, IID_ISpObjectTokenCategory,
                                  reinterpret_cast<void**>(&category));
    if (FAILED(hr) || !category)
    {
        ErrorHr("CoCreateInstance(SpObjectTokenCategory)", hr);
        return voices;
    }

    IEnumSpObjectTokens* tokens = nullptr;
    hr = category->SetId(SPCAT_VOICES, FALSE);
    if (SUCCEEDED(hr))
        hr = category->EnumTokens(nullptr, nullptr, &tokens);
    if (FAILED(hr) || !tokens)
    {
        ErrorHr("ISpObjectTokenCategory::EnumTokens", hr);
        category->Release();
        return voices;
    }

    ISpObjectToken* token = nullptr;
    while (tokens->Next(1, &token, nullptr) == S_OK && token)
    {
        voices.push_back(token);
        char line[320];
        std::snprintf(line, sizeof(line), "voice %zu: %s", voices.size(), TokenName(token).c_str());
        Info(line);
        token = nullptr;
    }

    tokens->Release();
    category->Release();
    return voices;
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

    // The voice Windows chose is kept so setting 0 can go back to it after a
    // numbered voice was in use.
    ISpObjectToken* defaultToken = nullptr;
    if (FAILED(voice->GetVoice(&defaultToken)))
        defaultToken = nullptr;

    std::vector<ISpObjectToken*> voices = EnumerateVoices();

    s_ready = true;
    Info("SAPI voice ready");

    long currentRate  = 0;
    int  currentVoice = 0;
    int  warnedVoice  = 0; // last out-of-range number complained about, so the log gets one line per choice

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

        // Voice first, since SetVoice can reset the rate on some engines; then
        // the rate; both apply to the Speak that follows. A number past the end
        // of the list falls back to the Windows default and is logged once.
        if (request->voice != currentVoice)
        {
            int wanted = request->voice;
            if (wanted < 0 || wanted > static_cast<int>(voices.size()))
            {
                if (wanted != warnedVoice)
                {
                    char line[160];
                    std::snprintf(line, sizeof(line), "voice %d is not installed (%zu available), using the Windows default",
                                  wanted, voices.size());
                    Error(line);
                    warnedVoice = wanted;
                }
                wanted = 0;
            }

            if (wanted != currentVoice)
            {
                ISpObjectToken* token   = wanted == 0 ? defaultToken : voices[static_cast<std::size_t>(wanted) - 1];
                const HRESULT   voiceHr = voice->SetVoice(token);
                if (FAILED(voiceHr))
                {
                    ErrorHr("ISpVoice::SetVoice", voiceHr);
                }
                else
                {
                    currentVoice = wanted;
                    currentRate  = 0x7fffffff; // force SetRate below
                    char line[320];
                    std::snprintf(line, sizeof(line), "voice set: %s", TokenName(token).c_str());
                    Info(line);
                }
            }
            else
            {
                currentVoice = wanted;
            }
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
    for (ISpObjectToken* token : voices)
        token->Release();
    if (defaultToken)
        defaultToken->Release();
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

bool Say(const std::string& aUtf8, int aRate, int aVoice)
{
    if (!s_ready.load())
        return false;

    {
        std::lock_guard lock(s_mutex);
        s_pending = Request{ToWide(aUtf8), static_cast<long>(std::clamp(aRate, -10, 10)), aVoice};
    }
    s_wake.notify_one();
    return true;
}
} // namespace freetts::speaker

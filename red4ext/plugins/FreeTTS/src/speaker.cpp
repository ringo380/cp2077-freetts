#include "speaker.h"

#include "reader.h"

#include <Windows.h>
#include <objbase.h>
#include <sapi.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cwchar>
#include <cstdio>
#include <fstream>
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
    bool         windowsVoice = false; // skip the screen reader (voice previews)
};

std::optional<Request> s_pending; // single slot: a new request replaces an unspoken one
// Display names, filled once by the worker: [0] is the Windows default voice,
// [n] the nth logged voice. Read by VoiceName from the script thread.
std::mutex               s_namesMutex;
std::vector<std::string> s_names;
// Each voice's language, parallel to s_names (0 when the token does not say).
std::vector<LANGID> s_langs;
// The game's on-screen text language, from SetLanguage; 0 until the script
// sends it or when the code was not recognised.
std::atomic<LANGID> s_gameLang{0};
// 0: a running screen reader speaks, else the Windows voice. 1: always the
// Windows voice. From SetOutput.
std::atomic<int> s_output{0};
// Position in s_names of the voice remembered in voice.txt, resolved once by
// the worker after the list is read and replaced by SelectVoice. 0 when
// nothing is remembered or the remembered name is gone.
std::atomic<int>            s_savedSlot{0};
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

// Where the chosen voice's name is kept: %LOCALAPPDATA%\FreeTTS\voice.txt.
// Outside the game folder on purpose, so a mod manager swapping versions
// never sees or removes it. Empty when LOCALAPPDATA is unset.
std::wstring VoiceFileDir()
{
    wchar_t   buffer[MAX_PATH];
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return {};
    return std::wstring(buffer, length) + L"\\FreeTTS";
}

std::wstring VoiceFilePath()
{
    const std::wstring dir = VoiceFileDir();
    return dir.empty() ? std::wstring{} : dir + L"\\voice.txt";
}

// The first line of voice.txt, the remembered voice's logged name, or "" when
// there is no file. UTF-8, trailing CR/LF dropped.
std::string ReadSavedName()
{
    const std::wstring path = VoiceFilePath();
    if (path.empty())
        return {};

    std::ifstream file(path, std::ios::binary);
    if (!file)
        return {};

    std::string line;
    std::getline(file, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
        line.pop_back();
    return line;
}

// Writes aName as the whole of voice.txt, or deletes the file when aName is
// empty. Logs the failure; the caller's in-memory choice stands regardless.
bool WriteSavedName(const std::string& aName)
{
    const std::wstring path = VoiceFilePath();
    if (path.empty())
    {
        Error("LOCALAPPDATA is not set, the voice choice will not survive a relaunch");
        return false;
    }

    if (aName.empty())
    {
        if (!DeleteFileW(path.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            Error("could not delete voice.txt, the old voice choice will come back on relaunch");
            return false;
        }
        return true;
    }

    CreateDirectoryW(VoiceFileDir().c_str(), nullptr); // already existing is fine
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
    {
        Error("could not write voice.txt, the voice choice will not survive a relaunch");
        return false;
    }
    file << aName << '\n';
    return static_cast<bool>(file);
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

// The voice's language from its Attributes\Language value, a hex LANGID such
// as "409", sometimes followed by ";9" and more. 0 when absent.
LANGID TokenLanguage(ISpObjectToken* aToken)
{
    ISpDataKey* attributes = nullptr;
    if (!aToken || FAILED(aToken->OpenKey(L"Attributes", &attributes)) || !attributes)
        return 0;

    LANGID language = 0;
    LPWSTR value    = nullptr;
    if (SUCCEEDED(attributes->GetStringValue(L"Language", &value)) && value)
    {
        language = static_cast<LANGID>(wcstoul(value, nullptr, 16));
        CoTaskMemFree(value);
    }
    attributes->Release();
    return language;
}

// The game's language setting ("de-de", "pt-br", "zh-cn") as a LANGID, 0 when
// Windows does not know it. The game spells some languages its own way (jp,
// kr, cz, ua, and ar-ar), so the region is dropped when the pair is unknown.
LANGID GameLanguageId(std::string aCode)
{
    std::transform(aCode.begin(), aCode.end(), aCode.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const std::size_t dash     = aCode.find('-');
    std::string       language = aCode.substr(0, dash);
    const std::string region   = dash == std::string::npos ? std::string{} : aCode.substr(dash + 1);

    static const std::pair<const char*, const char*> kAliases[] = {{"jp", "ja"}, {"kr", "ko"}, {"cz", "cs"}, {"ua", "uk"}};
    for (const auto& [game, bcp47] : kAliases)
    {
        if (language == game)
            language = bcp47;
    }

    // An unknown name gives 0 or LOCALE_CUSTOM_UNSPECIFIED (0x1000), not an error.
    const auto known = [](LCID aLcid) { return aLcid != 0 && aLcid != LOCALE_CUSTOM_UNSPECIFIED; };
    LCID       lcid  = 0;
    if (!region.empty())
        lcid = LocaleNameToLCID(ToWide(language + "-" + region).c_str(), 0);
    if (!known(lcid))
        lcid = LocaleNameToLCID(ToWide(language).c_str(), 0);
    if (!known(lcid))
        return 0;
    return LANGIDFROMLCID(lcid);
}

// Which voice "Windows default" means for the game's language: the default
// voice when it speaks that exact language, else the first voice that does,
// else the default when it at least shares the base language (en-GB for
// en-US), else the first that does, else the default. Caller holds
// s_namesMutex.
int LanguageSlot()
{
    const LANGID wanted = s_gameLang.load();
    if (wanted == 0 || s_langs.empty())
        return 0;

    for (std::size_t i = 0; i < s_langs.size(); ++i)
    {
        if (s_langs[i] == wanted)
            return static_cast<int>(i);
    }
    for (std::size_t i = 0; i < s_langs.size(); ++i)
    {
        if (s_langs[i] != 0 && PRIMARYLANGID(s_langs[i]) == PRIMARYLANGID(wanted))
            return static_cast<int>(i);
    }
    return 0;
}

// One log line naming the voice the game's language picked.
void LogLanguageSlot()
{
    char line[400];
    {
        std::lock_guard lock(s_namesMutex);
        if (s_names.empty())
            return;
        const int slot = LanguageSlot();
        if (slot == 0 && (s_langs.empty() || s_langs[0] == 0 ||
                          PRIMARYLANGID(s_langs[0]) != PRIMARYLANGID(s_gameLang.load())))
            std::snprintf(line, sizeof(line), "no voice for the game language (%04X), using the Windows default",
                          static_cast<unsigned>(s_gameLang.load()));
        else
            std::snprintf(line, sizeof(line), "game language voice: %s (voice %d)",
                          s_names[static_cast<std::size_t>(slot)].c_str(), slot);
    }
    Info(line);
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
    {
        std::lock_guard namesLock(s_namesMutex);
        s_names.clear();
        s_langs.clear();
        s_names.push_back(defaultToken ? TokenName(defaultToken) : "");
        s_langs.push_back(TokenLanguage(defaultToken));
        for (ISpObjectToken* t : voices)
        {
            s_names.push_back(TokenName(t));
            s_langs.push_back(TokenLanguage(t));
        }
    }
    if (s_gameLang.load() != 0)
        LogLanguageSlot(); // the script sent the language before the list was read

    // The remembered voice is matched by name, not by position, because
    // Windows reorders the list whenever a voice is added, removed, or made
    // the default. Resolved here, before ready, so the first Say sees it.
    {
        const std::string saved = ReadSavedName();
        int               slot  = 0;
        if (!saved.empty())
        {
            std::lock_guard namesLock(s_namesMutex);
            for (std::size_t i = 1; i < s_names.size(); ++i)
            {
                if (s_names[i] == saved)
                {
                    slot = static_cast<int>(i);
                    break;
                }
            }
        }
        s_savedSlot = slot;

        char line[400];
        if (saved.empty())
            std::snprintf(line, sizeof(line), "saved voice: none, using the Windows default");
        else if (slot == 0)
            std::snprintf(line, sizeof(line), "saved voice \"%s\" is not installed, using the Windows default",
                          saved.c_str());
        else
            std::snprintf(line, sizeof(line), "saved voice: %s (voice %d)", saved.c_str(), slot);
        if (slot == 0 && !saved.empty())
            Error(line);
        else
            Info(line);
    }

    freetts::reader::Load(s_info, s_error);

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

        // A running screen reader takes the text instead, in the player's own
        // reader voice and speed; the Windows voice is silenced so the two
        // never talk over each other.
        if (s_output.load() == 0 && !request->windowsVoice && freetts::reader::Speak(request->text))
        {
            voice->Speak(nullptr, SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
            continue;
        }

        // Voice first, since SetVoice can reset the rate on some engines; then
        // the rate; both apply to the Speak that follows. A number past the end
        // of the list falls back to the Windows default and is logged once.
        // "Windows default" means the game language's voice when there is one.
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
            if (wanted == 0)
            {
                std::lock_guard namesLock(s_namesMutex);
                wanted = LanguageSlot();
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
    freetts::reader::Unload();
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

std::string VoiceName(int aVoice)
{
    std::lock_guard lock(s_namesMutex);
    if (aVoice < 0 || aVoice >= static_cast<int>(s_names.size()))
        return {};
    return s_names[static_cast<std::size_t>(aVoice == 0 ? LanguageSlot() : aVoice)];
}

void SetOutput(int aOutput)
{
    const int output = aOutput == 1 ? 1 : 0;
    if (output == s_output.exchange(output))
        return;
    Info(output == 1 ? "output: Windows voice only" : "output: screen reader when running, else the Windows voice");
}

void SetLanguage(const std::string& aGameCode)
{
    const LANGID language = GameLanguageId(aGameCode);
    if (language == s_gameLang.exchange(language))
        return;

    char line[160];
    std::snprintf(line, sizeof(line), "game language: %s (%04X)", aGameCode.c_str(), static_cast<unsigned>(language));
    Info(line);
    if (s_ready.load())
        LogLanguageSlot();
}

bool SelectVoice(int aVoice)
{
    std::string name;
    {
        std::lock_guard lock(s_namesMutex);
        if (aVoice < 0 || aVoice >= static_cast<int>(s_names.size()))
            return false;
        if (aVoice > 0)
            name = s_names[static_cast<std::size_t>(aVoice)];
    }

    s_savedSlot = aVoice;
    char line[400];
    if (aVoice == 0)
        std::snprintf(line, sizeof(line), "voice choice cleared, using the Windows default");
    else
        std::snprintf(line, sizeof(line), "voice choice saved: %s (voice %d)", name.c_str(), aVoice);
    Info(line);
    return WriteSavedName(name);
}

int SavedVoice()
{
    return s_savedSlot.load();
}

bool Say(const std::string& aUtf8, int aRate, int aVoice, bool aWindowsVoice)
{
    if (!s_ready.load())
        return false;

    {
        std::lock_guard lock(s_mutex);
        s_pending = Request{ToWide(aUtf8), static_cast<long>(std::clamp(aRate, -10, 10)), aVoice, aWindowsVoice};
    }
    s_wake.notify_one();
    return true;
}
} // namespace freetts::speaker

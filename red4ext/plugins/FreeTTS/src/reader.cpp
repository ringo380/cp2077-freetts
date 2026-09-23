#include "reader.h"

#include <Windows.h>

namespace
{
// nvdaController.h's signatures; error_status_t is an unsigned long and 0 is
// success.
using TestIfRunningFn  = unsigned long(__stdcall*)();
using SpeakTextFn      = unsigned long(__stdcall*)(const wchar_t*);
using CancelSpeechFn   = unsigned long(__stdcall*)();
using BrailleMessageFn = unsigned long(__stdcall*)(const wchar_t*);

HMODULE          s_module         = nullptr;
TestIfRunningFn  s_testIfRunning  = nullptr;
SpeakTextFn      s_speakText      = nullptr;
CancelSpeechFn   s_cancelSpeech   = nullptr;
BrailleMessageFn s_brailleMessage = nullptr;
bool             s_loadTried      = false;
bool             s_wasRunning     = false;

freetts::reader::LogFn s_info  = nullptr;
freetts::reader::LogFn s_error = nullptr;

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

// The folder FreeTTS.dll was loaded from, with a trailing backslash, or ""
// when Windows will not say.
std::wstring OwnFolder()
{
    HMODULE self = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&OwnFolder), &self))
        return {};

    wchar_t     path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(self, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return {};

    std::wstring folder(path, length);
    const std::size_t slash = folder.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring{} : folder.substr(0, slash + 1);
}
} // namespace

namespace freetts::reader
{
void Load(LogFn aInfo, LogFn aError)
{
    s_info  = aInfo;
    s_error = aError;
    if (s_loadTried)
        return;
    s_loadTried = true;

    const std::wstring folder = OwnFolder();
    if (folder.empty())
    {
        Error("could not find FreeTTS.dll's folder, screen reader output is off");
        return;
    }

    // A full path, so only the copy shipped with FreeTTS is ever loaded.
    const std::wstring path = folder + L"nvdaControllerClient.dll";
    s_module = LoadLibraryExW(path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!s_module)
    {
        Info("nvdaControllerClient.dll not found, screen reader output is off");
        return;
    }

    s_testIfRunning  = reinterpret_cast<TestIfRunningFn>(GetProcAddress(s_module, "nvdaController_testIfRunning"));
    s_speakText      = reinterpret_cast<SpeakTextFn>(GetProcAddress(s_module, "nvdaController_speakText"));
    s_cancelSpeech   = reinterpret_cast<CancelSpeechFn>(GetProcAddress(s_module, "nvdaController_cancelSpeech"));
    s_brailleMessage = reinterpret_cast<BrailleMessageFn>(GetProcAddress(s_module, "nvdaController_brailleMessage"));
    if (!s_testIfRunning || !s_speakText || !s_cancelSpeech)
    {
        Error("nvdaControllerClient.dll is missing functions, screen reader output is off");
        FreeLibrary(s_module);
        s_module = nullptr;
        return;
    }

    Info("NVDA controller client loaded; speech goes to NVDA whenever it is running");
}

void Unload()
{
    if (s_module)
        FreeLibrary(s_module);
    s_module         = nullptr;
    s_testIfRunning  = nullptr;
    s_speakText      = nullptr;
    s_cancelSpeech   = nullptr;
    s_brailleMessage = nullptr;
    s_loadTried      = false;
    s_wasRunning     = false;
}

bool Speak(const std::wstring& aText)
{
    if (!s_module)
        return false;

    // Asked every time: NVDA can be started or quit mid-game.
    const bool running = s_testIfRunning() == 0;
    if (running != s_wasRunning)
    {
        s_wasRunning = running;
        Info(running ? "NVDA is running, speaking through it" : "NVDA is not running, speaking through the Windows voice");
    }
    if (!running)
        return false;

    // Cancel first so moving quickly through a list cuts the previous item
    // off, the same as the Windows voice's purge.
    s_cancelSpeech();
    if (aText.empty())
        return true;

    if (s_speakText(aText.c_str()) != 0)
    {
        Error("NVDA refused the text, speaking through the Windows voice");
        s_wasRunning = false;
        return false;
    }
    if (s_brailleMessage)
        s_brailleMessage(aText.c_str());
    return true;
}
} // namespace freetts::reader

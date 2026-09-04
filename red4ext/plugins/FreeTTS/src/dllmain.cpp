#include <RED4ext/RED4ext.hpp>

#include <string>

#include "speaker.h"

namespace
{
const RED4ext::v1::Sdk*   s_sdk    = nullptr;
RED4ext::v1::PluginHandle s_handle = nullptr;

void LogInfo(const char* aMessage)
{
    if (s_sdk)
        s_sdk->logger->Info(s_handle, aMessage);
}

void LogError(const char* aMessage)
{
    if (s_sdk)
        s_sdk->logger->Error(s_handle, aMessage);
}
} // namespace

// Script side: `public native func FreeTTS_Speak(text: String, rate: Int32) -> Bool;`
// rate is SAPI's -10..10 scale. Returns false when no voice is available, so
// the script can log instead of speaking into the void.
void FreeTTS_Speak(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    RED4ext::CString text;
    int32_t          rate = 0;
    RED4ext::GetParameter(aFrame, &text);
    RED4ext::GetParameter(aFrame, &rate);
    aFrame->code++; // skip ParamEnd - omitting this corrupts the script VM

    const std::string utf8(text.c_str(), text.Length());
    const bool        ok = freetts::speaker::Say(utf8, rate);

    if (s_sdk)
    {
        if (ok)
            s_sdk->logger->InfoF(s_handle, "speaking (rate %d): %s", rate, utf8.c_str());
        else
            s_sdk->logger->WarnF(s_handle, "no voice, dropped: %s", utf8.c_str());
    }

    if (aOut)
        *aOut = ok;
}

// Script side: `public native func FreeTTS_IsReady() -> Bool;`
void FreeTTS_IsReady(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    aFrame->code++; // skip ParamEnd - no parameters, but the marker is still there

    if (aOut)
        *aOut = freetts::speaker::IsReady();
}

void PostRegisterTypes()
{
    auto* rtti = RED4ext::CRTTISystem::Get();

    auto* speak = RED4ext::CGlobalFunction::Create("FreeTTS_Speak", "FreeTTS_Speak", &FreeTTS_Speak);
    speak->flags = {.isNative = true, .isStatic = true};
    speak->AddParam("String", "text");
    speak->AddParam("Int32", "rate");
    speak->SetReturnType("Bool");
    rtti->RegisterFunction(speak);

    auto* ready = RED4ext::CGlobalFunction::Create("FreeTTS_IsReady", "FreeTTS_IsReady", &FreeTTS_IsReady);
    ready->flags = {.isNative = true, .isStatic = true};
    ready->SetReturnType("Bool");
    rtti->RegisterFunction(ready);
}

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    switch (aReason)
    {
    case RED4ext::v1::EMainReason::Load:
        s_sdk    = aSdk;
        s_handle = aHandle;
        aSdk->logger->Info(aHandle, "FreeTTS loaded");
        RED4ext::CRTTISystem::Get()->AddPostRegisterCallback(PostRegisterTypes);
        freetts::speaker::Start(&LogInfo, &LogError);
        break;

    case RED4ext::v1::EMainReason::Unload:
        freetts::speaker::Stop();
        break;
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name    = L"FreeTTS";
    aInfo->author  = L"ringo";
    aInfo->version = RED4EXT_V1_SEMVER(0, 2, 0);
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_2_31;
    aInfo->sdk     = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}

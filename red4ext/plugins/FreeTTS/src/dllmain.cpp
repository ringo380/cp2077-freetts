#include <RED4ext/RED4ext.hpp>

#include <string>

#include "spatial.h"
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

// Script side: `public native func FreeTTS_Speak(text: String, rate: Int32, voice: Int32) -> Bool;`
// rate is SAPI's -10..10 scale; voice is 0 for the Windows default or a 1-based
// position in the voice list the plugin logs at start. Returns false when no
// voice is available, so the script can log instead of speaking into the void.
void FreeTTS_Speak(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    RED4ext::CString text;
    int32_t          rate  = 0;
    int32_t          voice = 0;
    RED4ext::GetParameter(aFrame, &text);
    RED4ext::GetParameter(aFrame, &rate);
    RED4ext::GetParameter(aFrame, &voice);
    aFrame->code++; // skip ParamEnd - omitting this corrupts the script VM

    const std::string utf8(text.c_str(), text.Length());
    const bool        ok = freetts::speaker::Say(utf8, rate, voice);

    if (s_sdk)
    {
        if (ok)
            s_sdk->logger->InfoF(s_handle, "speaking (rate %d, voice %d): %s", rate, voice, utf8.c_str());
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

// Script side: `public native func FreeTTS_VoiceName(voice: Int32) -> String;`
// The logged name of that voice (0 = the Windows default), "" when unknown.
void FreeTTS_VoiceName(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, RED4ext::CString* aOut,
                       int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    int32_t voice = 0;
    RED4ext::GetParameter(aFrame, &voice);
    aFrame->code++; // skip ParamEnd

    if (aOut)
        *aOut = RED4ext::CString(freetts::speaker::VoiceName(voice));
}

// Script side: `public native func FreeTTS_SelectVoice(voice: Int32) -> Bool;`
// Remembers that voice by name (0 forgets) so it survives a relaunch and
// Windows reordering the list. False when the list has no such voice.
void FreeTTS_SelectVoice(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    int32_t voice = 0;
    RED4ext::GetParameter(aFrame, &voice);
    aFrame->code++; // skip ParamEnd

    const bool ok = freetts::speaker::SelectVoice(voice);
    if (aOut)
        *aOut = ok;
}

// Script side: `public native func FreeTTS_SavedVoice() -> Int32;`
// The remembered voice's position in this session's list, 0 for the default.
void FreeTTS_SavedVoice(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    aFrame->code++; // skip ParamEnd

    if (aOut)
        *aOut = freetts::speaker::SavedVoice();
}

// Script side: `public native func FreeTTS_PlayAt(path: String, x: Float, y: Float, z: Float) -> Int32;`
// Plays a PCM 16-bit mono WAV from world position (x, y, z), panned and
// faded against the listener. Handle > 0, or 0 when the audio engine failed.
void FreeTTS_PlayAt(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, int32_t* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    RED4ext::CString path;
    float            x = 0.0f;
    float            y = 0.0f;
    float            z = 0.0f;
    RED4ext::GetParameter(aFrame, &path);
    RED4ext::GetParameter(aFrame, &x);
    RED4ext::GetParameter(aFrame, &y);
    RED4ext::GetParameter(aFrame, &z);
    aFrame->code++; // skip ParamEnd

    const int handle = freetts::spatial::PlayAt(std::string(path.c_str(), path.Length()), x, y, z);
    if (aOut)
        *aOut = handle;
}

// Script side: `public native func FreeTTS_SetListener(px: Float, py: Float, pz: Float,
//   fx: Float, fy: Float, fz: Float, ux: Float, uy: Float, uz: Float) -> Void;`
void FreeTTS_SetListener(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);

    float v[9] = {};
    for (float& f : v)
        RED4ext::GetParameter(aFrame, &f);
    aFrame->code++; // skip ParamEnd

    freetts::spatial::SetListener(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);
}

// Script side: `public native func FreeTTS_IsPlaying(handle: Int32) -> Bool;`
void FreeTTS_IsPlaying(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, bool* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(a4);

    int32_t handle = 0;
    RED4ext::GetParameter(aFrame, &handle);
    aFrame->code++; // skip ParamEnd

    if (aOut)
        *aOut = freetts::spatial::IsPlaying(handle);
}

// Script side: `public native func FreeTTS_StopSound(handle: Int32) -> Void;`
void FreeTTS_StopSound(RED4ext::IScriptable* aContext, RED4ext::CStackFrame* aFrame, void* aOut, int64_t a4)
{
    RED4EXT_UNUSED_PARAMETER(aContext);
    RED4EXT_UNUSED_PARAMETER(aOut);
    RED4EXT_UNUSED_PARAMETER(a4);

    int32_t handle = 0;
    RED4ext::GetParameter(aFrame, &handle);
    aFrame->code++; // skip ParamEnd

    freetts::spatial::StopSound(handle);
}

void PostRegisterTypes()
{
    auto* rtti = RED4ext::CRTTISystem::Get();

    auto* speak = RED4ext::CGlobalFunction::Create("FreeTTS_Speak", "FreeTTS_Speak", &FreeTTS_Speak);
    speak->flags = {.isNative = true, .isStatic = true};
    speak->AddParam("String", "text");
    speak->AddParam("Int32", "rate");
    speak->AddParam("Int32", "voice");
    speak->SetReturnType("Bool");
    rtti->RegisterFunction(speak);

    auto* ready = RED4ext::CGlobalFunction::Create("FreeTTS_IsReady", "FreeTTS_IsReady", &FreeTTS_IsReady);
    ready->flags = {.isNative = true, .isStatic = true};
    ready->SetReturnType("Bool");
    rtti->RegisterFunction(ready);

    auto* name = RED4ext::CGlobalFunction::Create("FreeTTS_VoiceName", "FreeTTS_VoiceName", &FreeTTS_VoiceName);
    name->flags = {.isNative = true, .isStatic = true};
    name->AddParam("Int32", "voice");
    name->SetReturnType("String");
    rtti->RegisterFunction(name);

    auto* select = RED4ext::CGlobalFunction::Create("FreeTTS_SelectVoice", "FreeTTS_SelectVoice", &FreeTTS_SelectVoice);
    select->flags = {.isNative = true, .isStatic = true};
    select->AddParam("Int32", "voice");
    select->SetReturnType("Bool");
    rtti->RegisterFunction(select);

    auto* saved = RED4ext::CGlobalFunction::Create("FreeTTS_SavedVoice", "FreeTTS_SavedVoice", &FreeTTS_SavedVoice);
    saved->flags = {.isNative = true, .isStatic = true};
    saved->SetReturnType("Int32");
    rtti->RegisterFunction(saved);

    auto* playAt = RED4ext::CGlobalFunction::Create("FreeTTS_PlayAt", "FreeTTS_PlayAt", &FreeTTS_PlayAt);
    playAt->flags = {.isNative = true, .isStatic = true};
    playAt->AddParam("String", "path");
    playAt->AddParam("Float", "x");
    playAt->AddParam("Float", "y");
    playAt->AddParam("Float", "z");
    playAt->SetReturnType("Int32");
    rtti->RegisterFunction(playAt);

    auto* listener =
        RED4ext::CGlobalFunction::Create("FreeTTS_SetListener", "FreeTTS_SetListener", &FreeTTS_SetListener);
    listener->flags = {.isNative = true, .isStatic = true};
    for (const char* name : {"px", "py", "pz", "fx", "fy", "fz", "ux", "uy", "uz"})
        listener->AddParam("Float", name);
    rtti->RegisterFunction(listener);

    auto* playing = RED4ext::CGlobalFunction::Create("FreeTTS_IsPlaying", "FreeTTS_IsPlaying", &FreeTTS_IsPlaying);
    playing->flags = {.isNative = true, .isStatic = true};
    playing->AddParam("Int32", "handle");
    playing->SetReturnType("Bool");
    rtti->RegisterFunction(playing);

    auto* stop = RED4ext::CGlobalFunction::Create("FreeTTS_StopSound", "FreeTTS_StopSound", &FreeTTS_StopSound);
    stop->flags = {.isNative = true, .isStatic = true};
    stop->AddParam("Int32", "handle");
    rtti->RegisterFunction(stop);
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
        freetts::spatial::Start(&LogInfo, &LogError);
        break;

    case RED4ext::v1::EMainReason::Unload:
        freetts::spatial::Stop();
        freetts::speaker::Stop();
        break;
    }

    return true;
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name    = L"FreeTTS";
    aInfo->author  = L"ringo";
    aInfo->version = RED4EXT_V1_SEMVER(0, 8, 0);
    // Not pinned to one game version: FreeTTS only registers script functions
    // through RTTI and hooks no game addresses, like Codeware and Mod Settings.
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT;
    aInfo->sdk     = RED4EXT_V1_SDK_VERSION_CURRENT;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1;
}

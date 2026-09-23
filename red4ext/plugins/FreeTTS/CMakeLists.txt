cmake_minimum_required(VERSION 3.21)
project(FreeTTS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  RED4ext.SDK
  GIT_REPOSITORY https://github.com/WopsS/RED4ext.SDK.git
  GIT_TAG        ad7277714ad30d6885d7050c5ba24fa0102f6920
)
FetchContent_MakeAvailable(RED4ext.SDK)

add_library(FreeTTS SHARED
  src/dllmain.cpp
  src/speaker.cpp
  src/spatial.cpp
)

# ole32 for CoInitializeEx/CoCreateInstance; SAPI itself is header-only COM.
# xaudio2 (XAudio2 2.9 + X3DAudio) ships with Windows 10 and later.
target_link_libraries(FreeTTS PRIVATE RED4ext::SDK ole32 xaudio2)
target_compile_definitions(FreeTTS PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

set_target_properties(FreeTTS PROPERTIES OUTPUT_NAME "FreeTTS" SUFFIX ".dll")

# Ship debug symbols with the optimised build. Without a PDB, a crash dump that
# names FreeTTS.dll gives only a raw offset, which is unusable for diagnosis.
# /Zi does not change codegen; /OPT:REF,ICF restores the size folding that
# linking with /DEBUG would otherwise turn off.
target_compile_options(FreeTTS PRIVATE $<$<CONFIG:Release>:/Zi>)
target_link_options(FreeTTS PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF /MAP>)

cmake_minimum_required (VERSION 3.22)

project (EfluneScaleShifter VERSION 1.0.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# JUCE: -DJUCE_PATH=/path/to/JUCE でローカルの JUCE を使うか、自動でダウンロード
set (JUCE_PATH "" CACHE PATH "Path to a local JUCE checkout (optional)")
if (JUCE_PATH)
    add_subdirectory (${JUCE_PATH} JUCE)
else()
    include (FetchContent)
    FetchContent_Declare (JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable (JUCE)
endif()

juce_add_plugin (EfluneScaleShifter
    COMPANY_NAME                "Eflune"
    PRODUCT_NAME                "Eflune Scale Shifter"
    PLUGIN_MANUFACTURER_CODE    Eflu
    PLUGIN_CODE                 EfSs
    FORMATS                     VST3 Standalone
    IS_SYNTH                    TRUE        # Cubase ではインストゥルメントとして読み込み、MIDI出力を他のトラックへ送る
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Tools
    BUNDLE_ID                   com.eflune.scaleshifter)

target_sources (EfluneScaleShifter PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/MidiFileConverter.cpp)

juce_generate_juce_header (EfluneScaleShifter)

target_compile_definitions (EfluneScaleShifter PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

if (MSVC)
    target_compile_options (EfluneScaleShifter PRIVATE /utf-8)
endif()

target_link_libraries (EfluneScaleShifter
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# 変換エンジンの単体テスト（JUCE 不要）
option (EFLUNE_BUILD_TESTS "Build ScaleEngine unit tests" ON)
if (EFLUNE_BUILD_TESTS)
    enable_testing()
    add_executable (ScaleEngineTests Tests/ScaleEngineTests.cpp)
    target_include_directories (ScaleEngineTests PRIVATE Source)
    if (MSVC)
        target_compile_options (ScaleEngineTests PRIVATE /utf-8)
    endif()
    add_test (NAME ScaleEngineTests COMMAND ScaleEngineTests)
endif()

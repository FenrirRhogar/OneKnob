cmake_minimum_required(VERSION 3.22)

project(OneKnobToRuleThemAll VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ──────────────────────────────────────────────────────────────────────────────
# JUCE framework — use the local installation two levels up
# ──────────────────────────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ──────────────────────────────────────────────────────────────────────────────
# Plugin target
# ──────────────────────────────────────────────────────────────────────────────
juce_add_plugin(OneKnobToRuleThemAll
    COMPANY_NAME             "fenrikos"
    BUNDLE_ID                "com.fenrikos.OneKnobToRuleThemAll"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Fnrk
    PLUGIN_CODE              Okrg
    FORMATS                  VST3 AU Standalone
    PRODUCT_NAME             "OneKnobToRuleThemAll"
    ICON_BIG                 "Assets/Knob.png"
    ICON_SMALL               "Assets/Knob.png"
    COPY_PLUGIN_AFTER_BUILD  FALSE
)

juce_generate_juce_header(OneKnobToRuleThemAll)

# ──────────────────────────────────────────────────────────────────────────────
# Source files
# ──────────────────────────────────────────────────────────────────────────────
target_sources(OneKnobToRuleThemAll PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/OneKnobToRuleThemAllDSP.cpp
)

target_include_directories(OneKnobToRuleThemAll
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ──────────────────────────────────────────────────────────────────────────────
# Compile definitions
# ──────────────────────────────────────────────────────────────────────────────
target_compile_definitions(OneKnobToRuleThemAll
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# ──────────────────────────────────────────────────────────────────────────────
# Assets
# ──────────────────────────────────────────────────────────────────────────────
juce_add_binary_data(OneKnobToRuleThemAll_Assets
    HEADER_NAME "BinaryData.h"
    NAMESPACE BinaryData
    SOURCES
        Assets/Background.png
        Assets/Knob.png
)

# ──────────────────────────────────────────────────────────────────────────────
# Linked JUCE modules — includes juce_dsp and juce_audio_utils as required
# ──────────────────────────────────────────────────────────────────────────────
target_link_libraries(OneKnobToRuleThemAll
    PRIVATE
        OneKnobToRuleThemAll_Assets
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

cmake_minimum_required(VERSION 3.22)

project(OneKnob VERSION 1.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ONEKNOB_LTO "Link-time optimisation (slow links; turn off for development)" ON)
set(JUCE_DIR "" CACHE PATH "Local JUCE 8 checkout; fetched from GitHub when empty")

# ──────────────────────────────────────────────────────────────────────────────
# JUCE framework: a local checkout if given, otherwise the pinned release
# ──────────────────────────────────────────────────────────────────────────────
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.13
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# VST3 and Standalone everywhere; AU for Logic/GarageBand on macOS; LV2 on Linux.
if(APPLE)
    set(ONEKNOB_FORMATS VST3 AU Standalone)
elseif(WIN32)
    set(ONEKNOB_FORMATS VST3 Standalone)
else()
    set(ONEKNOB_FORMATS VST3 LV2 Standalone)
endif()

# ──────────────────────────────────────────────────────────────────────────────
# Plugin target
# ──────────────────────────────────────────────────────────────────────────────
juce_add_plugin(OneKnob
    COMPANY_NAME             "fenrikos"
    BUNDLE_ID                "com.fenrikos.OneKnob"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Fnrk
    PLUGIN_CODE              Okrg
    FORMATS                  ${ONEKNOB_FORMATS}
    LV2URI                   "https://github.com/FenrirRhogar/OneKnob"
    PRODUCT_NAME             "One Knob"
    ICON_BIG                 "Assets/Knob.png"
    ICON_SMALL               "Assets/Knob.png"
    COPY_PLUGIN_AFTER_BUILD  FALSE
)

juce_generate_juce_header(OneKnob)

# ──────────────────────────────────────────────────────────────────────────────
# Source files
# ──────────────────────────────────────────────────────────────────────────────
target_sources(OneKnob PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/OneKnobDSP.cpp
)

target_include_directories(OneKnob
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ──────────────────────────────────────────────────────────────────────────────
# Compile definitions
# ──────────────────────────────────────────────────────────────────────────────
target_compile_definitions(OneKnob
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_JACK=1                           # Standalone on Linux: JACK / PipeWire-JACK for low latency (loaded at run time)
        JUCE_USE_CURL=1                       # update check (UpdateCheck.h); on Linux libcurl is
        JUCE_LOAD_CURL_SYMBOLS_LAZILY=1       # loaded at run time, so it is not a hard dependency
        FENRIKOS_UPDATE_REPO="OneKnob"
        FENRIKOS_UPDATE_NAME="One Knob"
        FENRIKOS_PRODUCT_URL="https://fenrikos.gumroad.com/l/oneknob"
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# ──────────────────────────────────────────────────────────────────────────────
# Assets
# ──────────────────────────────────────────────────────────────────────────────
juce_add_binary_data(OneKnob_Assets
    HEADER_NAME "BinaryData.h"
    NAMESPACE BinaryData
    SOURCES
        Assets/Background.png
        Assets/Knob.png
)

# ──────────────────────────────────────────────────────────────────────────────
# Linked JUCE modules — includes juce_dsp and juce_audio_utils as required
# ──────────────────────────────────────────────────────────────────────────────
target_link_libraries(OneKnob
    PRIVATE
        OneKnob_Assets
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
        juce::juce_recommended_warning_flags
)

if(ONEKNOB_LTO)
    target_link_libraries(OneKnob PUBLIC juce::juce_recommended_lto_flags)
endif()

# ──────────────────────────────────────────────────────────────────────────────
# Unit tests (run build/OneKnobTests, or build/Release/OneKnobTests.exe on Windows)
# ──────────────────────────────────────────────────────────────────────────────
option(ONEKNOB_TESTS "Build the unit tests" ON)
if(ONEKNOB_TESTS)
    add_executable(OneKnobTests tests/ProcessorTests.cpp)
    target_include_directories(OneKnobTests PRIVATE $<TARGET_PROPERTY:OneKnob,INCLUDE_DIRECTORIES>)
    target_compile_definitions(OneKnobTests PRIVATE $<TARGET_PROPERTY:OneKnob,COMPILE_DEFINITIONS>)
    target_link_libraries(OneKnobTests PRIVATE OneKnob)
endif()

# ──────────────────────────────────────────────────────────────────────────────
# Optional: OneKnob_calibrate measures the level change per mode and prints the trim table
# for Source/LevelCompensation.h
# ──────────────────────────────────────────────────────────────────────────────
option(ONEKNOB_CALIBRATE "Build the level calibration tool" OFF)
if(ONEKNOB_CALIBRATE)
    add_executable(OneKnob_calibrate tools/CalibrateLevel.cpp)
    target_include_directories(OneKnob_calibrate PRIVATE $<TARGET_PROPERTY:OneKnob,INCLUDE_DIRECTORIES>)
    target_compile_definitions(OneKnob_calibrate PRIVATE $<TARGET_PROPERTY:OneKnob,COMPILE_DEFINITIONS>)
    target_link_libraries(OneKnob_calibrate PRIVATE OneKnob)
endif()

# ──────────────────────────────────────────────────────────────────────────────
# Optional: OneKnob_snapshot renders the editor to PNG (store artwork, visual checks)
# ──────────────────────────────────────────────────────────────────────────────
option(ONEKNOB_SNAPSHOT "Build the editor snapshot tool" OFF)
if(ONEKNOB_SNAPSHOT)
    add_executable(OneKnob_snapshot tools/Snapshot.cpp)
    target_include_directories(OneKnob_snapshot PRIVATE $<TARGET_PROPERTY:OneKnob,INCLUDE_DIRECTORIES>)
    target_compile_definitions(OneKnob_snapshot PRIVATE $<TARGET_PROPERTY:OneKnob,COMPILE_DEFINITIONS>)
    target_link_libraries(OneKnob_snapshot PRIVATE OneKnob)
endif()

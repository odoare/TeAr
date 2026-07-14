cmake_minimum_required(VERSION 3.22)

project(TeAr VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
endif()

# JUCE is expected as a sibling directory: ../JUCE
add_subdirectory(../JUCE JUCE)

# FX-Mechanics shared tools (JUCE module), vendored as a git submodule.
# Registers the `FxmeTools` module target.
include(${CMAKE_CURRENT_SOURCE_DIR}/Source/libs/FxmeTools/cmake/FxmeTools.cmake)

juce_add_plugin(TeAr
    COMPANY_NAME                "FX-Mechanics"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 TEAR
    FORMATS                     VST3 AU
    PRODUCT_NAME                "TeAr"
    DESCRIPTION                 "The Text Arpeggiator"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES             "Instrument|Synth"
    BUNDLE_ID                   "com.fx-mechanics.TeAr"
)

juce_add_binary_data(TeArBinaryData SOURCES
    Source/assets/logo686.png
)

target_sources(TeAr PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ArpeggiatorComponent.cpp
    Source/popupWindow.cpp
    Source/FxmeLogo.cpp
    Source/ScaleComponent.cpp
    Source/KeyboardComponent.cpp
)

target_include_directories(TeAr PRIVATE Source)

juce_generate_juce_header(TeAr)

target_compile_definitions(TeAr
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(TeAr
    PRIVATE
        TeArBinaryData
        FxmeTools
        juce_audio_basics
        juce_audio_devices
        juce_audio_formats
        juce_audio_plugin_client
        juce_audio_processors
        juce_audio_processors_headless
        juce_audio_utils
        juce_core
        juce_data_structures
        juce_events
        juce_graphics
        juce_gui_basics
        juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Unit tests for the FxmeTools arpeggiator engine (opt-in, off by default)
# Build with: cmake -DTEAR_BUILD_TESTS=ON ...
# Run with:   Source/libs/FxmeTools/tests/run_tests.sh
# ---------------------------------------------------------------------------
option(TEAR_BUILD_TESTS "Build FxmeTools unit tests" OFF)
if(TEAR_BUILD_TESTS)
    enable_testing()  # must be in the top-level CMakeLists for CTest to discover tests
    add_subdirectory(Source/libs/FxmeTools/tests)
endif()

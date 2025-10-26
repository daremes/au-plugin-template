cmake_minimum_required(VERSION 3.15)

project(CosmicGlitch VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch JUCE when configuring the project. Feel free to point this to a local
# checkout if you already have JUCE on disk.
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(
    CosmicGlitch
    COMPANY_NAME "Outer Echo"
    PLUGIN_MANUFACTURER_CODE CgMf
    PLUGIN_CODE Cg02
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Cosmic Glitch"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(CosmicGlitch
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/GrainEngine.cpp
        Source/GrainEngine.h
        Source/GlitchLookAndFeel.h
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/GrainEngine.cpp
    Source/GrainEngine.h
    Source/GlitchLookAndFeel.h
)

target_compile_definitions(CosmicGlitch
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(CosmicGlitch
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
)

if(MSVC)
    target_compile_options(CosmicGlitch PRIVATE /W4 /WX-)
else()
    target_compile_options(CosmicGlitch PRIVATE -Wall -Wextra -Wpedantic)
endif()


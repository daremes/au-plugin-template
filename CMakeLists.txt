cmake_minimum_required(VERSION 3.15)

project(CosmicGrainDelay VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow the user to point to a JUCE checkout via JUCE_DIR
if (NOT DEFINED JUCE_DIR)
    set(JUCE_DIR "${CMAKE_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE" FORCE)
endif()

if (NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Could not find JUCE. Please set JUCE_DIR to a valid JUCE checkout.")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(CosmicGrainDelay
    COMPANY_NAME "Experimental Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Fx"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Cosmic Grain Delay")

juce_generate_juce_header(CosmicGrainDelay)

target_sources(CosmicGrainDelay
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/GrainEngine.cpp
        Source/GrainEngine.h)

target_compile_definitions(CosmicGrainDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

juce_add_binary_data(CosmicGrainDelayData SOURCES)

set_target_properties(CosmicGrainDelay PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

juce_enable_vst3_category(CosmicGrainDelay "Fx")

target_link_libraries(CosmicGrainDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

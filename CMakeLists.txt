cmake_minimum_required(VERSION 3.15)

project(CosmicGrainDelay VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow the user to point to a JUCE checkout via JUCE_DIR or fetch it automatically.
if (DEFINED JUCE_DIR AND NOT JUCE_DIR STREQUAL "")
    # Resolve the path relative to the binary directory to match typical cmake usage
    get_filename_component(JUCE_DIR "${JUCE_DIR}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

    if (NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "JUCE_DIR ('${JUCE_DIR}') does not contain a JUCE checkout.\n"
            "Please provide a path that contains JUCE's CMakeLists.txt or omit JUCE_DIR to download JUCE automatically.")
    endif()

    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.9)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(CosmicGrainDelay
    COMPANY_NAME "Experimental Audio"
    BUNDLE_ID "com.ExperimentalAudio.CosmicGrainDelay"
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

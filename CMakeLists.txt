cmake_minimum_required(VERSION 3.15)

if (APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    # macOS 15 removes several CoreGraphics capture APIs that JUCE still relies on.
    # Setting an explicit deployment target before declaring the project ensures
    # those symbols remain available when building with the macOS 15 SDK while
    # still allowing users to override it.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")
endif()

project(CosmicGrainDelay VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COSMIC_COPY_PLUGIN_AFTER_BUILD
    "Copy built plug-ins into the system plug-in folders after each build"
    OFF)

# Allow the user to point to a JUCE checkout via JUCE_DIR or fetch it automatically.
if (APPLE)
    # Force ScreenCaptureKit usage on macOS 15 SDKs where the legacy
    # CoreGraphics screen-grab APIs have been removed. JUCE exposes both a
    # cache-variable toggle and a preprocessor define for this behaviour, and
    # juceaide needs the same definitions to build successfully.
    set(JUCE_USE_SCREEN_CAPTURE_KIT ON CACHE BOOL
        "Use ScreenCaptureKit instead of deprecated CoreGraphics APIs" FORCE)
    set(JUCE_MAC_SCREEN_CAPTURE_USE_SCREEN_CAPTURE_KIT ON CACHE BOOL
        "Use ScreenCaptureKit instead of deprecated CoreGraphics APIs" FORCE)
    add_compile_definitions(JUCE_USE_SCREEN_CAPTURE_KIT=1)
endif()

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

if (APPLE)
    # juceaide is built from the JUCE tree and does not automatically inherit
    # compile definitions declared before the JUCE subdirectory is added.
    # Explicitly push the ScreenCaptureKit toggles onto juceaide and the
    # relevant JUCE module targets so they avoid the removed CoreGraphics APIs
    # when compiling with the macOS 15 SDK.
    foreach(screen_capture_target IN ITEMS
        juceaide
        juce::juce_gui_basics
        juce::juce_graphics)
        if (TARGET ${screen_capture_target})
            # Resolve aliases/imported targets before applying the definitions.
            set(actual_target "${screen_capture_target}")
            get_target_property(alias_target "${screen_capture_target}" ALIASED_TARGET)
            if (alias_target)
                set(actual_target "${alias_target}")
            endif()
            get_target_property(is_imported "${actual_target}" IMPORTED)
            if (NOT is_imported)
                get_target_property(target_type "${actual_target}" TYPE)
                if (target_type STREQUAL "INTERFACE_LIBRARY")
                    target_compile_definitions("${actual_target}" INTERFACE
                        JUCE_USE_SCREEN_CAPTURE_KIT=1
                        JUCE_MAC_SCREEN_CAPTURE_USE_SCREEN_CAPTURE_KIT=1)
                else()
                    target_compile_definitions("${actual_target}" PRIVATE
                        JUCE_USE_SCREEN_CAPTURE_KIT=1
                        JUCE_MAC_SCREEN_CAPTURE_USE_SCREEN_CAPTURE_KIT=1)
                endif()
            endif()
        endif()
    endforeach()
endif()

juce_add_plugin(CosmicGrainDelay
    COMPANY_NAME "Feline Astronauts"
    BUNDLE_ID "com.FelineAstronauts.CosmicScratches"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${COSMIC_COPY_PLUGIN_AFTER_BUILD}
    VST3_CATEGORIES "Fx"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Cosmic Scratches")

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

target_link_libraries(CosmicGrainDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

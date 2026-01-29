# SurgeBox - A groovebox built on Surge XT
# https://github.com/surge-synthesizer/surge
cmake_minimum_required(VERSION 3.20)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Minimum macOS version")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Runtime library")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(SurgeBox VERSION 0.1.0 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

# ============================================================================
# Surge Configuration - Build only what we need
# ============================================================================

set(SURGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/surge)

# Skip building Surge's plugins - we just need the common library
set(SURGE_BUILD_TESTRUNNER OFF CACHE BOOL "" FORCE)
set(SURGE_BUILD_FX OFF CACHE BOOL "" FORCE)
set(SURGE_BUILD_XT OFF CACHE BOOL "" FORCE)
set(SURGE_BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)
set(SURGE_COPY_AFTER_BUILD OFF CACHE BOOL "" FORCE)
set(SURGE_BUILD_CLAP OFF CACHE BOOL "" FORCE)
set(SURGE_SKIP_JUCE_FOR_RACK OFF CACHE BOOL "" FORCE)
set(CMAKE_CROSSCOMPILING TRUE CACHE BOOL "" FORCE) # Skip pluginval

# Add Surge - it will only build surge-common due to our options
add_subdirectory(libs/surge)

# ============================================================================
# SurgeBox Core Library
# ============================================================================

add_library(surgebox-core STATIC
    src/core/GrooveboxProject.cpp
    src/core/GrooveboxProject.h
    src/core/SurgeBoxEngine.cpp
    src/core/SurgeBoxEngine.h
)

target_include_directories(surgebox-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${SURGE_SOURCE_DIR}/src/common
)

# Add r8brain manually since Surge's CMake uses CMAKE_SOURCE_DIR which doesn't work as subdir
target_sources(surgebox-core PRIVATE
    ${SURGE_SOURCE_DIR}/libs/r8brain-free-src/r8bbase.cpp
)
target_include_directories(surgebox-core PRIVATE
    ${SURGE_SOURCE_DIR}/libs/r8brain-free-src
)

target_link_libraries(surgebox-core PUBLIC
    surge::surge-common
)

# ============================================================================
# SurgeBox Plugin
# ============================================================================

juce_add_plugin(surgebox
    PRODUCT_NAME "SurgeBox"
    COMPANY_NAME "Surge Synth Team"
    BUNDLE_ID "org.surge-synth-team.surgebox"
    PLUGIN_MANUFACTURER_CODE SrgB
    PLUGIN_CODE SBox

    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    FORMATS VST3 AU Standalone

    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice

    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(surgebox PRIVATE
    src/plugin/SurgeBoxProcessor.cpp
    src/plugin/SurgeBoxProcessor.h
    src/plugin/SurgeBoxEditor.cpp
    src/plugin/SurgeBoxEditor.h
    src/gui/widgets/PianoRollWidget.cpp
    src/gui/widgets/PianoRollWidget.h
    src/gui/widgets/VoiceSelector.cpp
    src/gui/widgets/VoiceSelector.h
    src/gui/widgets/TransportControls.cpp
    src/gui/widgets/TransportControls.h
)

target_include_directories(surgebox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SURGE_SOURCE_DIR}/src/common
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui
    ${SURGE_SOURCE_DIR}/src/surge-xt
)

target_compile_definitions(surgebox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

target_link_libraries(surgebox PRIVATE
    surgebox-core
    surge::surge-common
    juce::juce_audio_utils
    juce::juce_audio_processors
)

# ============================================================================
# Convenience target
# ============================================================================

add_custom_target(surgebox-all)
add_dependencies(surgebox-all surgebox_VST3 surgebox_Standalone)

if(APPLE)
    add_dependencies(surgebox-all surgebox_AU)
endif()

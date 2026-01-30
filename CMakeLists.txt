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

# Build surge-xt to get access to the GUI components
set(SURGE_BUILD_TESTRUNNER OFF CACHE BOOL "" FORCE)
set(SURGE_BUILD_FX OFF CACHE BOOL "" FORCE)
set(SURGE_BUILD_XT ON CACHE BOOL "" FORCE)  # Enable to get GUI
set(SURGE_BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)
set(SURGE_COPY_AFTER_BUILD OFF CACHE BOOL "" FORCE)
set(SURGE_BUILD_CLAP OFF CACHE BOOL "" FORCE)
set(SURGE_SKIP_JUCE_FOR_RACK OFF CACHE BOOL "" FORCE)
set(CMAKE_CROSSCOMPILING TRUE CACHE BOOL "" FORCE) # Skip pluginval
set(SURGE_JUCE_FORMATS "Standalone" CACHE STRING "" FORCE) # Only build standalone for surge-xt

# Add Surge - it will only build surge-common due to our options
add_subdirectory(libs/surge)

# ============================================================================
# SurgeBox Core Library
# ============================================================================

add_library(surgebox-core STATIC
    src/core/GrooveboxProject.cpp
    src/core/GrooveboxProject.h
    src/core/PatternModel.cpp
    src/core/PatternModel.h
    src/core/SurgeBoxEngine.cpp
    src/core/SurgeBoxEngine.h
)

target_include_directories(surgebox-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${SURGE_SOURCE_DIR}/src/common
    ${SURGE_SOURCE_DIR}/src/surge-xt
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui
    ${SURGE_SOURCE_DIR}/libs/sst/sst-jucegui/include
)

# Add r8brain manually since Surge's CMake uses CMAKE_SOURCE_DIR which doesn't work as subdir
target_sources(surgebox-core PRIVATE
    ${SURGE_SOURCE_DIR}/libs/r8brain-free-src/r8bbase.cpp
)
target_include_directories(surgebox-core PRIVATE
    ${SURGE_SOURCE_DIR}/libs/r8brain-free-src
)

# NOTE: We don't link surge-xt here to avoid duplicate createPluginFilter symbol
# The final plugin will link it. surgebox-core uses forward declarations.
# We do need JUCE audio basics for AudioBuffer in the engine
target_link_libraries(surgebox-core PUBLIC
    surge::surge-common
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_data_structures
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
    ${SURGE_SOURCE_DIR}/libs/sst/sst-jucegui/include
)

target_compile_definitions(surgebox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=1
)

# Get the sources from surge-xt but not the plugin factory (createPluginFilter)
# We compile the surge-xt sources directly to avoid duplicate symbol issues
target_sources(surgebox PRIVATE
    ${SURGE_SOURCE_DIR}/src/surge-xt/SurgeSynthProcessor.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/SurgeSynthEditor.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/SurgeCLAPPresetDiscovery.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/util/LuaTokeniserSurge.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/RuntimeFont.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SkinFontLoader.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SkinSupport.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeGUIEditor.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeGUIEditorHtmlGenerators.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeGUIEditorInfowindow.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeGUIEditorOverlays.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeGUIEditorValueCallbacks.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeGUIEditorMenuStructures.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeGUIUtils.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeImage.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeImageStore.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/SurgeJUCELookAndFeel.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/UndoManager.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/AboutScreen.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/FilterAnalysis.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/KeyBindingsOverlay.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/LuaEditors.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/MSEGEditor.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/MiniEdit.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/Alert.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/OverlayUtils.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/ModulationEditor.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/Oscilloscope.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/OverlayWrapper.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/PatchDBViewer.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/PatchStoreDialog.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/TuningOverlays.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/TypeinParamEditor.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/WaveShaperAnalysis.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/overlays/OpenSoundControlSettings.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/CurrentFxDisplay.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/EffectChooser.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/LFOAndStepDisplay.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/MainFrame.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/MenuButtonFromIcon.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/MenuCustomComponents.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/MenuForDiscreteParams.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/ModulatableSlider.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/ModulationSourceButton.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/MultiSwitch.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/NumberField.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/OscillatorWaveformDisplay.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/ParameterInfowindow.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/PatchSelector.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/Switch.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/SurgeTextButton.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/TypeAheadTextEditor.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/VerticalLabel.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/VuMeter.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/WaveShaperSelector.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/gui/widgets/XMLConfiguredMenus.cpp
    ${SURGE_SOURCE_DIR}/src/surge-xt/osc/OpenSoundControl.cpp
)

# Rename Surge's createPluginFilter to avoid conflict with ours
# We use preprocessor to rename it during compilation
set_source_files_properties(
    ${SURGE_SOURCE_DIR}/src/surge-xt/SurgeSynthProcessor.cpp
    PROPERTIES COMPILE_DEFINITIONS "createPluginFilter=_surge_unused_createPluginFilter"
)

target_link_libraries(surgebox PRIVATE
    surgebox-core
    surge::surge-common
    surge-platform
    surge-juce
    surge-xt-binary  # Binary resources
    sst-filters-extras
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_osc
    juce::juce_dsp
)

# ============================================================================
# Convenience target
# ============================================================================

add_custom_target(surgebox-all)
add_dependencies(surgebox-all surgebox_VST3 surgebox_Standalone)

if(APPLE)
    add_dependencies(surgebox-all surgebox_AU)
endif()

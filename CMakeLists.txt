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
# Dexed Configuration - Compiled as object library with isolated include paths
# to avoid conflicts between Dexed's and Surge's tuning-library headers
# ============================================================================

set(DEXED_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/dexed)

# Dexed binary resources (UI assets, built-in presets)
juce_add_binary_data(DexedResources SOURCES
    ${DEXED_SOURCE_DIR}/assets/ui/ButtonUnlabeled_50x30.png
    ${DEXED_SOURCE_DIR}/assets/ui/dexedIcon.png
    ${DEXED_SOURCE_DIR}/assets/ui/dexed-logo.png
    ${DEXED_SOURCE_DIR}/assets/ui/GlobalEditor_1728x288.png
    ${DEXED_SOURCE_DIR}/assets/ui/HelpButton.png
    ${DEXED_SOURCE_DIR}/assets/ui/HelpButton.svg
    ${DEXED_SOURCE_DIR}/assets/ui/Knob_68x68.png
    ${DEXED_SOURCE_DIR}/assets/ui/LFO_36_26.png
    ${DEXED_SOURCE_DIR}/assets/ui/Light_28x28.png
    ${DEXED_SOURCE_DIR}/assets/ui/Meter_140x8.png
    ${DEXED_SOURCE_DIR}/assets/ui/NotoSans-Bold.ttf
    ${DEXED_SOURCE_DIR}/assets/ui/NotoSans-Regular.ttf
    ${DEXED_SOURCE_DIR}/assets/ui/OperatorEditor_574x436.png
    ${DEXED_SOURCE_DIR}/assets/ui/Scaling_36_26.png
    ${DEXED_SOURCE_DIR}/assets/ui/Slider_52x52.png
    ${DEXED_SOURCE_DIR}/assets/ui/Switch_64x64.png
    ${DEXED_SOURCE_DIR}/assets/ui/Switch_96x52.png
    ${DEXED_SOURCE_DIR}/assets/ui/SwitchLighted_48x26.png
    ${DEXED_SOURCE_DIR}/assets/builtin_pgm.zip
)

# Dexed object library - isolated include paths prevent tuning-library header conflicts
add_library(dexed-sources OBJECT
    # Audio/synth
    ${DEXED_SOURCE_DIR}/Source/PluginProcessor.cpp
    ${DEXED_SOURCE_DIR}/Source/PluginData.cpp
    ${DEXED_SOURCE_DIR}/Source/PluginParam.cpp
    ${DEXED_SOURCE_DIR}/Source/PluginFx.cpp
    ${DEXED_SOURCE_DIR}/Source/EngineMkI.cpp
    ${DEXED_SOURCE_DIR}/Source/EngineOpl.cpp
    ${DEXED_SOURCE_DIR}/Source/SysexComm.cpp
    # UI
    ${DEXED_SOURCE_DIR}/Source/PluginEditor.cpp
    ${DEXED_SOURCE_DIR}/Source/GlobalEditor.cpp
    ${DEXED_SOURCE_DIR}/Source/OperatorEditor.cpp
    ${DEXED_SOURCE_DIR}/Source/ParamDialog.cpp
    ${DEXED_SOURCE_DIR}/Source/AlgoDisplay.cpp
    ${DEXED_SOURCE_DIR}/Source/DXComponents.cpp
    ${DEXED_SOURCE_DIR}/Source/DXLookNFeel.cpp
    ${DEXED_SOURCE_DIR}/Source/ProgramListBox.cpp
    ${DEXED_SOURCE_DIR}/Source/CartManager.cpp
    ${DEXED_SOURCE_DIR}/Source/VUMeter.cpp
    ${DEXED_SOURCE_DIR}/Source/TuningShow.cpp
    # MSFA engine
    ${DEXED_SOURCE_DIR}/Source/msfa/dx7note.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/env.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/exp2.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/fm_core.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/fm_op_kernel.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/freqlut.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/lfo.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/pitchenv.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/sin.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/tuning.cc
    ${DEXED_SOURCE_DIR}/Source/msfa/porta.cpp
    # MTS-ESP client is provided by Surge's oddsound-mts target, so we skip it here
    # Surge synth team tuning UI
    ${DEXED_SOURCE_DIR}/libs/surgesynthteam_tuningui/surgesynthteam_tuningui.cpp
    # CLAP-JUCE extensions (provides clap_properties constructor)
    ${DEXED_SOURCE_DIR}/libs/clap-juce-extensions/src/extensions/clap-juce-extensions.cpp
    # Factory function for SurgeBox integration
    ${DEXED_SOURCE_DIR}/Source/DexedFactory.cpp
)

target_include_directories(dexed-sources PRIVATE
    ${DEXED_SOURCE_DIR}/Source
    ${DEXED_SOURCE_DIR}/Source/msfa
    ${DEXED_SOURCE_DIR}/JuceLibraryCode  # For #include "JuceHeader.h" (without path prefix)
    ${DEXED_SOURCE_DIR}/libs/MTS-ESP/Client
    ${DEXED_SOURCE_DIR}/libs/surgesynthteam_tuningui
    ${DEXED_SOURCE_DIR}/libs/tuning-library/include
    # CLAP extensions (wrapper, SDK headers, and helpers)
    ${DEXED_SOURCE_DIR}/libs/clap-juce-extensions/include
    ${DEXED_SOURCE_DIR}/libs/clap-juce-extensions/clap-libs/clap/include
    ${DEXED_SOURCE_DIR}/libs/clap-juce-extensions/clap-libs/clap-helpers/include
    # DexedResources binary data (explicit path since object lib may not propagate)
    ${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_DexedResources/JuceLibraryCode
)

target_link_libraries(dexed-sources PRIVATE
    DexedResources
    oddsound-mts  # MTS-ESP client (shared with Surge)
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_extra
)

target_compile_definitions(dexed-sources PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JucePlugin_Name="Dexed"
)

# Rename Dexed's createPluginFilter to avoid conflict with ours
set_source_files_properties(
    ${DEXED_SOURCE_DIR}/Source/PluginProcessor.cpp
    PROPERTIES COMPILE_DEFINITIONS "createPluginFilter=_dexed_unused_createPluginFilter;DEXED_ID=SURGEBOX"
)

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
    src/core/MusicTheory.cpp
    src/core/MusicTheory.h
    src/core/TR808Processor.cpp
    src/core/TR808Processor.h
    src/core/MasterFXChain.cpp
    src/core/MasterFXChain.h
    src/core/ProjectModel.cpp
    src/core/ProjectModel.h
    src/core/PlaceholderProcessor.h
    src/core/MidiMappingEngine.cpp
    src/core/MidiMappingEngine.h
    src/core/VoicePreset.cpp
    src/core/VoicePreset.h
    src/core/VoiceProcessor.cpp
    src/core/VoiceProcessor.h
    src/core/SurgeBoxContext.h
    src/core/drums/DrumVoices.h
    src/core/drums/FastMath.h
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
    # Plugin
    src/plugin/SurgeBoxProcessor.cpp
    src/plugin/SurgeBoxProcessor.h
    src/plugin/SurgeBoxEditor.cpp
    src/plugin/SurgeBoxEditor.h
    src/plugin/CommandBar.cpp
    src/plugin/CommandBar.h
    src/plugin/MeasureControls.cpp
    src/plugin/MeasureControls.h
    # GUI - Core
    src/gui/Theme.h
    src/gui/Layout.h
    src/gui/SurgeBoxLookAndFeel.h
    # GUI - Widgets
    src/gui/widgets/MultiSwitch.cpp
    src/gui/widgets/MultiSwitch.h
    src/gui/widgets/SelfDrawButton.cpp
    src/gui/widgets/SelfDrawButton.h
    src/gui/widgets/ToggleButton.cpp
    src/gui/widgets/ToggleButton.h
    src/gui/widgets/NumberField.cpp
    src/gui/widgets/NumberField.h
    src/gui/widgets/IconButton.cpp
    src/gui/widgets/IconButton.h
    src/gui/widgets/SurgeBoxWidgets.h
    src/gui/widgets/VoiceSelector.cpp
    src/gui/widgets/VoiceSelector.h
    src/gui/widgets/TransportControls.cpp
    src/gui/widgets/TransportControls.h
    src/gui/widgets/PianoKeyboardWidget.cpp
    src/gui/widgets/PianoKeyboardWidget.h
    # GUI - TR-808 Editor
    src/gui/widgets/TR808Editor.cpp
    src/gui/widgets/TR808Editor.h
    # GUI - Master FX Editor
    src/gui/widgets/MasterFXEditor.cpp
    src/gui/widgets/MasterFXEditor.h
    # GUI - Piano Roll
    src/gui/widgets/PianoRollWidget.cpp
    src/gui/widgets/PianoRollWidget.h
    src/gui/widgets/pianoroll/PianoRollRenderer.cpp
    src/gui/widgets/pianoroll/PianoRollRenderer.h
    src/gui/widgets/pianoroll/NoteSelection.cpp
    src/gui/widgets/pianoroll/NoteSelection.h
    src/gui/widgets/pianoroll/NoteEditor.cpp
    src/gui/widgets/pianoroll/NoteEditor.h
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

# ============================================================================
# Dexed object files + include path for SurgeBoxProcessor to find PluginProcessor.h
# ============================================================================

target_sources(surgebox PRIVATE $<TARGET_OBJECTS:dexed-sources>)

# SurgeBoxProcessor.cpp uses DexedFactory.h (thin factory header, no Dexed internals)
target_include_directories(surgebox PRIVATE
    ${DEXED_SOURCE_DIR}/Source
)

target_link_libraries(surgebox PRIVATE
    surgebox-core
    surge::surge-common
    surge-platform
    surge-juce
    surge-xt-binary  # Binary resources
    sst-filters-extras
    DexedResources   # Dexed binary resources (also linked by dexed-sources, but needed for final link)
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
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

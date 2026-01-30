# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all targets (VST3, AU, Standalone)
cmake --build build --target surgebox-all

# If VST3 fails with "surgebox_vst3_helper: command not found", add build dir to PATH:
PATH="$PWD/build:$PATH" cmake --build build --target surgebox_VST3

# Build specific targets
cmake --build build --target surgebox_VST3
cmake --build build --target surgebox_AU        # macOS only
cmake --build build --target surgebox_Standalone

# Clean rebuild
cmake --build build --clean-first --target surgebox-all
```

Requires: CMake 3.20+, C++20 compiler, Git. Clone with `--recursive` to get the Surge submodule.

## Architecture

SurgeBox is a groovebox that wraps 4 instances of Surge XT synthesizer with a built-in piano roll sequencer.

### Core Layer (`src/core/`)

- **SurgeBoxEngine** - Orchestrates 4 SurgeSynthProcessor instances. Owns the GrooveboxProject, SequencerEngine, and PatternModels. Called from audio thread via `process()`.
- **SequencerEngine** - Sample-accurate MIDI playback. Populates per-voice MidiBuffers during `process()`, handles note on/off timing, loop points.
- **GrooveboxProject** - Serialization container for all state: tempo, patterns, voice patches, mixer settings. Saves to `.sbox` binary format (header + XML).
- **PatternModel** - ValueTree-backed pattern data with undo/redo support. Wraps the raw Pattern struct for UI editing.

### Plugin Layer (`src/plugin/`)

- **SurgeBoxProcessor** - JUCE AudioProcessor. Owns 4 SurgeSynthProcessor instances and the SurgeBoxEngine. Handles state save/restore.
- **SurgeBoxEditor** - Main UI. Embeds the Surge XT editor (via SurgeSynthProcessor), piano roll, voice selector, transport controls. Uses viewport scrolling for both synth and piano roll areas.

### GUI Widgets (`src/gui/widgets/`)

- **PianoRollWidget** - Vertical piano roll (time flows top-to-bottom). Supports drawing notes, box selection, step recording, mouse interaction.
- **VoiceSelector** - 4-button voice switcher with mute/solo per voice
- **TransportControls** - Play/stop/rewind, playhead position display

### Surge Integration

SurgeBox compiles Surge XT sources directly (not as a linked library) to avoid duplicate `createPluginFilter` symbols. The Surge processor's factory function is renamed via preprocessor. Each voice's SurgeSynthProcessor provides both the synth engine and the full Surge GUI.

### Key Patterns

- Audio thread calls `SurgeBoxEngine::process()` which calls `SequencerEngine::process()` to generate MIDI, then processes each voice
- Pattern edits go through PatternModel which uses JUCE's UndoManager
- Voice switching rebuilds the embedded Surge editor via `rebuildSurgeEditor()`
- Constants: `NUM_VOICES = 4`, `NUM_GLOBAL_FX = 4`

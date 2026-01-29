# SurgeBox

A groovebox built on [Surge XT](https://surge-synthesizer.github.io/) - the free and open-source hybrid synthesizer.

## Concept

SurgeBox combines four instances of the Surge synthesizer with a built-in piano roll sequencer, creating a self-contained groovebox experience:

- **4 Voices** - Each voice is a full Surge XT instance with complete synthesis capabilities
- **Piano Roll** - Draw notes directly into a looping pattern for each voice
- **Unified Transport** - One play/stop, one tempo, all voices loop together
- **Global Effects** - Shared reverb, delay, and master effects across all voices

## Building

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler
- Git

### Clone and Build

```bash
git clone --recursive https://github.com/yourusername/surgebox.git
cd surgebox
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target surgebox-all
```

### Build Targets

- `surgebox_VST3` - VST3 plugin
- `surgebox_AU` - Audio Unit (macOS only)
- `surgebox_Standalone` - Standalone application
- `surgebox-all` - Build all targets

## Project Structure

```
surgebox/
├── CMakeLists.txt           # Main build configuration
├── libs/
│   └── surge/               # Surge XT (git submodule)
├── src/
│   ├── core/                # Core engine classes
│   │   ├── GrooveboxProject.h/cpp  # Project/pattern data
│   │   └── SurgeBoxEngine.h/cpp    # Multi-instance manager
│   ├── plugin/              # JUCE plugin wrapper
│   │   ├── SurgeBoxProcessor.h/cpp
│   │   └── SurgeBoxEditor.h/cpp
│   └── gui/widgets/         # UI components
│       ├── PianoRollWidget.h/cpp
│       ├── VoiceSelector.h/cpp
│       └── TransportControls.h/cpp
└── resources/               # Assets
```

## File Format

SurgeBox projects are saved as `.sbox` files containing:
- Global settings (tempo, loop length, master volume)
- Full patch data for all 4 voices
- MIDI patterns for each voice
- Mixer settings (volume, pan, sends, mute/solo)

## License

SurgeBox is released under the GNU General Public License v3 (GPL-3.0-or-later), the same license as Surge XT.

## Credits

Built on [Surge XT](https://surge-synthesizer.github.io/) by the Surge Synth Team.

# Cosmic Grain Delay

Cosmic Grain Delay is a JUCE-based AU/VST3 granular delay and reverb effect tailored for experimental music workflows. It combines a flexible grain cloud engine, feedback delay line, and lush reverb with a space- and glitch-inspired interface for immediate inspiration.

## Features

- **Granular delay core** with adjustable grain size, density, pitch shift, stereo spread, delay time, and feedback.
- **Integrated reverb** featuring width, damping, room size, and freeze control for evolving textures.
- **Space & glitch themed UI** including animated star field, glitch scans, and custom rotary controls.
- **Parameter automation ready** via `AudioProcessorValueTreeState` and preset serialization.
- **Cross-format output** (AU, VST3, Standalone) through JUCE's CMake build system.

## Getting Started

### Prerequisites

- CMake 3.15+
- A C++17 compatible compiler (Xcode, Visual Studio, or clang/gcc toolchain).
- For AU builds, macOS with Xcode command-line tools installed.

### Configure JUCE dependency

By default the CMake project will download JUCE (tag `7.0.9`) automatically via `FetchContent`. If you already have a local JUCE checkout, pass its path through `-DJUCE_DIR=/path/to/JUCE` when invoking CMake (relative paths are resolved from the build directory).

```
git clone https://github.com/juce-framework/JUCE.git
cmake -DJUCE_DIR=/absolute/or/relative/path/to/JUCE ..
```

### Build steps

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

The resulting plug-in binaries can be found under `build/CosmicGrainDelay_artefacts`. Copy the appropriate format (e.g. `.vst3`, `.component`, or standalone app) to your plug-in folder.

## Project Structure

```
Source/
 ├── GrainEngine.*        Granular delay engine implementation
 ├── PluginProcessor.*    Audio processing, parameters, and state handling
 └── PluginEditor.*       Custom UI with space/glitch theme
CMakeLists.txt            JUCE CMake entry point
```

## Next Steps

- Integrate parameter smoothing/modulation.
- Extend the grain engine with reverse grains or random pitch jitter controls.
- Add preset browser and randomizer for fast experimentation.

Contributions, forks, and wild sonic experiments are welcome!

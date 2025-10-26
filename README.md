# Cosmic Scratches

Cosmic Scratches by Feline Astronauts is a JUCE-based AU/VST3 granular delay and reverb effect tailored for experimental music workflows. It combines a flexible grain cloud engine, feedback delay line, and lush reverb with a space- and glitch-inspired interface for immediate inspiration.

## Features

- **Granular warp core** with Nebula Size, Meteor Swarm, Orbit Shift, and Comet Spread controls for immediate texture shaping.
- **Advanced grain lab** adds Wormhole Scatter offsets, Gravity Envelope sculpting, and Quantum Drift pitch jitter for evolving motion.
- **Tempo-aware delay** that can free-run in milliseconds or snap to BPM-synchronised cosmic divisions (triplets included).
- **Meteor Burn drive stage** slots before the reverb, with tone and blend controls for optional pre-space saturation.
- **Nebula reverb suite** offering Horizon, Stellar Damping, Cosmic Width, Space Freeze, and independent Stardust/Reverb blends.
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

> **Note for macOS 15 SDK users:** The project pins the deployment target to macOS 13.0 and forces JUCE to use ScreenCaptureKit when none is supplied. The ScreenCaptureKit switches are pushed onto JUCE's cache variables **and** the juceaide/module targets directly so the helper tooling also avoids the removed CoreGraphics APIs. If you need a different target, override it at configure time via `-DCMAKE_OSX_DEPLOYMENT_TARGET=14.0` (or another supported version).

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

- Add modulation sources/LFOs for the new cosmic parameters.
- Experiment with additional grain envelope shapes or spectral shapers.
- Introduce a preset browser and snapshot morphing for live performance.

Contributions, forks, and wild sonic experiments are welcome!

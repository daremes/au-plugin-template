# Cosmic Glitch – Granular Space Delay

This repository contains a JUCE-based AU/VST3 plugin template for experimental granular delay processing with an integrated spacey reverb tail. The project is set up with modern CMake and ready for further sound design exploration.

## Features

- Granular delay engine with controllable grain size, density, spray, pitch shifting, and feedback
- Glitch parameter for reversing and stretching grains for experimental textures
- Built-in stereo widening controls and reverb section with adjustable mix, size, and damping
- Space and glitch inspired UI with animated databent streaks and neon highlights
- Configured to build AU, VST3, and Standalone targets via the JUCE framework

## Getting started

1. **Clone JUCE (optional)** – The project uses CMake's `FetchContent` to pull JUCE automatically. If you already have JUCE locally, update the `FetchContent_Declare` section in the root `CMakeLists.txt` to point to your checkout.
2. **Configure with CMake**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```
3. The built plugin binaries will appear in `build/CosmicGlitch_artefacts` after compilation.

## Project layout

```
Source/
  ├── PluginProcessor.h/.cpp   // DSP core and parameter management
  ├── PluginEditor.h/.cpp      // Custom space-glitch UI
  ├── GrainEngine.h/.cpp       // Granular delay buffer and grain scheduling
  └── GlitchLookAndFeel.h      // Neon inspired rotary sliders
```

Use this template as a starting point to explore deeper modulation, randomisation, and control surface integration for your own experimental music tools.


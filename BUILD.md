# Building TillySynth

## Prerequisites

- **CMake** 3.22 or later
- **C++17 compiler** (Xcode on macOS, MSVC on Windows)
- **Git** (JUCE is fetched automatically via CPM)

### macOS

Install Xcode Command Line Tools:

```bash
xcode-select --install
```

### Windows

Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with the "Desktop development with C++" workload.

## Configure

```bash
cmake -B build
```

For a specific build type:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

On Windows with Visual Studio (multi-config generator), the build type is selected at build time instead.

## Build

### All targets

```bash
cmake --build build --config Release
```

### Individual plugin formats

```bash
cmake --build build --config Release --target TillySynth_Standalone
cmake --build build --config Release --target TillySynth_VST3
cmake --build build --config Release --target TillySynth_AU          # macOS only
```

### Preset Review app

```bash
cmake --build build --config Release --target TillyPresetReview
```

## Output locations

After building, artefacts are placed in:

```
build/TillySynth_artefacts/<Config>/Standalone/
build/TillySynth_artefacts/<Config>/VST3/TillySynth.vst3
build/TillySynth_artefacts/<Config>/AU/TillySynth.component          # macOS only
build/TillyPresetReview_artefacts/<Config>/TillySynth Preset Review.app
```

With `COPY_PLUGIN_AFTER_BUILD` enabled (default), plugins are also installed to:

| Format | macOS path |
|--------|-----------|
| AU | `~/Library/Audio/Plug-Ins/Components/TillySynth.component` |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/TillySynth.vst3` |

## Running the Standalone

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --target TillySynth_Standalone && open build/TillySynth_artefacts/Release/Standalone/TillySynth.app    # macOS
```

Or on Windows, run the `.exe` directly from the Standalone output folder.

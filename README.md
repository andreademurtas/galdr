# Galdr

*Galdr* (Old Norse for a spell sung rather than spoken) is a free and open-source
synthesizer plugin (VST3 + standalone) with a black-metal soul, built with [JUCE](https://juce.com).

![Galdr GUI](docs/screenshot.png)

## Features

- 12-voice polyphonic engine: two band-limited (PolyBLEP) oscillators with up to
  7-voice unison, detune and stereo spread, plus sub oscillator and white/pink noise
- Multimode filter (LP 12/24 dB, HP, BP) with drive, dedicated envelope and keytracking
- Two LFOs (vibrato and filter sweep), glide, separate amp and filter envelopes
- Genre FX chain: four-flavour distortion, bitcrusher, chorus, tremolo picking,
  delay and cavern reverb
- Factory presets and on-screen MIDI keyboard
- Dark-fantasy themed GUI
- VST3 and standalone builds for Windows, macOS and Linux

## Building

Requirements: git, CMake ≥ 3.22 and a C++20 compiler (Visual Studio 2022 on Windows,
Xcode on macOS, GCC or Clang on Linux).

```sh
git clone --recurse-submodules <repo-url>
cd <repo>
cmake -B build
cmake --build build --config Release
```

On Linux, install the JUCE build dependencies first (see the package list in
`.github/workflows/build.yml`).

Artifacts are written to `build/Galdr_artefacts/Release/`:

| Artifact | Install location |
|---|---|
| `VST3/Galdr.vst3` | Windows: `C:\Program Files\Common Files\VST3` · macOS: `~/Library/Audio/Plug-Ins/VST3` · Linux: `~/.vst3` |
| `Standalone/` | Run directly, no installation needed |

## License

This project is licensed under the **GNU General Public License v3.0 or later**
(see [LICENSE](LICENSE)).

It uses the [JUCE framework](https://github.com/juce-framework/JUCE), licensed under
the AGPLv3, and the VST3 SDK interfaces distributed with JUCE under the GPLv3.

The GUI embeds the fonts [UnifrakturMaguntia](https://fonts.google.com/specimen/UnifrakturMaguntia)
and [IM Fell English](https://fonts.google.com/specimen/IM+Fell+English), both licensed under the
[SIL Open Font License](assets/fonts/OFL-UnifrakturMaguntia.txt).

*VST is a registered trademark of Steinberg Media Technologies GmbH.*

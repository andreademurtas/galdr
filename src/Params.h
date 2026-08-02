// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs. Frozen at first public release: hosts store automation by ID.
namespace pid
{
    // oscillators
    inline constexpr auto osc1Wave   = "osc1Wave";
    inline constexpr auto osc1Oct    = "osc1Oct";
    inline constexpr auto osc1Uni    = "osc1Uni";
    inline constexpr auto osc1Det    = "osc1Det";
    inline constexpr auto osc1Spread = "osc1Spread";
    inline constexpr auto osc1PW     = "osc1PW";
    inline constexpr auto osc1Lvl    = "osc1Lvl";

    inline constexpr auto osc2Wave   = "osc2Wave";
    inline constexpr auto osc2Oct    = "osc2Oct";
    inline constexpr auto osc2Semi   = "osc2Semi";
    inline constexpr auto osc2Uni    = "osc2Uni";
    inline constexpr auto osc2Det    = "osc2Det";
    inline constexpr auto osc2Spread = "osc2Spread";
    inline constexpr auto osc2PW     = "osc2PW";
    inline constexpr auto osc2Lvl    = "osc2Lvl";

    inline constexpr auto subWave    = "subWave";
    inline constexpr auto subOct     = "subOct";
    inline constexpr auto subLvl     = "subLvl";
    inline constexpr auto noiseType  = "noiseType";
    inline constexpr auto noiseLvl   = "noiseLvl";

    // filter
    inline constexpr auto filterType  = "filterType";
    inline constexpr auto cutoff      = "cutoff";
    inline constexpr auto resonance   = "resonance";
    inline constexpr auto filterDrive = "filterDrive";
    inline constexpr auto fEnvAmt     = "fEnvAmt";
    inline constexpr auto keytrack    = "keytrack";

    // envelopes (amp envelope keeps the original v0 ids)
    inline constexpr auto attack   = "attack";
    inline constexpr auto decay    = "decay";
    inline constexpr auto sustain  = "sustain";
    inline constexpr auto release  = "release";
    inline constexpr auto fAttack  = "fAttack";
    inline constexpr auto fDecay   = "fDecay";
    inline constexpr auto fSustain = "fSustain";
    inline constexpr auto fRelease = "fRelease";
    inline constexpr auto glide    = "glide";

    // lfos
    inline constexpr auto lfo1Shape = "lfo1Shape";
    inline constexpr auto lfo1Rate  = "lfo1Rate";
    inline constexpr auto lfo1Depth = "lfo1Depth";
    inline constexpr auto lfo2Shape = "lfo2Shape";
    inline constexpr auto lfo2Rate  = "lfo2Rate";
    inline constexpr auto lfo2Depth = "lfo2Depth";

    // v0.2: tempo sync ("Free" or musical divisions of the host tempo)
    inline constexpr auto lfo1Sync  = "lfo1Sync";
    inline constexpr auto lfo2Sync  = "lfo2Sync";
    inline constexpr auto tremSync  = "tremSync";
    inline constexpr auto delaySync = "delaySync";

    // v0.2: formant filter mode and ring modulator
    inline constexpr auto vowel  = "vowel";
    inline constexpr auto rmFreq = "rmFreq";
    inline constexpr auto rmMix  = "rmMix";

    // v0.3: performance, wavetable, reverb predelay, blizzard granular layer
    inline constexpr auto voiceMode = "voiceMode";
    inline constexpr auto bendRange = "bendRange";
    inline constexpr auto osc1Morph = "osc1Morph";
    inline constexpr auto osc2Morph = "osc2Morph";
    inline constexpr auto revPre    = "revPre";
    inline constexpr auto bzDensity = "bzDensity";
    inline constexpr auto bzSize    = "bzSize";
    inline constexpr auto bzPitch   = "bzPitch";
    inline constexpr auto bzSpread  = "bzSpread";
    inline constexpr auto bzLvl     = "bzLvl";

    // fx chain
    inline constexpr auto distType  = "distType";
    inline constexpr auto distDrive = "distDrive";
    inline constexpr auto distTone  = "distTone";
    inline constexpr auto distMix   = "distMix";

    inline constexpr auto crushBits = "crushBits";
    inline constexpr auto crushRate = "crushRate";
    inline constexpr auto crushMix  = "crushMix";

    inline constexpr auto chorusRate  = "chorusRate";
    inline constexpr auto chorusDepth = "chorusDepth";
    inline constexpr auto chorusMix   = "chorusMix";

    inline constexpr auto tremShape = "tremShape";
    inline constexpr auto tremRate  = "tremRate";
    inline constexpr auto tremDepth = "tremDepth";

    inline constexpr auto delayTime = "delayTime";
    inline constexpr auto delayFb   = "delayFb";
    inline constexpr auto delayMix  = "delayMix";

    inline constexpr auto revSize  = "revSize";
    inline constexpr auto revDamp  = "revDamp";
    inline constexpr auto revWidth = "revWidth";
    inline constexpr auto revMix   = "revMix";

    inline constexpr auto gain = "gain";
}

juce::AudioProcessorValueTreeState::ParameterLayout createGaldrParameterLayout();

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"

namespace presets
{

struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, double>> values; // plain (denormalised) values
};

inline const std::vector<Preset>& all()
{
    static const std::vector<Preset> list = {
        { "Init", {} },

        { "Buzzsaw Wall", {
            { pid::osc1Uni, 7 }, { pid::osc1Det, 35 }, { pid::osc1Spread, 0.9f }, { pid::osc1Lvl, 0.9f },
            { pid::osc2Uni, 5 }, { pid::osc2Semi, 7 }, { pid::osc2Det, 20 }, { pid::osc2Spread, 0.8f },
            { pid::osc2Lvl, 0.55f },
            { pid::noiseLvl, 0.06f },
            { pid::cutoff, 9000 }, { pid::resonance, 0.15f }, { pid::filterDrive, 0.35f },
            { pid::attack, 0.003f }, { pid::decay, 0.1f }, { pid::sustain, 0.9f }, { pid::release, 0.12f },
            { pid::distType, 1 }, { pid::distDrive, 0.55f }, { pid::distTone, 7000 },
            { pid::tremDepth, 0.85f }, { pid::tremRate, 13 },
            { pid::delayMix, 0.12f }, { pid::delayTime, 0.38f }, { pid::delayFb, 0.35f },
            { pid::revMix, 0.25f }, { pid::revSize, 0.6f } } },

        { "Frostbitten Pad", {
            { pid::osc1Uni, 5 }, { pid::osc1Det, 18 }, { pid::osc1Spread, 1 }, { pid::osc1Lvl, 0.7f },
            { pid::osc2Wave, 2 }, { pid::osc2PW, 0.3f }, { pid::osc2Semi, 7 }, { pid::osc2Uni, 3 },
            { pid::osc2Det, 14 }, { pid::osc2Lvl, 0.5f },
            { pid::noiseType, 1 }, { pid::noiseLvl, 0.12f },
            { pid::cutoff, 3500 }, { pid::resonance, 0.25f }, { pid::fEnvAmt, 0.3f },
            { pid::fAttack, 1.2f }, { pid::fDecay, 2 }, { pid::fSustain, 0.6f }, { pid::fRelease, 2 },
            { pid::attack, 1.5f }, { pid::decay, 1 }, { pid::sustain, 0.8f }, { pid::release, 2.5f },
            { pid::chorusMix, 0.5f }, { pid::chorusRate, 0.8f }, { pid::chorusDepth, 0.35f },
            { pid::revSize, 0.95f }, { pid::revMix, 0.5f }, { pid::revDamp, 0.4f },
            { pid::lfo2Rate, 0.15f }, { pid::lfo2Depth, 0.2f },
            { pid::distDrive, 0.2f }, { pid::distMix, 0.6f },
            { pid::gain, 0.7f } } },

        { "Necro Lead", {
            { pid::osc1Wave, 1 }, { pid::osc1Lvl, 0.85f },
            { pid::subWave, 1 }, { pid::subLvl, 0.4f },
            { pid::crushBits, 6 }, { pid::crushRate, 6 },
            { pid::distType, 3 }, { pid::distDrive, 0.5f },
            { pid::filterType, 1 }, { pid::cutoff, 6000 }, { pid::resonance, 0.35f },
            { pid::delayTime, 0.3f }, { pid::delayFb, 0.45f }, { pid::delayMix, 0.3f },
            { pid::revMix, 0.3f },
            { pid::lfo1Depth, 0.25f }, { pid::lfo1Rate, 5.5f },
            { pid::glide, 0.06f } } },

        { "Cavern Drone", {
            { pid::osc1Oct, -1 }, { pid::osc1Uni, 7 }, { pid::osc1Det, 25 }, { pid::osc1Spread, 1 },
            { pid::osc1Lvl, 0.8f },
            { pid::osc2Oct, -1 }, { pid::osc2Semi, 7 }, { pid::osc2Uni, 5 }, { pid::osc2Det, 18 },
            { pid::osc2Lvl, 0.6f },
            { pid::subLvl, 0.8f },
            { pid::cutoff, 900 }, { pid::resonance, 0.3f },
            { pid::attack, 2 }, { pid::decay, 1 }, { pid::sustain, 1 }, { pid::release, 3 },
            { pid::distType, 2 }, { pid::distDrive, 0.35f }, { pid::distMix, 0.8f },
            { pid::revSize, 1 }, { pid::revMix, 0.6f }, { pid::revDamp, 0.2f },
            { pid::lfo2Rate, 0.08f }, { pid::lfo2Depth, 0.35f },
            { pid::gain, 0.75f } } },

        { "Shrieking Gale", {
            { pid::noiseLvl, 0.9f },
            { pid::osc1Lvl, 0.15f },
            { pid::filterType, 2 }, { pid::cutoff, 2500 }, { pid::resonance, 0.7f },
            { pid::lfo2Shape, 4 }, { pid::lfo2Rate, 8 }, { pid::lfo2Depth, 0.5f },
            { pid::distType, 1 }, { pid::distDrive, 0.45f },
            { pid::revMix, 0.45f }, { pid::revSize, 0.85f },
            { pid::attack, 0.4f }, { pid::sustain, 1 }, { pid::release, 1.2f },
            { pid::tremDepth, 0.3f }, { pid::tremRate, 9 } } },
    };
    return list;
}

inline void apply(juce::AudioProcessorValueTreeState& apvts, int index)
{
    if (index < 0 || index >= (int) all().size())
        return;

    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            rp->setValueNotifyingHost(rp->getDefaultValue());

    for (const auto& [id, value] : all()[(size_t) index].values)
        if (auto* rp = apvts.getParameter(id))
            rp->setValueNotifyingHost(rp->convertTo0to1((float) value));
}

} // namespace presets

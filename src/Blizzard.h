// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace galdr
{

// Granular "snowstorm" texture layer: stochastic Hann-windowed grains of
// resonated noise, spread across pitch and the stereo field.
class Blizzard
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        for (auto& grain : grains)
            grain.remaining = 0;
    }

    void process(juce::AudioBuffer<float>& buffer, float density, float sizeSeconds,
                 float pitchHz, float spread, float level)
    {
        if (level < 0.001f)
            return;

        const int numSamples = buffer.getNumSamples();
        const int channels = buffer.getNumChannels();
        auto* L = buffer.getWritePointer(0);
        auto* R = channels > 1 ? buffer.getWritePointer(1) : L;
        const float spawnProb = density / (float) sr;

        for (int i = 0; i < numSamples; ++i)
        {
            if (rng.nextFloat() < spawnProb)
                spawn(sizeSeconds, pitchHz, spread);

            float l = 0.0f, r = 0.0f;
            for (auto& gr : grains)
            {
                if (gr.remaining <= 0)
                    continue;
                const float white = rng.nextFloat() * 2.0f - 1.0f;
                const float y = gr.b1 * gr.z1 - gr.b2 * gr.z2 + white * gr.inGain;
                gr.z2 = gr.z1;
                gr.z1 = y;
                const float w = 0.5f - 0.5f * std::cos(gr.phase * juce::MathConstants<float>::twoPi);
                gr.phase += gr.phaseInc;
                --gr.remaining;
                l += y * w * gr.ampL;
                r += y * w * gr.ampR;
            }
            L[i] += l * level;
            if (channels > 1)
                R[i] += r * level;
        }
    }

private:
    struct Grain
    {
        int remaining = 0;
        float phase = 0.0f, phaseInc = 0.0f;
        float b1 = 0.0f, b2 = 0.0f, z1 = 0.0f, z2 = 0.0f, inGain = 0.0f;
        float ampL = 0.0f, ampR = 0.0f;
    };

    void spawn(float sizeSeconds, float pitchHz, float spread)
    {
        for (auto& gr : grains)
        {
            if (gr.remaining > 0)
                continue;

            const float len = juce::jlimit(0.005f, 1.0f, sizeSeconds * (0.5f + rng.nextFloat()));
            gr.remaining = juce::jmax(16, (int) (len * (float) sr));
            gr.phase = 0.0f;
            gr.phaseInc = 1.0f / (float) gr.remaining;

            const float f = juce::jlimit(40.0f, (float) sr * 0.45f,
                pitchHz * std::exp2((rng.nextFloat() * 2.0f - 1.0f) * spread * 2.0f));
            const float theta = juce::MathConstants<float>::twoPi * f / (float) sr;
            const float pole = std::exp(-juce::MathConstants<float>::pi * (f / 12.0f) / (float) sr);
            gr.b1 = 2.0f * pole * std::cos(theta);
            gr.b2 = pole * pole;
            gr.z1 = gr.z2 = 0.0f;
            gr.inGain = (1.0f - pole) * 2.0f;

            const float pan = (rng.nextFloat() * 2.0f - 1.0f) * spread;
            const float a = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            gr.ampL = std::cos(a) * 0.7f;
            gr.ampR = std::sin(a) * 0.7f;
            return;
        }
    }

    static constexpr int maxGrains = 48;
    std::array<Grain, maxGrains> grains;
    juce::Random rng;
    double sr = 44100.0;
};

} // namespace galdr

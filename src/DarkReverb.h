// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

namespace galdr
{

// 8-line feedback delay network with a Householder feedback matrix, input
// diffusion allpasses, per-line damping and predelay. Tuned dark and cavernous:
// long decays stay smooth instead of turning metallic.
class DarkReverb
{
public:
    static constexpr int numLines = 8;

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        const float scale = (float) (sr / 44100.0);
        static constexpr int baseLens[numLines] = { 1237, 1381, 1597, 1867, 2053, 2251, 2399, 2617 };
        static constexpr int diffLens[3] = { 142, 379, 613 };

        for (int n = 0; n < numLines; ++n)
        {
            lineLen[n] = juce::jmax(4, (int) ((float) baseLens[n] * scale));
            lines[n].assign((size_t) lineLen[n], 0.0f);
            linePos[n] = 0;
            lpState[n] = 0.0f;
        }
        for (int a = 0; a < 3; ++a)
        {
            apfLen[a] = juce::jmax(4, (int) ((float) diffLens[a] * scale));
            apf[a].assign((size_t) apfLen[a], 0.0f);
            apfPos[a] = 0;
        }
        predelay.assign((size_t) (0.3 * sr) + 8, 0.0f);
        prePos = 0;
    }

    void setParams(float size, float damp, float width01, float predelaySeconds)
    {
        const float decaySeconds = 0.4f * std::pow(40.0f, juce::jlimit(0.0f, 1.0f, size));
        for (int n = 0; n < numLines; ++n)
            g[n] = std::pow(10.0f, -3.0f * (float) lineLen[n] / (decaySeconds * (float) sr));

        const float cutoff = 12000.0f * std::pow(1.0f / 12.0f, juce::jlimit(0.0f, 1.0f, damp));
        lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoff / (float) sr);
        width = width01;
        preSamples = juce::jlimit(1, (int) predelay.size() - 2, (int) (predelaySeconds * (float) sr));
    }

    void process(juce::AudioBuffer<float>& buffer, float mix)
    {
        const int channels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        auto* L = buffer.getWritePointer(0);
        auto* R = channels > 1 ? buffer.getWritePointer(1) : L;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            float x = 0.5f * (dryL + dryR);

            predelay[(size_t) prePos] = x;
            int rp = prePos - preSamples;
            if (rp < 0)
                rp += (int) predelay.size();
            x = predelay[(size_t) rp];
            if (++prePos >= (int) predelay.size())
                prePos = 0;

            for (int a = 0; a < 3; ++a)
            {
                const float d = apf[a][(size_t) apfPos[a]];
                const float v = x + 0.68f * d;
                apf[a][(size_t) apfPos[a]] = v;
                x = d - 0.68f * v;
                if (++apfPos[a] >= apfLen[a])
                    apfPos[a] = 0;
            }

            float r[numLines], sum = 0.0f;
            for (int k = 0; k < numLines; ++k)
            {
                r[k] = lines[k][(size_t) linePos[k]];
                sum += r[k];
            }
            const float householder = sum * (2.0f / (float) numLines);

            float wetL = 0.35f * (r[0] - r[2] + r[4] - r[6]);
            float wetR = 0.35f * (r[1] - r[3] + r[5] - r[7]);

            for (int k = 0; k < numLines; ++k)
            {
                const float v = g[k] * (r[k] - householder) + ((k & 1) ? -x : x) * 0.25f;
                lpState[k] += lpCoeff * (v - lpState[k]);
                lines[k][(size_t) linePos[k]] = lpState[k];
                if (++linePos[k] >= lineLen[k])
                    linePos[k] = 0;
            }

            const float mid  = 0.5f * (wetL + wetR);
            const float side = 0.5f * (wetL - wetR) * width;
            wetL = mid + side;
            wetR = mid - side;

            const float dryGain = 1.0f - mix * 0.5f;
            L[i] = dryL * dryGain + wetL * mix;
            if (channels > 1)
                R[i] = dryR * dryGain + wetR * mix;
        }
    }

private:
    double sr = 44100.0;
    std::vector<float> lines[numLines], apf[3], predelay;
    int lineLen[numLines] {}, linePos[numLines] {}, apfLen[3] {}, apfPos[3] {};
    int prePos = 0, preSamples = 1;
    float lpState[numLines] {}, g[numLines] {};
    float lpCoeff = 0.3f, width = 1.0f;
};

} // namespace galdr

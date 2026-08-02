// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include "Wavetable.h"

namespace galdr
{

enum class Wave { saw, square, pulse, triangle, sine, wavetable };

// Polynomial band-limited step: removes the aliasing of hard waveform edges.
inline float polyBlep(float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

struct BlepOsc
{
    float phase = 0.0f;

    float next(Wave wave, float dt, float pw, const Wavetable* wt = nullptr, float morph = 0.0f)
    {
        const float t = phase;
        phase += dt;
        if (phase >= 1.0f)
            phase -= 1.0f;

        switch (wave)
        {
            case Wave::wavetable:
                if (wt != nullptr)
                    return wt->sample(wt->mipForDt(dt), morph, t);
                [[fallthrough]];
            case Wave::saw:
                return 2.0f * t - 1.0f - polyBlep(t, dt);

            case Wave::square:
                pw = 0.5f;
                [[fallthrough]];
            case Wave::pulse:
            {
                float v = t < pw ? 1.0f : -1.0f;
                v += polyBlep(t, dt);
                float t2 = t - pw;
                if (t2 < 0.0f)
                    t2 += 1.0f;
                return v - polyBlep(t2, dt);
            }

            case Wave::triangle:
                return t < 0.5f ? 4.0f * t - 1.0f : 3.0f - 4.0f * t;

            case Wave::sine:
            default:
                return std::sin(t * juce::MathConstants<float>::twoPi);
        }
    }
};

constexpr int maxUnison = 7;

// A stack of detuned oscillators spread across the stereo field.
struct UnisonOsc
{
    BlepOsc voices[maxUnison];

    void resetPhases()
    {
        for (auto& v : voices)
            v.phase = 0.0f;
    }

    void randomisePhases(juce::Random& rng)
    {
        for (auto& v : voices)
            v.phase = rng.nextFloat();
    }

    void next(Wave wave, int unison, float detuneCents, float spread, float pw,
              float freq, float sampleRate, const Wavetable* wt, float morph,
              float& outL, float& outR)
    {
        unison = juce::jlimit(1, maxUnison, unison);
        const float norm = 1.0f / std::sqrt((float) unison);
        float l = 0.0f, r = 0.0f;

        for (int k = 0; k < unison; ++k)
        {
            const float pos = unison == 1 ? 0.0f
                                          : 2.0f * (float) k / (float) (unison - 1) - 1.0f;
            const float f  = freq * std::exp2(pos * detuneCents / 1200.0f);
            const float dt = juce::jmin(0.45f, f / sampleRate);
            const float s  = voices[k].next(wave, dt, pw, wt, morph) * norm;

            const float angle = (pos * spread + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            l += s * std::cos(angle);
            r += s * std::sin(angle);
        }

        outL = l;
        outR = r;
    }
};

// Paul Kellet's economy pink noise filter.
struct PinkFilter
{
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;

    float next(float white)
    {
        b0 = 0.99765f * b0 + white * 0.0990460f;
        b1 = 0.96300f * b1 + white * 0.2965164f;
        b2 = 0.57000f * b2 + white * 1.0526913f;
        return (b0 + b1 + b2 + white * 0.1848f) * 0.25f;
    }
};

inline float distort(int type, float x, float drive01)
{
    const float d = 1.0f + drive01 * 39.0f;
    switch (type)
    {
        case 0:  return std::tanh(x * d);                                       // soft
        case 1:  return juce::jlimit(-1.0f, 1.0f, x * d);                       // hard
        case 2:  return std::sin(x * d * juce::MathConstants<float>::halfPi);   // fold
        default:                                                                // grim (fuzz)
        {
            const float t = x * d;
            const float y = 1.0f - std::exp(-std::abs(t));
            return t >= 0.0f ? y : -y;
        }
    }
}

} // namespace galdr

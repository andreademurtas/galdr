// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <functional>
#include "BlackMetalLookAndFeel.h"

namespace galdr
{

// Lock-free mono tap the audio thread pushes into and a GUI timer drains.
class VisFifo
{
public:
    static constexpr int capacity = 1 << 14;

    void push(const float* l, const float* r, int numSamples, int channels)
    {
        if (fifo.getFreeSpace() < numSamples)
        {
            int s1, n1, s2, n2;
            fifo.prepareToRead(numSamples - fifo.getFreeSpace(), s1, n1, s2, n2);
            fifo.finishedRead(n1 + n2);
        }
        int s1, n1, s2, n2;
        fifo.prepareToWrite(numSamples, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i)
            data[(size_t) (s1 + i)] = mono(l, r, i, channels);
        for (int i = 0; i < n2; ++i)
            data[(size_t) (s2 + i)] = mono(l, r, n1 + i, channels);
        fifo.finishedWrite(n1 + n2);
    }

    int pull(float* dest, int maxSamples)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead(juce::jmin(maxSamples, fifo.getNumReady()), s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i)
            dest[i] = data[(size_t) (s1 + i)];
        for (int i = 0; i < n2; ++i)
            dest[n1 + i] = data[(size_t) (s2 + i)];
        fifo.finishedRead(n1 + n2);
        return n1 + n2;
    }

private:
    static float mono(const float* l, const float* r, int i, int channels)
    {
        return channels > 1 ? 0.5f * (l[i] + r[i]) : l[i];
    }

    juce::AbstractFifo fifo { capacity };
    std::array<float, capacity> data {};
};

class ScopeComponent : public juce::Component, private juce::Timer
{
public:
    explicit ScopeComponent(VisFifo& f) : fifoRef(f)
    {
        setInterceptsMouseClicks(false, false);
        startTimerHz(30);
    }

private:
    void timerCallback() override
    {
        float tmp[2048];
        int got;
        while ((got = fifoRef.pull(tmp, 2048)) > 0)
            for (int i = 0; i < got; ++i)
            {
                ring[(size_t) writePos] = tmp[i];
                writePos = (writePos + 1) % (int) ring.size();
            }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(juce::Colour(0xcc08080b));
        g.fillRect(b);
        g.setColour(theme::outline.withAlpha(0.6f));
        g.drawRect(b, 1.0f);
        g.setColour(theme::boneDim.withAlpha(0.2f));
        g.drawLine(b.getX(), b.getCentreY(), b.getRight(), b.getCentreY(), 1.0f);

        constexpr int windowLen = 1024;
        auto at = [this](int back)
        {
            return ring[(size_t) ((writePos + (int) ring.size() - 1 - back) % (int) ring.size())];
        };

        int trig = windowLen;
        for (int i = windowLen; i < windowLen + 900; ++i)
            if (at(i + 1) < 0.0f && at(i) >= 0.0f)
            {
                trig = i;
                break;
            }

        juce::Path p;
        for (int k = 0; k < windowLen; ++k)
        {
            const float v = juce::jlimit(-1.0f, 1.0f, at(trig - k));
            const float px = b.getX() + b.getWidth() * (float) k / (float) (windowLen - 1);
            const float py = b.getCentreY() - v * b.getHeight() * 0.45f;
            if (k == 0)
                p.startNewSubPath(px, py);
            else
                p.lineTo(px, py);
        }
        g.setColour(theme::blood.withAlpha(0.35f));
        g.strokePath(p, juce::PathStrokeType(3.0f));
        g.setColour(theme::bloodBright);
        g.strokePath(p, juce::PathStrokeType(1.2f));
    }

    VisFifo& fifoRef;
    std::array<float, 4096> ring {};
    int writePos = 0;
};

class SpectrumComponent : public juce::Component, private juce::Timer
{
public:
    SpectrumComponent(VisFifo& f, std::function<double()> sampleRateFn)
        : fifoRef(f), getSampleRate(std::move(sampleRateFn))
    {
        setInterceptsMouseClicks(false, false);
        display.fill(-100.0f);
        startTimerHz(30);
    }

private:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;

    void timerCallback() override
    {
        float tmp[2048];
        int got;
        while ((got = fifoRef.pull(tmp, 2048)) > 0)
            for (int i = 0; i < got; ++i)
            {
                ring[(size_t) ringPos] = tmp[i];
                ringPos = (ringPos + 1) % fftSize;
            }

        for (int i = 0; i < fftSize; ++i)
        {
            const float w = 0.5f - 0.5f * std::cos(juce::MathConstants<float>::twoPi
                                                   * (float) i / (float) (fftSize - 1));
            fftData[(size_t) i] = ring[(size_t) ((ringPos + i) % fftSize)] * w;
        }
        std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        for (int bin = 0; bin < fftSize / 2; ++bin)
        {
            const float db = juce::Decibels::gainToDecibels(
                fftData[(size_t) bin] / (float) (fftSize / 4), -100.0f);
            display[(size_t) bin] = juce::jmax(db, display[(size_t) bin] - 2.0f);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(juce::Colour(0xcc08080b));
        g.fillRect(b);
        g.setColour(theme::outline.withAlpha(0.6f));
        g.drawRect(b, 1.0f);

        const double sr = juce::jmax(8000.0, getSampleRate());
        const int w = juce::jmax(2, (int) b.getWidth());

        juce::Path p;
        p.startNewSubPath(b.getX(), b.getBottom());
        for (int x = 0; x < w; ++x)
        {
            const float freq = 20.0f * std::pow(1000.0f, (float) x / (float) (w - 1));
            const int bin = juce::jlimit(1, fftSize / 2 - 1,
                (int) (freq / (float) (sr * 0.5) * (float) (fftSize / 2)));
            const float db = juce::jlimit(-100.0f, 0.0f, display[(size_t) bin]);
            const float y = juce::jmap(db, -100.0f, 0.0f, b.getBottom(), b.getY() + 2.0f);
            p.lineTo(b.getX() + (float) x, y);
        }
        p.lineTo(b.getRight(), b.getBottom());
        p.closeSubPath();

        g.setColour(theme::blood.withAlpha(0.25f));
        g.fillPath(p);
        g.setColour(theme::bloodBright.withAlpha(0.9f));
        g.strokePath(p, juce::PathStrokeType(1.2f));
    }

    VisFifo& fifoRef;
    std::function<double()> getSampleRate;
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize> ring {};
    std::array<float, (size_t) fftSize * 2> fftData {};
    std::array<float, fftSize / 2> display {};
    int ringPos = 0;
};

} // namespace galdr

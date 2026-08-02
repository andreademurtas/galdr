// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        level = velocity * 0.2f;
        phase = 0.0;
        phaseDelta = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber) / getSampleRate();

        adsr.setSampleRate(getSampleRate());
        adsr.setParameters(adsrParams);
        adsr.noteOn();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            adsr.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void setParameters(const juce::ADSR::Parameters& envelope, int waveformType)
    {
        adsrParams = envelope;
        waveform = waveformType;
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        while (--numSamples >= 0)
        {
            auto sample = getWaveformSample() * level * adsr.getNextSample();

            for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                outputBuffer.addSample(ch, startSample, sample);

            phase += phaseDelta;
            if (phase >= 1.0)
                phase -= 1.0;
            ++startSample;

            if (! adsr.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    float getWaveformSample() const
    {
        switch (waveform)
        {
            case 1:  return static_cast<float>(2.0 * phase - 1.0);   // saw
            case 2:  return phase < 0.5 ? 1.0f : -1.0f;              // square
            default: return static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi));
        }
    }

    double phase = 0.0, phaseDelta = 0.0;
    float level = 0.0f;
    int waveform = 0;
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
};

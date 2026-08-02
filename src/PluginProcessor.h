// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "GaldrVoice.h"

class GaldrAudioProcessor : public juce::AudioProcessor
{
public:
    GaldrAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    void updateSettings(int numSamples);
    void applyDistortion(juce::AudioBuffer<float>&);
    void applyCrusher(juce::AudioBuffer<float>&);
    void applyTremolo(juce::AudioBuffer<float>&);
    void applyDelay(juce::AudioBuffer<float>&);
    void applyReverbAndGain(juce::AudioBuffer<float>&);

    float raw(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    float lfoShapeValue(int shape, float phase, float& held, bool wrapped);

    juce::Synthesiser synth;
    GaldrVoice::Settings settings;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 192000 * 2 };
    juce::Reverb reverb;

    juce::SmoothedValue<float> delaySamplesSm, gainSm;
    juce::Random lfoRng;

    double sr = 44100.0;
    float lfo1Phase = 0.0f, lfo2Phase = 0.0f, tremPhase = 0.0f;
    float lfo1Held = 0.0f, lfo2Held = 0.0f;
    float tremSmooth = 1.0f, tremCoeff = 0.05f;
    float toneState[2] { 0.0f, 0.0f };
    float crushHeld[2] { 0.0f, 0.0f };
    int   crushCount[2] { 0, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GaldrAudioProcessor)
};

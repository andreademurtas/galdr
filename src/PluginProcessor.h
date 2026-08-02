// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "GaldrVoice.h"
#include "DarkReverb.h"
#include "Blizzard.h"
#include "ScalaTuning.h"
#include "Visualizers.h"

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
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadTuning(const juce::File& sclFile);
    void resetTuning();
    juce::String getTuningName() const { return tuning.name; }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    galdr::VisFifo scopeFifo, spectrumFifo;

private:
    void updateSettings(int numSamples);
    void scanMidiControllers(const juce::MidiBuffer&);
    void processArp(juce::MidiBuffer&, int numSamples);
    void updateGlobalMods();
    void applyDistortion(juce::AudioBuffer<float>&, double sampleRate);
    void applyCrusher(juce::AudioBuffer<float>&, int osFactor);
    void applyRingMod(juce::AudioBuffer<float>&, double sampleRate);
    void applyTremolo(juce::AudioBuffer<float>&);
    void applyDelay(juce::AudioBuffer<float>&);
    void applyReverbAndGain(juce::AudioBuffer<float>&);

    float raw(const char* id) const { return apvts.getRawParameterValue(id)->load(); }
    float lfoShapeValue(int shape, float phase, float& held, bool wrapped);

    GaldrSynth synth;
    GaldrVoice::Settings settings;
    galdr::Tuning tuning;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine { 192000 * 2 };
    galdr::DarkReverb darkReverb;
    galdr::Blizzard blizzard;

    juce::SmoothedValue<float> delaySamplesSm, gainSm;
    juce::Random lfoRng;

    double sr = 44100.0;
    float hostBpm = 120.0f;
    float rmPhase = 0.0f;
    float lfo1Phase = 0.0f, lfo2Phase = 0.0f, tremPhase = 0.0f;
    float lfo1Held = 0.0f, lfo2Held = 0.0f;
    float tremSmooth = 1.0f, tremCoeff = 0.05f;
    float toneState[2] { 0.0f, 0.0f };
    float crushHeld[2] { 0.0f, 0.0f };
    int   crushCount[2] { 0, 0 };
    int   prevVoiceMode = 0;

    // controllers and global mod-matrix values
    std::atomic<juce::uint32> noteSerial { 0 };
    float modWheel = 0.0f, globalPressure = 0.0f;
    float lastVelocity = 0.0f, lastRandom = 0.0f, lastEnv3 = 0.0f, lastFiltEnv = 0.0f;
    float rmFreqModOct = 0.0f, tremDepthMod = 0.0f, delayMixMod = 0.0f;
    float revMixMod = 0.0f, bzLvlMod = 0.0f;
    float bzGateEnv = 0.0f;

    // arpeggiator
    struct ArpState
    {
        juce::SortedSet<int> held;
        juce::Array<int> heldOrder;
        float velocities[128] {};
        int samplesToNext = 0;
        int gateSamplesLeft = -1;
        int lastNote = -1;
        int step = 0;
        juce::Random rng;
    } arp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GaldrAudioProcessor)
};

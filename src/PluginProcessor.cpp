// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthVoice.h"

GaldrAudioProcessor::GaldrAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new SynthVoice());

    synth.addSound(new SynthSound());

    waveformParam = apvts.getRawParameterValue("waveform");
    attackParam   = apvts.getRawParameterValue("attack");
    decayParam    = apvts.getRawParameterValue("decay");
    sustainParam  = apvts.getRawParameterValue("sustain");
    releaseParam  = apvts.getRawParameterValue("release");
    gainParam     = apvts.getRawParameterValue("gain");
}

juce::AudioProcessorValueTreeState::ParameterLayout GaldrAudioProcessor::createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    const auto seconds = AudioParameterFloatAttributes()
        .withStringFromValueFunction([](float value, int)
        {
            return value < 1.0f ? String(roundToInt(value * 1000.0f)) + " ms"
                                : String(value, 2) + " s";
        })
        .withValueFromStringFunction([](const String& text)
        {
            auto value = text.getFloatValue();
            if (text.containsIgnoreCase("ms") || value > 5.0f)
                value /= 1000.0f;
            return value;
        });

    const auto percent = AudioParameterFloatAttributes()
        .withStringFromValueFunction([](float value, int)
        {
            return String(roundToInt(value * 100.0f)) + " %";
        })
        .withValueFromStringFunction([](const String& text)
        {
            auto value = text.getFloatValue();
            return value > 1.0f ? value / 100.0f : value;
        });

    layout.add(std::make_unique<AudioParameterChoice>(
        ParameterID { "waveform", 1 }, "Waveform", StringArray { "Sine", "Saw", "Square" }, 0));
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID { "attack", 1 }, "Attack",
        NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.35f), 0.01f, seconds));
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID { "decay", 1 }, "Decay",
        NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.35f), 0.1f, seconds));
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID { "sustain", 1 }, "Sustain",
        NormalisableRange<float>(0.0f, 1.0f), 0.8f, percent));
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID { "release", 1 }, "Release",
        NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.35f), 0.2f, seconds));
    layout.add(std::make_unique<AudioParameterFloat>(
        ParameterID { "gain", 1 }, "Gain",
        NormalisableRange<float>(0.0f, 1.0f), 0.8f, percent));

    return layout;
}

void GaldrAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
}

bool GaldrAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::stereo() || mainOut == juce::AudioChannelSet::mono();
}

void GaldrAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const juce::ADSR::Parameters envelope {
        attackParam->load(), decayParam->load(), sustainParam->load(), releaseParam->load()
    };
    const auto waveform = static_cast<int>(waveformParam->load());

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
            voice->setParameters(envelope, waveform);

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    buffer.applyGain(gainParam->load());
}

void GaldrAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    apvts.copyState().writeToStream(stream);
}

void GaldrAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto tree = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes)); tree.isValid())
        apvts.replaceState(tree);
}

juce::AudioProcessorEditor* GaldrAudioProcessor::createEditor()
{
    return new GaldrAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GaldrAudioProcessor();
}

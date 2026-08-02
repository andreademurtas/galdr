// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include "PluginProcessor.h"
#include "BlackMetalLookAndFeel.h"

class GaldrAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit GaldrAudioProcessorEditor(GaldrAudioProcessor&);
    ~GaldrAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& name);
    void drawRidge(juce::Graphics& g, float baseY, float amplitude, int seedStep,
                   juce::Colour colour) const;

    GaldrAudioProcessor& processorRef;
    BlackMetalLookAndFeel lnf;

    juce::ComboBox waveformBox;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider, gainSlider;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel, gainLabel;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ComboBoxAttachment> waveformAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment, sustainAttachment,
                                      releaseAttachment, gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GaldrAudioProcessorEditor)
};

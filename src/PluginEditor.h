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
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
    };

    struct Row
    {
        bool tall = false;                      // knob rows are tall, combo rows are not
        std::vector<juce::Component*> comps;    // sliders or combo boxes
        std::vector<juce::Label*> labels;       // parallel; nullptr for combos
    };

    struct Section
    {
        juce::String title;
        juce::Rectangle<int> bounds;
        std::vector<Row> rows;
    };

    Section& addSection(const juce::String& title, juce::Rectangle<int> bounds);
    Row& comboRow(Section&);
    Row& knobRow(Section&);
    void addKnob(Row&, const char* paramID, const juce::String& name);
    void addCombo(Row&, const char* paramID);
    void layoutSection(Section&);

    void drawRidge(juce::Graphics&, float baseY, float amplitude, int seedStep,
                   juce::Colour colour) const;

    GaldrAudioProcessor& processorRef;
    BlackMetalLookAndFeel lnf;

    juce::MidiKeyboardComponent keyboard;
    juce::ComboBox presetBox;

    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<juce::ComboBox> combos;
    std::vector<Section> sections;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GaldrAudioProcessorEditor)
};

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include "PluginProcessor.h"
#include "BlackMetalLookAndFeel.h"
#include "Visualizers.h"
#include "PresetBrowser.h"

class GaldrAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit GaldrAudioProcessorEditor(GaldrAudioProcessor&);
    ~GaldrAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

private:
    // Slider whose right click opens the parameter menu instead of dragging.
    struct GaldrSlider : juce::Slider
    {
        std::function<void()> onRightClick;

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (e.mods.isPopupMenu())
            {
                if (onRightClick)
                    onRightClick();
                return;
            }
            juce::Slider::mouseDown(e);
        }
    };

    struct Knob
    {
        GaldrSlider slider;
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
        juce::Rectangle<int> baseBounds;        // layout at reference size
        juce::Rectangle<int> bounds;            // scaled, set in resized()
        std::vector<Row> rows;
        juce::Component* custom = nullptr;      // fills the body instead of rows
    };

    Section& addSection(const juce::String& title, juce::Rectangle<int> baseBounds);
    Section& addCustomSection(const juce::String& title, juce::Rectangle<int> baseBounds,
                              juce::Component& content);
    Row& comboRow(Section&);
    Row& knobRow(Section&);
    void addKnob(Row&, const char* paramID, const juce::String& name);
    void addCombo(Row&, const char* paramID);
    void addHSlider(Row&, const char* paramID);
    void layoutSection(Section&, float scale);

    void showParamMenu(GaldrSlider&, const juce::String& paramID);
    void timerCallback() override;

    void drawRidge(juce::Graphics&, float baseY, float amplitude, int seedStep,
                   juce::Colour colour) const;

    GaldrAudioProcessor& processorRef;
    BlackMetalLookAndFeel lnf;

    juce::MidiKeyboardComponent keyboard;
    galdr::ScopeComponent scope;
    galdr::SpectrumComponent spectrum;

    juce::TextButton presetPrev { "<" }, presetNext { ">" };
    juce::TextButton presetNameButton;
    juce::TextButton undoButton { "Undo" }, redoButton { "Redo" };
    juce::TextButton saveButton { "Save" }, tuningButton;
    juce::TooltipWindow tooltipWindow { this };
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<PresetBrowser> presetBrowser;

    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<GaldrSlider> hsliders;
    std::vector<Section> sections;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GaldrAudioProcessorEditor)
};

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#include "PluginEditor.h"

GaldrAudioProcessorEditor::GaldrAudioProcessorEditor(GaldrAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Saw", 2);
    waveformBox.addItem("Square", 3);
    addAndMakeVisible(waveformBox);
    waveformAttachment = std::make_unique<ComboBoxAttachment>(processorRef.apvts, "waveform", waveformBox);

    setupKnob(attackSlider,  attackLabel,  "Attack");
    setupKnob(decaySlider,   decayLabel,   "Decay");
    setupKnob(sustainSlider, sustainLabel, "Sustain");
    setupKnob(releaseSlider, releaseLabel, "Release");
    setupKnob(gainSlider,    gainLabel,    "Gain");

    attackAttachment  = std::make_unique<SliderAttachment>(processorRef.apvts, "attack",  attackSlider);
    decayAttachment   = std::make_unique<SliderAttachment>(processorRef.apvts, "decay",   decaySlider);
    sustainAttachment = std::make_unique<SliderAttachment>(processorRef.apvts, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<SliderAttachment>(processorRef.apvts, "release", releaseSlider);
    gainAttachment    = std::make_unique<SliderAttachment>(processorRef.apvts, "gain",    gainSlider);

    // Must come after the children exist: sliders snapshot their text-box
    // colours when notified of a look-and-feel change.
    setLookAndFeel(&lnf);

    setSize(660, 430);
}

GaldrAudioProcessorEditor::~GaldrAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void GaldrAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label,
                                              const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 18);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void GaldrAudioProcessorEditor::resized()
{
    waveformBox.setBounds(55, 140, 140, 30);

    gainLabel.setBounds(30, 214, 190, 20);
    gainSlider.setBounds(65, 238, 120, 146);

    juce::Slider* knobs[]  { &attackSlider, &decaySlider, &sustainSlider, &releaseSlider };
    juce::Label*  labels[] { &attackLabel,  &decayLabel,  &sustainLabel,  &releaseLabel };

    for (int i = 0; i < 4; ++i)
    {
        auto cellX = 250 + i * 95;
        labels[i]->setBounds(cellX, 140, 95, 20);
        knobs[i]->setBounds(cellX + 2, 164, 91, 146);
    }
}

void GaldrAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();

    juce::ColourGradient sky(juce::Colour(0xff111116), 0.0f, 0.0f,
                             juce::Colour(0xff07070a), 0.0f, h, false);
    g.setGradientFill(sky);
    g.fillAll();

    drawRidge(g, 0.68f, 0.20f, 3, juce::Colour(0xff0e0e12));
    drawRidge(g, 0.82f, 0.14f, 5, juce::Colour(0xff121218));

    juce::ColourGradient vignette(juce::Colours::transparentBlack, w * 0.5f, h * 0.45f,
                                  juce::Colour(0xaa000000), 0.0f, 0.0f, true);
    g.setGradientFill(vignette);
    g.fillAll();

    // title with a blood-red shadow pass underneath
    auto titleArea = juce::Rectangle<int>(0, 8, (int) w, 58);
    g.setFont(lnf.getTitleFont(50.0f));
    g.setColour(theme::blood.withAlpha(0.6f));
    g.drawText("Galdr", titleArea.translated(0, 3), juce::Justification::centred);
    g.setColour(theme::bone);
    g.drawText("Galdr", titleArea, juce::Justification::centred);

    g.setFont(lnf.getBodyFont(15.0f));
    g.setColour(theme::boneDim);
    g.drawText("a grim & frostbitten polysynth", 0, 64, (int) w, 18, juce::Justification::centred);

    // horizontal divider with central diamond
    g.setColour(theme::outline);
    g.drawLine(40.0f, 94.0f, w - 40.0f, 94.0f, 1.0f);
    juce::Path diamond;
    diamond.addQuadrilateral(w * 0.5f, 89.0f, w * 0.5f + 6.0f, 94.0f,
                             w * 0.5f, 99.0f, w * 0.5f - 6.0f, 94.0f);
    g.setColour(theme::bloodBright);
    g.fillPath(diamond);

    // section headers with short red underlines
    g.setFont(lnf.getBodyFont(17.0f));
    g.setColour(theme::boneDim);
    g.drawText("Oscillator", 30, 108, 190, 20, juce::Justification::centred);
    g.drawText("Envelope", 250, 108, 380, 20, juce::Justification::centred);
    g.setColour(theme::blood);
    g.fillRect(109.0f, 130.0f, 32.0f, 1.5f);
    g.fillRect(424.0f, 130.0f, 32.0f, 1.5f);

    g.setColour(theme::outline.withAlpha(0.6f));
    g.drawLine(235.0f, 112.0f, 235.0f, 390.0f, 1.0f);

    // outer frame and blood-red corner brackets
    g.setColour(theme::outline);
    g.drawRect(getLocalBounds(), 2);
    g.setColour(theme::blood);
    const float m = 8.0f, len = 16.0f, t = 2.0f;
    g.fillRect(m, m, len, t);                 g.fillRect(m, m, t, len);
    g.fillRect(w - m - len, m, len, t);       g.fillRect(w - m - t, m, t, len);
    g.fillRect(m, h - m - t, len, t);         g.fillRect(m, h - m - len, t, len);
    g.fillRect(w - m - len, h - m - t, len, t); g.fillRect(w - m - t, h - m - len, t, len);
}

void GaldrAudioProcessorEditor::drawRidge(juce::Graphics& g, float baseY, float amplitude,
                                              int seedStep, juce::Colour colour) const
{
    static const float peaks[] = { 0.55f, 0.95f, 0.30f, 0.75f, 0.15f, 0.85f,
                                   0.45f, 0.65f, 0.25f, 0.90f, 0.40f };
    constexpr int numPoints = 12;
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();

    juce::Path ridge;
    ridge.startNewSubPath(0.0f, h);
    for (int i = 0; i < numPoints; ++i)
    {
        auto xPos = w * (float) i / (float) (numPoints - 1);
        auto peak = peaks[(i * seedStep) % 11];
        ridge.lineTo(xPos, h * baseY - h * amplitude * peak);
    }
    ridge.lineTo(w, h);
    ridge.closeSubPath();

    g.setColour(colour);
    g.fillPath(ridge);
}

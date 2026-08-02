// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace theme
{
    inline const juce::Colour background  { 0xff0a0a0c };
    inline const juce::Colour panel       { 0xff141416 };
    inline const juce::Colour outline     { 0xff2e2e34 };
    inline const juce::Colour bone        { 0xffcfc6b4 };
    inline const juce::Colour boneDim     { 0xff8d8678 };
    inline const juce::Colour blood       { 0xff8f1010 };
    inline const juce::Colour bloodBright { 0xffb31515 };
    inline const juce::Colour iron        { 0xff1d1d22 };
}

class BlackMetalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BlackMetalLookAndFeel()
        : titleTypeface(juce::Typeface::createSystemTypefaceFor(
              BinaryData::UnifrakturMaguntia_ttf, BinaryData::UnifrakturMaguntia_ttfSize)),
          bodyTypeface(juce::Typeface::createSystemTypefaceFor(
              BinaryData::IMFellEnglish_ttf, BinaryData::IMFellEnglish_ttfSize))
    {
        setColour(juce::Label::textColourId, theme::bone);
        setColour(juce::Slider::textBoxTextColourId, theme::bone);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxHighlightColourId, theme::blood.withAlpha(0.4f));
        setColour(juce::Slider::trackColourId, theme::blood);
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff26262b));
        setColour(juce::Slider::thumbColourId, theme::bone);
        setColour(juce::BubbleComponent::backgroundColourId, theme::panel);
        setColour(juce::BubbleComponent::outlineColourId, theme::outline);
        setColour(juce::ComboBox::backgroundColourId, theme::iron);
        setColour(juce::ComboBox::textColourId, theme::bone);
        setColour(juce::ComboBox::outlineColourId, theme::outline);
        setColour(juce::ComboBox::arrowColourId, theme::bloodBright);
        setColour(juce::PopupMenu::backgroundColourId, theme::panel);
        setColour(juce::PopupMenu::textColourId, theme::bone);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, theme::blood.withAlpha(0.4f));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::TextEditor::textColourId, theme::bone);
        setColour(juce::TextEditor::highlightColourId, theme::blood.withAlpha(0.4f));
        setColour(juce::CaretComponent::caretColourId, theme::bloodBright);
        setColour(juce::TextButton::buttonColourId, theme::iron);
        setColour(juce::TextButton::textColourOffId, theme::bone);
        setColour(juce::TextButton::textColourOnId, theme::bone);
        setColour(juce::TooltipWindow::backgroundColourId, theme::panel);
        setColour(juce::TooltipWindow::textColourId, theme::bone);
        setColour(juce::TooltipWindow::outlineColourId, theme::outline);
    }

    // Set by the editor when the window is rescaled.
    float uiScale = 1.0f;

    juce::Font getTitleFont(float height) const
    {
        return juce::Font(juce::FontOptions(titleTypeface).withHeight(height));
    }

    juce::Font getBodyFont(float height) const
    {
        return juce::Font(juce::FontOptions(bodyTypeface).withHeight(height));
    }

    juce::Font getLabelFont(juce::Label&) override        { return getBodyFont(15.0f * uiScale); }
    juce::Font getComboBoxFont(juce::ComboBox&) override  { return getBodyFont(16.0f * uiScale); }
    juce::Font getPopupMenuFont() override                { return getBodyFont(17.0f * uiScale); }

    juce::Font getTextButtonFont(juce::TextButton&, int) override
    {
        return getBodyFont(15.0f * uiScale);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                              bool highlighted, bool down) override
    {
        auto r = button.getLocalBounds();
        g.setColour(down ? theme::blood.withAlpha(0.5f)
                         : highlighted ? theme::iron.brighter(0.3f) : theme::iron);
        g.fillRect(r);
        g.setColour(theme::outline);
        g.drawRect(r, 1);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto arcRadius = radius - 2.0f;

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff26262b));
        g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));

        // value arc: wide translucent pass first for an ember-like glow
        juce::Path value;
        value.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, angle, true);
        g.setColour(theme::blood.withAlpha(0.35f));
        g.strokePath(value, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved));
        g.setColour(theme::bloodBright);
        g.strokePath(value, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));

        auto knobRadius = radius * 0.68f;
        auto knobArea = juce::Rectangle<float>(knobRadius * 2.0f, knobRadius * 2.0f).withCentre(centre);
        juce::ColourGradient body(theme::iron.brighter(0.25f), centre.x, centre.y - knobRadius,
                                  juce::Colour(0xff0d0d10), centre.x, centre.y + knobRadius, false);
        g.setGradientFill(body);
        g.fillEllipse(knobArea);
        g.setColour(theme::outline);
        g.drawEllipse(knobArea, 1.5f);

        juce::Path pointer;
        pointer.addTriangle(0.0f, -(knobRadius - 4.0f),
                            -2.8f, -knobRadius * 0.25f,
                            2.8f, -knobRadius * 0.25f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
        g.setColour(theme::bone);
        g.fillPath(pointer);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
                      int, int, int, int, juce::ComboBox& box) override
    {
        juce::Rectangle<int> b(0, 0, width, height);
        g.setColour(theme::iron);
        g.fillRect(b);
        g.setColour(box.hasKeyboardFocus(true) ? theme::blood : theme::outline);
        g.drawRect(b, 1);

        auto arrowZone = b.removeFromRight(26).toFloat();
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getCentreX() - 5.0f, arrowZone.getCentreY() - 2.5f,
                          arrowZone.getCentreX() + 5.0f, arrowZone.getCentreY() - 2.5f,
                          arrowZone.getCentreX(),        arrowZone.getCentreY() + 4.5f);
        g.setColour(findColour(juce::ComboBox::arrowColourId));
        g.fillPath(arrow);
    }

private:
    juce::Typeface::Ptr titleTypeface, bodyTypeface;
};

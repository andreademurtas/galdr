// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#include "PluginEditor.h"

namespace
{
constexpr int baseW = 1220;
constexpr int baseH = 1010;

const char* tipFor(const juce::String& id)
{
    static const std::map<juce::String, const char*> tips = {
        { pid::osc1Uni,    "Stacked detuned copies of the oscillator: the wall of saws" },
        { pid::osc1Det,    "Unison detune in cents" },
        { pid::osc1Morph,  "Wavetable position: sine, triangle, saw, square, grim (Wavetable mode)" },
        { pid::osc2Morph,  "Wavetable position: sine, triangle, saw, square, grim (Wavetable mode)" },
        { pid::osc2Semi,   "+7 semitones gives a power-chord drone" },
        { pid::fmAmt,      "Osc 2 frequency-modulates osc 1: bells, screams, metal" },
        { pid::oscSync,    "Hard-syncs osc 1 to osc 2's cycle: aggressive ripping timbres" },
        { pid::driftAmt,   "Slow random pitch drift per voice: old hardware instability" },
        { pid::filterDrive,"Saturation before the filter" },
        { pid::vowel,      "Vowel morph A-E-I-O-U (Formant filter type only)" },
        { pid::crushBits,  "Bit depth reduction" },
        { pid::crushRate,  "Sample-rate divider: the necro knob" },
        { pid::tremRate,   "13-16 Hz feels like tremolo picking" },
        { pid::glide,      "Portamento time between notes" },
        { pid::bendRange,  "Pitch-bend range in semitones; set 48 for MPE controllers" },
        { pid::rmFreq,     "Ring modulator carrier frequency" },
        { pid::rmMix,      "Ring modulator amount" },
        { pid::fEnvAmt,    "How much the filter envelope moves the cutoff (bipolar)" },
        { pid::keytrack,   "Cutoff follows the played note" },
        { pid::lfo1Depth,  "Vibrato depth (up to one semitone)" },
        { pid::lfo2Depth,  "Cutoff sweep depth" },
        { pid::lfo1Sync,   "Locks the rate to the host tempo" },
        { pid::lfo2Sync,   "Locks the rate to the host tempo" },
        { pid::tremSync,   "Locks the rate to the host tempo" },
        { pid::delaySync,  "Locks the delay time to the host tempo" },
        { pid::voiceMode,  "Poly, mono retrigger, or legato (slides without retriggering)" },
        { pid::bzGate,     "Gated: blows only while notes sound. Free: continuous drone" },
        { pid::bzDensity,  "Grains per second of the snowstorm layer" },
        { pid::bzPitch,    "Centre frequency of the grains" },
        { pid::bzSpread,   "Random pitch and stereo scatter of the grains" },
        { pid::revPre,     "Predelay before the cavern answers" },
        { pid::revShimmer, "Feeds the tail back one octave up: a spectral choir rising" },
        { pid::arpMode,    "Arpeggiator pattern; Off disables it" },
        { pid::arpRate,    "Arp step length, locked to the host tempo" },
        { pid::arpGate,    "How much of each step the note sustains" },
    };
    auto it = tips.find(id);
    return it != tips.end() ? it->second : nullptr;
}
} // namespace

GaldrAudioProcessorEditor::GaldrAudioProcessorEditor(GaldrAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
      scope(p.scopeFifo),
      spectrum(p.spectrumFifo, [&p] { return p.getSampleRate(); })
{
    sections.reserve(24);

    // ---- row 1: sound sources and filter
    {
        auto& s = addSection("Oscillator I", { 12, 80, 280, 238 });
        auto& c = comboRow(s);
        addCombo(c, pid::osc1Wave);
        addCombo(c, pid::oscSync);
        auto& r1 = knobRow(s);
        addKnob(r1, pid::osc1Oct, "Octave");
        addKnob(r1, pid::osc1Uni, "Unison");
        addKnob(r1, pid::osc1Det, "Detune");
        addKnob(r1, pid::fmAmt, "FM");
        auto& r2 = knobRow(s);
        addKnob(r2, pid::osc1Spread, "Spread");
        addKnob(r2, pid::osc1PW, "PW");
        addKnob(r2, pid::osc1Morph, "Morph");
        addKnob(r2, pid::osc1Lvl, "Level");
    }
    {
        auto& s = addSection("Oscillator II", { 300, 80, 280, 238 });
        addCombo(comboRow(s), pid::osc2Wave);
        auto& r1 = knobRow(s);
        addKnob(r1, pid::osc2Oct, "Octave");
        addKnob(r1, pid::osc2Semi, "Semi");
        addKnob(r1, pid::osc2Uni, "Unison");
        addKnob(r1, pid::osc2Det, "Detune");
        auto& r2 = knobRow(s);
        addKnob(r2, pid::osc2Spread, "Spread");
        addKnob(r2, pid::osc2PW, "PW");
        addKnob(r2, pid::osc2Morph, "Morph");
        addKnob(r2, pid::osc2Lvl, "Level");
    }
    {
        auto& s = addSection("Sub & Noise", { 588, 80, 220, 238 });
        auto& c1 = comboRow(s);
        addCombo(c1, pid::subWave);
        addCombo(c1, pid::subOct);
        addCombo(comboRow(s), pid::noiseType);
        auto& r = knobRow(s);
        addKnob(r, pid::subLvl, "Sub");
        addKnob(r, pid::noiseLvl, "Noise");
    }
    {
        auto& s = addSection("Filter", { 816, 80, 392, 238 });
        addCombo(comboRow(s), pid::filterType);
        auto& r = knobRow(s);
        addKnob(r, pid::cutoff, "Cutoff");
        addKnob(r, pid::resonance, "Reso");
        addKnob(r, pid::filterDrive, "Drive");
        addKnob(r, pid::fEnvAmt, "Env Amt");
        addKnob(r, pid::keytrack, "Track");
        auto& r2 = knobRow(s);
        addKnob(r2, pid::vowel, "Vowel");
        addKnob(r2, pid::driftAmt, "Drift");
    }

    // ---- row 2: envelopes, lfos, performance
    {
        auto& s = addSection("Amp Envelope", { 12, 324, 240, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::attack, "Attack");
        addKnob(r, pid::decay, "Decay");
        addKnob(r, pid::sustain, "Sustain");
        addKnob(r, pid::release, "Release");
    }
    {
        auto& s = addSection("Filter Envelope", { 260, 324, 240, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::fAttack, "Attack");
        addKnob(r, pid::fDecay, "Decay");
        addKnob(r, pid::fSustain, "Sustain");
        addKnob(r, pid::fRelease, "Release");
    }
    {
        auto& s = addSection("LFO I / Vibrato", { 508, 324, 204, 144 });
        auto& c = comboRow(s);
        addCombo(c, pid::lfo1Shape);
        addCombo(c, pid::lfo1Sync);
        auto& r = knobRow(s);
        addKnob(r, pid::lfo1Rate, "Rate");
        addKnob(r, pid::lfo1Depth, "Depth");
    }
    {
        auto& s = addSection("LFO II / Filter", { 720, 324, 204, 144 });
        auto& c = comboRow(s);
        addCombo(c, pid::lfo2Shape);
        addCombo(c, pid::lfo2Sync);
        auto& r = knobRow(s);
        addKnob(r, pid::lfo2Rate, "Rate");
        addKnob(r, pid::lfo2Depth, "Depth");
    }
    {
        auto& s = addSection("Perform / Ring", { 932, 324, 276, 144 });
        addCombo(comboRow(s), pid::voiceMode);
        auto& r = knobRow(s);
        addKnob(r, pid::glide, "Glide");
        addKnob(r, pid::bendRange, "Bend");
        addKnob(r, pid::rmFreq, "RM Freq");
        addKnob(r, pid::rmMix, "RM Mix");
        addKnob(r, pid::gain, "Master");
    }

    // ---- row 3: fx chain
    {
        auto& s = addSection("Distortion", { 12, 474, 220, 144 });
        addCombo(comboRow(s), pid::distType);
        auto& r = knobRow(s);
        addKnob(r, pid::distDrive, "Drive");
        addKnob(r, pid::distTone, "Tone");
        addKnob(r, pid::distMix, "Mix");
    }
    {
        auto& s = addSection("Crusher", { 240, 474, 160, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::crushBits, "Bits");
        addKnob(r, pid::crushRate, "Downsmp");
        addKnob(r, pid::crushMix, "Mix");
    }
    {
        auto& s = addSection("Chorus", { 408, 474, 160, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::chorusRate, "Rate");
        addKnob(r, pid::chorusDepth, "Depth");
        addKnob(r, pid::chorusMix, "Mix");
    }
    {
        auto& s = addSection("Tremolo", { 576, 474, 160, 144 });
        auto& c = comboRow(s);
        addCombo(c, pid::tremShape);
        addCombo(c, pid::tremSync);
        auto& r = knobRow(s);
        addKnob(r, pid::tremRate, "Rate");
        addKnob(r, pid::tremDepth, "Depth");
    }
    {
        auto& s = addSection("Delay", { 744, 474, 164, 144 });
        addCombo(comboRow(s), pid::delaySync);
        auto& r = knobRow(s);
        addKnob(r, pid::delayTime, "Time");
        addKnob(r, pid::delayFb, "Feedb");
        addKnob(r, pid::delayMix, "Mix");
    }
    {
        auto& s = addSection("Reverb", { 916, 474, 292, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::revSize, "Size");
        addKnob(r, pid::revDamp, "Damp");
        addKnob(r, pid::revPre, "Pre");
        addKnob(r, pid::revWidth, "Width");
        addKnob(r, pid::revShimmer, "Shim");
        addKnob(r, pid::revMix, "Mix");
    }

    // ---- row 4: arpeggiator, third envelope, mod matrix
    {
        auto& s = addSection("Arpeggiator", { 12, 624, 300, 150 });
        auto& c = comboRow(s);
        addCombo(c, pid::arpMode);
        addCombo(c, pid::arpRate);
        auto& r = knobRow(s);
        addKnob(r, pid::arpOct, "Octaves");
        addKnob(r, pid::arpGate, "Gate");
    }
    {
        auto& s = addSection("Envelope 3", { 320, 624, 260, 150 });
        auto& r = knobRow(s);
        addKnob(r, pid::env3A, "Attack");
        addKnob(r, pid::env3D, "Decay");
        addKnob(r, pid::env3S, "Sustain");
        addKnob(r, pid::env3R, "Release");
    }
    {
        auto& s = addSection("Mod Matrix", { 588, 624, 620, 150 });
        for (int slot = 0; slot < GaldrVoice::numModSlots; slot += 2)
        {
            auto& r = comboRow(s);
            addCombo(r, pid::modSrc[slot]);
            addCombo(r, pid::modDst[slot]);
            addHSlider(r, pid::modAmt[slot]);
            addCombo(r, pid::modSrc[slot + 1]);
            addCombo(r, pid::modDst[slot + 1]);
            addHSlider(r, pid::modAmt[slot + 1]);
        }
    }

    // ---- row 5: blizzard and visualizers
    {
        auto& s = addSection("Blizzard", { 12, 780, 340, 148 });
        addCombo(comboRow(s), pid::bzGate);
        auto& r = knobRow(s);
        addKnob(r, pid::bzDensity, "Density");
        addKnob(r, pid::bzSize, "Size");
        addKnob(r, pid::bzPitch, "Pitch");
        addKnob(r, pid::bzSpread, "Spread");
        addKnob(r, pid::bzLvl, "Level");
    }
    addCustomSection("Oscilloscope", { 360, 780, 420, 148 }, scope);
    addCustomSection("Spectrum", { 788, 780, 420, 148 }, spectrum);

    // ---- preset browser and header controls
    presetBrowser = std::make_unique<PresetBrowser>(processorRef, lnf);
    addChildComponent(*presetBrowser);

    presetNameButton.setButtonText("Init");
    presetNameButton.setTooltip("Open the preset browser");
    presetNameButton.onClick = [this]
    {
        if (presetBrowser->isVisible())
            presetBrowser->setVisible(false);
        else
            presetBrowser->open(false);
    };
    addAndMakeVisible(presetNameButton);

    presetPrev.onClick = [this] { presetBrowser->step(-1); };
    presetNext.onClick = [this] { presetBrowser->step(1); };
    addAndMakeVisible(presetPrev);
    addAndMakeVisible(presetNext);

    saveButton.setTooltip("Save the current sound as a user preset");
    saveButton.onClick = [this] { presetBrowser->open(true); };
    addAndMakeVisible(saveButton);

    // ---- undo / redo
    undoButton.setTooltip("Undo (Ctrl+Z)");
    redoButton.setTooltip("Redo (Ctrl+Shift+Z)");
    undoButton.onClick = [this] { processorRef.undoManager.undo(); };
    redoButton.onClick = [this] { processorRef.undoManager.redo(); };
    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);

    // ---- microtuning
    tuningButton.setButtonText(processorRef.getTuningName());
    tuningButton.setTooltip("Microtuning: load a Scala .scl file");
    tuningButton.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&lnf);
        menu.addItem(1, "Load Scala tuning (.scl)...");
        menu.addItem(2, "Reset to 12-TET");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(tuningButton),
            [this](int result)
            {
                if (result == 1)
                {
                    chooser = std::make_unique<juce::FileChooser>(
                        "Load Scala tuning",
                        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.scl");
                    chooser->launchAsync(juce::FileBrowserComponent::openMode
                                             | juce::FileBrowserComponent::canSelectFiles,
                                         [this](const juce::FileChooser& fc)
                                         {
                                             auto file = fc.getResult();
                                             if (file.existsAsFile() && processorRef.loadTuning(file))
                                                 tuningButton.setButtonText(processorRef.getTuningName());
                                         });
                }
                else if (result == 2)
                {
                    processorRef.resetTuning();
                    tuningButton.setButtonText(processorRef.getTuningName());
                }
            });
    };
    addAndMakeVisible(tuningButton);

    // ---- keyboard
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xff2a2a30));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff0c0c0e));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff1a1a1e));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, theme::blood.withAlpha(0.4f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, theme::bloodBright.withAlpha(0.7f));
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, theme::boneDim);
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setLowestVisibleKey(36);
    addAndMakeVisible(keyboard);

    // Must come after the children exist: sliders snapshot their text-box
    // colours when notified of a look-and-feel change.
    setLookAndFeel(&lnf);

    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio((double) baseW / (double) baseH);
    setResizeLimits(baseW * 3 / 4, baseH * 3 / 4, baseW * 2, baseH * 2);
    setSize(baseW, baseH);

    setWantsKeyboardFocus(true);
    startTimerHz(4);
}

GaldrAudioProcessorEditor::~GaldrAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void GaldrAudioProcessorEditor::timerCallback()
{
    // Group the parameter edits since the last tick into one undoable step.
    processorRef.undoManager.beginNewTransaction();
    undoButton.setEnabled(processorRef.undoManager.canUndo());
    redoButton.setEnabled(processorRef.undoManager.canRedo());

    auto name = processorRef.apvts.state.getProperty("presetName").toString();
    if (name.isEmpty())
        name = "Init";
    if (processorRef.presetDirty.load())
        name += " *";
    presetNameButton.setButtonText(name);

    tuningButton.setButtonText(processorRef.getTuningName());
}

bool GaldrAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && presetBrowser->isVisible())
    {
        presetBrowser->setVisible(false);
        return true;
    }
    const auto cmd = juce::ModifierKeys::commandModifier;
    if (key == juce::KeyPress('z', cmd, 0))
        return processorRef.undoManager.undo();
    if (key == juce::KeyPress('z', cmd | juce::ModifierKeys::shiftModifier, 0)
        || key == juce::KeyPress('y', cmd, 0))
        return processorRef.undoManager.redo();
    return false;
}

void GaldrAudioProcessorEditor::showParamMenu(GaldrSlider& slider, const juce::String& paramID)
{
    auto* param = processorRef.apvts.getParameter(paramID);
    if (param == nullptr)
        return;

    const int mappedCC = processorRef.midiCCFor(paramID);
    const bool learning = processorRef.isMidiLearnArmed();

    juce::PopupMenu menu;
    menu.setLookAndFeel(&lnf);
    menu.addItem(1, "Reset to default");
    if (slider.getTextBoxPosition() != juce::Slider::NoTextBox)
        menu.addItem(2, "Enter value...");
    menu.addSeparator();
    if (learning)
        menu.addItem(5, "Cancel MIDI learn");
    else
        menu.addItem(3, "MIDI learn");
    if (mappedCC >= 0)
        menu.addItem(4, "Clear MIDI map (CC " + juce::String(mappedCC) + ")");

    juce::Component::SafePointer<GaldrAudioProcessorEditor> safeThis(this);
    juce::Component::SafePointer<juce::Slider> safeSlider(&slider);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(slider),
        [safeThis, safeSlider, param, paramID](int result)
        {
            if (safeThis == nullptr)
                return;
            switch (result)
            {
                case 1:
                    param->beginChangeGesture();
                    param->setValueNotifyingHost(param->getDefaultValue());
                    param->endChangeGesture();
                    break;
                case 2:
                    if (safeSlider != nullptr)
                        safeSlider->showTextBox();
                    break;
                case 3: safeThis->processorRef.armMidiLearn(paramID); break;
                case 4: safeThis->processorRef.clearMidiCC(paramID); break;
                case 5: safeThis->processorRef.cancelMidiLearn(); break;
                default: break;
            }
        });
}

GaldrAudioProcessorEditor::Section& GaldrAudioProcessorEditor::addSection(const juce::String& title,
                                                                          juce::Rectangle<int> baseBounds)
{
    auto& s = sections.emplace_back();
    s.title = title;
    s.baseBounds = baseBounds;
    s.bounds = baseBounds;
    return s;
}

GaldrAudioProcessorEditor::Section& GaldrAudioProcessorEditor::addCustomSection(
    const juce::String& title, juce::Rectangle<int> baseBounds, juce::Component& content)
{
    auto& s = addSection(title, baseBounds);
    s.custom = &content;
    addAndMakeVisible(content);
    return s;
}

GaldrAudioProcessorEditor::Row& GaldrAudioProcessorEditor::comboRow(Section& s)
{
    auto& r = s.rows.emplace_back();
    r.tall = false;
    return r;
}

GaldrAudioProcessorEditor::Row& GaldrAudioProcessorEditor::knobRow(Section& s)
{
    auto& r = s.rows.emplace_back();
    r.tall = true;
    return r;
}

void GaldrAudioProcessorEditor::addKnob(Row& row, const char* paramID, const juce::String& name)
{
    auto* k = knobs.add(new Knob());
    k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 14);
    k->slider.onRightClick = [this, k, id = juce::String(paramID)] { showParamMenu(k->slider, id); };
    if (auto* tip = tipFor(paramID))
        k->slider.setTooltip(tip);
    addAndMakeVisible(k->slider);

    k->label.setText(name, juce::dontSendNotification);
    k->label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(k->label);

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(processorRef.apvts, paramID, k->slider));
    row.comps.push_back(&k->slider);
    row.labels.push_back(&k->label);
}

void GaldrAudioProcessorEditor::addCombo(Row& row, const char* paramID)
{
    auto* c = combos.add(new juce::ComboBox());
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(processorRef.apvts.getParameter(paramID)))
        c->addItemList(choice->choices, 1);
    if (auto* tip = tipFor(paramID))
        c->setTooltip(tip);
    addAndMakeVisible(c);

    comboAttachments.push_back(std::make_unique<ComboBoxAttachment>(processorRef.apvts, paramID, *c));
    row.comps.push_back(c);
    row.labels.push_back(nullptr);
}

void GaldrAudioProcessorEditor::addHSlider(Row& row, const char* paramID)
{
    auto* s = hsliders.add(new GaldrSlider());
    s->setSliderStyle(juce::Slider::LinearHorizontal);
    s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s->setPopupDisplayEnabled(true, true, this);
    s->onRightClick = [this, s, id = juce::String(paramID)] { showParamMenu(*s, id); };
    addAndMakeVisible(s);

    sliderAttachments.push_back(std::make_unique<SliderAttachment>(processorRef.apvts, paramID, *s));
    row.comps.push_back(s);
    row.labels.push_back(nullptr);
}

void GaldrAudioProcessorEditor::layoutSection(Section& s, float scale)
{
    auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };

    if (s.custom != nullptr)
    {
        s.custom->setBounds(s.bounds.withTrimmedTop(sc(20)).reduced(sc(6), sc(4)));
        return;
    }

    int y = s.bounds.getY() + sc(20);
    for (auto& row : s.rows)
    {
        const int h = row.tall ? sc(94) : sc(28);
        const int n = (int) row.comps.size();
        if (n == 0)
            continue;
        const int cw = (s.bounds.getWidth() - sc(12)) / n;
        const int x0 = s.bounds.getX() + sc(6);

        for (int i = 0; i < n; ++i)
        {
            juce::Rectangle<int> cell(x0 + i * cw, y, cw, h);
            if (row.tall)
            {
                row.labels[(size_t) i]->setBounds(cell.removeFromTop(sc(16)));
                if (auto* slider = dynamic_cast<juce::Slider*>(row.comps[(size_t) i]))
                    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, sc(62), sc(14));
                row.comps[(size_t) i]->setBounds(cell.reduced(1, 0));
            }
            else
            {
                row.comps[(size_t) i]->setBounds(cell.reduced(sc(4), 2));
            }
        }
        y += h + 2;
    }
}

void GaldrAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) baseW;
    lnf.uiScale = scale;
    auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };

    for (auto& s : sections)
    {
        s.bounds = { sc(s.baseBounds.getX()), sc(s.baseBounds.getY()),
                     sc(s.baseBounds.getWidth()), sc(s.baseBounds.getHeight()) };
        layoutSection(s, scale);
    }

    tuningButton.setBounds(sc(baseW - 690), sc(20), sc(104), sc(26));
    undoButton.setBounds(sc(baseW - 578), sc(20), sc(52), sc(26));
    redoButton.setBounds(sc(baseW - 522), sc(20), sc(52), sc(26));
    saveButton.setBounds(sc(baseW - 458), sc(20), sc(60), sc(26));
    presetPrev.setBounds(sc(baseW - 306), sc(20), sc(26), sc(26));
    presetNameButton.setBounds(sc(baseW - 276), sc(20), sc(240), sc(26));
    presetNext.setBounds(sc(baseW - 32), sc(20), sc(20), sc(26));

    keyboard.setKeyWidth(16.0f * scale);
    keyboard.setBounds(sc(12), sc(934), getWidth() - sc(24), sc(70));

    presetBrowser->setBounds(getLocalBounds());

    repaint();
}

void GaldrAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();
    const float scale = w / (float) baseW;

    juce::ColourGradient sky(juce::Colour(0xff111116), 0.0f, 0.0f,
                             juce::Colour(0xff07070a), 0.0f, h, false);
    g.setGradientFill(sky);
    g.fillAll();

    drawRidge(g, 0.72f, 0.20f, 3, juce::Colour(0xff0e0e12));
    drawRidge(g, 0.86f, 0.14f, 5, juce::Colour(0xff121218));

    juce::ColourGradient vignette(juce::Colours::transparentBlack, w * 0.5f, h * 0.45f,
                                  juce::Colour(0xaa000000), 0.0f, 0.0f, true);
    g.setGradientFill(vignette);
    g.fillAll();

    // header
    auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };
    auto titleArea = juce::Rectangle<int>(sc(20), sc(8), sc(260), sc(46));
    g.setFont(lnf.getTitleFont(42.0f * scale));
    g.setColour(theme::blood.withAlpha(0.6f));
    g.drawText("Galdr", titleArea.translated(0, sc(2)), juce::Justification::centredLeft);
    g.setColour(theme::bone);
    g.drawText("Galdr", titleArea, juce::Justification::centredLeft);

    g.setFont(lnf.getBodyFont(14.0f * scale));
    g.setColour(theme::boneDim);
    g.drawText("a grim & frostbitten polysynth", sc(22), sc(52), sc(400), sc(15),
               juce::Justification::centredLeft);

    // section panels
    for (const auto& s : sections)
    {
        auto b = s.bounds;
        g.setColour(theme::panel.withAlpha(0.55f));
        g.fillRect(b);
        g.setColour(theme::outline.withAlpha(0.8f));
        g.drawRect(b, 1);

        g.setFont(lnf.getBodyFont(15.0f * scale));
        g.setColour(theme::boneDim);
        g.drawText(s.title, b.getX() + sc(8), b.getY() + 2, b.getWidth() - sc(16), sc(16),
                   juce::Justification::centredLeft);
        g.setColour(theme::blood);
        g.fillRect((float) b.getX() + 8.0f * scale, (float) b.getY() + 18.0f * scale,
                   24.0f * scale, 1.5f);
    }

    // outer frame and corner brackets
    g.setColour(theme::outline);
    g.drawRect(getLocalBounds(), 2);
    g.setColour(theme::blood);
    const float m = 6.0f, len = 16.0f, t = 2.0f;
    g.fillRect(m, m, len, t);                   g.fillRect(m, m, t, len);
    g.fillRect(w - m - len, m, len, t);         g.fillRect(w - m - t, m, t, len);
    g.fillRect(m, h - m - t, len, t);           g.fillRect(m, h - m - len, t, len);
    g.fillRect(w - m - len, h - m - t, len, t); g.fillRect(w - m - t, h - m - len, t, len);
}

void GaldrAudioProcessorEditor::drawRidge(juce::Graphics& g, float baseY, float amplitude,
                                          int seedStep, juce::Colour colour) const
{
    static const float peaks[] = { 0.55f, 0.95f, 0.30f, 0.75f, 0.15f, 0.85f,
                                   0.45f, 0.65f, 0.25f, 0.90f, 0.40f };
    constexpr int numPoints = 16;
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

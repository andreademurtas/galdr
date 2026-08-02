// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#include "PluginEditor.h"
#include "Presets.h"

namespace
{
constexpr int baseW = 1220;
constexpr int baseH = 708;

const char* tipFor(const juce::String& id)
{
    static const std::map<juce::String, const char*> tips = {
        { pid::osc1Uni,    "Stacked detuned copies of the oscillator: the wall of saws" },
        { pid::osc1Det,    "Unison detune in cents" },
        { pid::osc2Semi,   "+7 semitones gives a power-chord drone" },
        { pid::filterDrive,"Saturation before the filter" },
        { pid::vowel,      "Vowel morph A-E-I-O-U (Formant filter type only)" },
        { pid::crushBits,  "Bit depth reduction" },
        { pid::crushRate,  "Sample-rate divider: the necro knob" },
        { pid::tremRate,   "13-16 Hz feels like tremolo picking" },
        { pid::glide,      "Portamento time between notes" },
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
    };
    auto it = tips.find(id);
    return it != tips.end() ? it->second : nullptr;
}
} // namespace

GaldrAudioProcessorEditor::GaldrAudioProcessorEditor(GaldrAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    sections.reserve(16);

    // ---- row 1: sound sources and filter
    {
        auto& s = addSection("Oscillator I", { 12, 80, 280, 238 });
        addCombo(comboRow(s), pid::osc1Wave);
        auto& r1 = knobRow(s);
        addKnob(r1, pid::osc1Oct, "Octave");
        addKnob(r1, pid::osc1Uni, "Unison");
        addKnob(r1, pid::osc1Det, "Detune");
        auto& r2 = knobRow(s);
        addKnob(r2, pid::osc1Spread, "Spread");
        addKnob(r2, pid::osc1PW, "PW");
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
    }

    // ---- row 2: envelopes, lfos, performance
    {
        auto& s = addSection("Amp Envelope", { 12, 324, 260, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::attack, "Attack");
        addKnob(r, pid::decay, "Decay");
        addKnob(r, pid::sustain, "Sustain");
        addKnob(r, pid::release, "Release");
    }
    {
        auto& s = addSection("Filter Envelope", { 280, 324, 260, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::fAttack, "Attack");
        addKnob(r, pid::fDecay, "Decay");
        addKnob(r, pid::fSustain, "Sustain");
        addKnob(r, pid::fRelease, "Release");
    }
    {
        auto& s = addSection("LFO I / Vibrato", { 548, 324, 208, 144 });
        auto& c = comboRow(s);
        addCombo(c, pid::lfo1Shape);
        addCombo(c, pid::lfo1Sync);
        auto& r = knobRow(s);
        addKnob(r, pid::lfo1Rate, "Rate");
        addKnob(r, pid::lfo1Depth, "Depth");
    }
    {
        auto& s = addSection("LFO II / Filter", { 764, 324, 208, 144 });
        auto& c = comboRow(s);
        addCombo(c, pid::lfo2Shape);
        addCombo(c, pid::lfo2Sync);
        auto& r = knobRow(s);
        addKnob(r, pid::lfo2Rate, "Rate");
        addKnob(r, pid::lfo2Depth, "Depth");
    }
    {
        auto& s = addSection("Perform / Ring", { 980, 324, 228, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::glide, "Glide");
        addKnob(r, pid::rmFreq, "RM Freq");
        addKnob(r, pid::rmMix, "RM Mix");
        addKnob(r, pid::gain, "Master");
    }

    // ---- row 3: fx chain
    {
        auto& s = addSection("Distortion", { 12, 474, 236, 144 });
        addCombo(comboRow(s), pid::distType);
        auto& r = knobRow(s);
        addKnob(r, pid::distDrive, "Drive");
        addKnob(r, pid::distTone, "Tone");
        addKnob(r, pid::distMix, "Mix");
    }
    {
        auto& s = addSection("Crusher", { 256, 474, 180, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::crushBits, "Bits");
        addKnob(r, pid::crushRate, "Downsmp");
        addKnob(r, pid::crushMix, "Mix");
    }
    {
        auto& s = addSection("Chorus", { 444, 474, 180, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::chorusRate, "Rate");
        addKnob(r, pid::chorusDepth, "Depth");
        addKnob(r, pid::chorusMix, "Mix");
    }
    {
        auto& s = addSection("Tremolo", { 632, 474, 166, 144 });
        auto& c = comboRow(s);
        addCombo(c, pid::tremShape);
        addCombo(c, pid::tremSync);
        auto& r = knobRow(s);
        addKnob(r, pid::tremRate, "Rate");
        addKnob(r, pid::tremDepth, "Depth");
    }
    {
        auto& s = addSection("Delay", { 806, 474, 170, 144 });
        addCombo(comboRow(s), pid::delaySync);
        auto& r = knobRow(s);
        addKnob(r, pid::delayTime, "Time");
        addKnob(r, pid::delayFb, "Feedb");
        addKnob(r, pid::delayMix, "Mix");
    }
    {
        auto& s = addSection("Reverb", { 984, 474, 224, 144 });
        auto& r = knobRow(s);
        addKnob(r, pid::revSize, "Size");
        addKnob(r, pid::revDamp, "Damp");
        addKnob(r, pid::revWidth, "Width");
        addKnob(r, pid::revMix, "Mix");
    }

    // ---- presets
    int itemId = 1;
    for (const auto& preset : presets::all())
        presetBox.addItem(preset.name, itemId++);
    presetBox.setTextWhenNothingSelected("Presets");
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0)
            presets::apply(processorRef.apvts, idx);
    };
    addAndMakeVisible(presetBox);

    auto presetDir = []
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                       .getChildFile("Galdr Presets");
        dir.createDirectory();
        return dir;
    };

    saveButton.onClick = [this, presetDir]
    {
        chooser = std::make_unique<juce::FileChooser>("Save preset",
                                                      presetDir().getChildFile("Preset.galdr"),
                                                      "*.galdr");
        chooser->launchAsync(juce::FileBrowserComponent::saveMode
                                 | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (file == juce::File())
                                     return;
                                 if (auto xml = processorRef.apvts.copyState().createXml())
                                     xml->writeTo(file.withFileExtension("galdr"));
                             });
    };
    addAndMakeVisible(saveButton);

    loadButton.onClick = [this, presetDir]
    {
        chooser = std::make_unique<juce::FileChooser>("Load preset", presetDir(), "*.galdr");
        chooser->launchAsync(juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
                             [this](const juce::FileChooser& fc)
                             {
                                 auto file = fc.getResult();
                                 if (! file.existsAsFile())
                                     return;
                                 if (auto xml = juce::XmlDocument::parse(file))
                                     processorRef.apvts.replaceState(juce::ValueTree::fromXml(*xml));
                             });
    };
    addAndMakeVisible(loadButton);

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
}

GaldrAudioProcessorEditor::~GaldrAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
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

void GaldrAudioProcessorEditor::layoutSection(Section& s, float scale)
{
    auto sc = [scale](int v) { return juce::roundToInt((float) v * scale); };

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

    loadButton.setBounds(sc(baseW - 398), sc(20), sc(88), sc(26));
    saveButton.setBounds(sc(baseW - 306), sc(20), sc(88), sc(26));
    presetBox.setBounds(sc(baseW - 212), sc(20), sc(200), sc(26));

    keyboard.setKeyWidth(16.0f * scale);
    keyboard.setBounds(sc(12), sc(626), getWidth() - sc(24), sc(70));

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

    drawRidge(g, 0.70f, 0.20f, 3, juce::Colour(0xff0e0e12));
    drawRidge(g, 0.84f, 0.14f, 5, juce::Colour(0xff121218));

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

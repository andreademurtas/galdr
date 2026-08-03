// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int numVoices = 12;

// Beats per cycle for each entry of the sync choice params; 0 = free-running.
float syncBeats(int index)
{
    static constexpr float beats[] = { 0.0f, 8.0f, 4.0f, 2.0f, 1.0f, 0.5f,
                                       1.0f / 3.0f, 0.25f, 1.0f / 6.0f, 0.125f };
    return beats[juce::jlimit(0, 9, index)];
}

// Arp rate choices have no "Free" entry: index 0 is 1/1.
float arpBeats(int index)
{
    static constexpr float beats[] = { 4.0f, 2.0f, 1.0f, 0.5f, 1.0f / 3.0f,
                                       0.25f, 1.0f / 6.0f, 0.125f };
    return beats[juce::jlimit(0, 7, index)];
}
}

GaldrAudioProcessor::GaldrAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "PARAMETERS", createGaldrParameterLayout())
{
    settings.noteFreqs = tuning.freqs;
    settings.noteCounter = &noteSerial;

    for (int i = 0; i < numVoices; ++i)
        synth.addVoice(new GaldrVoice(settings));

    synth.addSound(new SynthSound());

    galdr::Wavetable::global(); // build the tables up front, off the audio thread

    // any parameter edit marks the current preset as modified
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            apvts.addParameterListener(rp->paramID, this);
}

GaldrAudioProcessor::~GaldrAudioProcessor()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            apvts.removeParameterListener(rp->paramID, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout createGaldrParameterLayout()
{
    using namespace juce;

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
            if (text.contains("%") || std::abs(value) > 1.0f)
                value /= 100.0f;
            return value;
        });

    const auto hertz = AudioParameterFloatAttributes()
        .withStringFromValueFunction([](float value, int)
        {
            if (value >= 1000.0f) return String(value / 1000.0f, 2) + " kHz";
            if (value >= 100.0f)  return String(roundToInt(value)) + " Hz";
            return String(value, 2) + " Hz";
        })
        .withValueFromStringFunction([](const String& text)
        {
            auto value = text.getFloatValue();
            if (text.containsIgnoreCase("k"))
                value *= 1000.0f;
            return value;
        });

    const auto cents = AudioParameterFloatAttributes()
        .withStringFromValueFunction([](float value, int)
        {
            return String(roundToInt(value)) + " ct";
        });

    const auto perSecond = AudioParameterFloatAttributes()
        .withStringFromValueFunction([](float value, int)
        {
            return String(value, 1) + " /s";
        });

    const StringArray waveNames { "Saw", "Square", "Pulse", "Triangle", "Sine", "Wavetable" };
    const StringArray lfoShapes { "Sine", "Triangle", "Saw", "Square", "S&H" };
    const StringArray syncNames { "Free", "2/1", "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };

    const NormalisableRange<float> envRange(0.001f, 5.0f, 0.0f, 0.35f);
    const NormalisableRange<float> zeroOne(0.0f, 1.0f);
    const NormalisableRange<float> freqRange(20.0f, 20000.0f, 0.0f, 0.25f);

    AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout](auto param) { layout.add(std::move(param)); };

    // OSC 1
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::osc1Wave, 1 }, "Osc 1 Wave", waveNames, 0));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::osc1Oct, 1 }, "Osc 1 Octave", -2, 2, 0));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::osc1Uni, 1 }, "Osc 1 Unison", 1, 7, 1));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc1Det, 1 }, "Osc 1 Detune",
        NormalisableRange<float>(0.0f, 100.0f), 12.0f, cents));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc1Spread, 1 }, "Osc 1 Spread", zeroOne, 0.7f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc1PW, 1 }, "Osc 1 PW",
        NormalisableRange<float>(0.05f, 0.95f), 0.5f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc1Lvl, 1 }, "Osc 1 Level", zeroOne, 0.8f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc1Morph, 1 }, "Osc 1 Morph", zeroOne, 0.5f, percent));

    // OSC 2
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::osc2Wave, 1 }, "Osc 2 Wave", waveNames, 0));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::osc2Oct, 1 }, "Osc 2 Octave", -2, 2, 0));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::osc2Semi, 1 }, "Osc 2 Semi", -12, 12, 0));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::osc2Uni, 1 }, "Osc 2 Unison", 1, 7, 1));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc2Det, 1 }, "Osc 2 Detune",
        NormalisableRange<float>(0.0f, 100.0f), 12.0f, cents));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc2Spread, 1 }, "Osc 2 Spread", zeroOne, 0.7f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc2PW, 1 }, "Osc 2 PW",
        NormalisableRange<float>(0.05f, 0.95f), 0.5f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc2Lvl, 1 }, "Osc 2 Level", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::osc2Morph, 1 }, "Osc 2 Morph", zeroOne, 0.5f, percent));

    // SUB + NOISE
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::subWave, 1 }, "Sub Wave",
        StringArray { "Sine", "Square" }, 0));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::subOct, 1 }, "Sub Octave",
        StringArray { "-1 oct", "-2 oct" }, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::subLvl, 1 }, "Sub Level", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::noiseType, 1 }, "Noise Type",
        StringArray { "White", "Pink" }, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::noiseLvl, 1 }, "Noise Level", zeroOne, 0.0f, percent));

    // FILTER
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::filterType, 1 }, "Filter Type",
        StringArray { "LP 24", "LP 12", "HP", "BP", "Formant" }, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::vowel, 1 }, "Vowel", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::cutoff, 1 }, "Cutoff", freqRange, 12000.0f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::resonance, 1 }, "Resonance", zeroOne, 0.2f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::filterDrive, 1 }, "Filter Drive", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::fEnvAmt, 1 }, "Env Amount",
        NormalisableRange<float>(-1.0f, 1.0f), 0.0f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::keytrack, 1 }, "Keytrack", zeroOne, 0.0f, percent));

    // ENVELOPES
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::attack, 1 }, "Attack", envRange, 0.005f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::decay, 1 }, "Decay", envRange, 0.1f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::sustain, 1 }, "Sustain", zeroOne, 0.8f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::release, 1 }, "Release", envRange, 0.25f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::fAttack, 1 }, "F.Attack", envRange, 0.005f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::fDecay, 1 }, "F.Decay", envRange, 0.15f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::fSustain, 1 }, "F.Sustain", zeroOne, 0.5f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::fRelease, 1 }, "F.Release", envRange, 0.3f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::glide, 1 }, "Glide",
        NormalisableRange<float>(0.0f, 1.0f, 0.0f, 0.5f), 0.0f, seconds));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::voiceMode, 1 }, "Voice Mode",
        StringArray { "Poly", "Mono", "Legato" }, 0));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::bendRange, 1 }, "Bend Range", 1, 48, 2));

    // v0.4: FM / hard sync, drift, third envelope
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::fmAmt, 1 }, "FM Amount", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::oscSync, 1 }, "Osc Sync",
        StringArray { "Sync Off", "Sync On" }, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::driftAmt, 1 }, "Drift", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::env3A, 1 }, "E3 Attack", envRange, 0.01f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::env3D, 1 }, "E3 Decay", envRange, 0.2f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::env3S, 1 }, "E3 Sustain", zeroOne, 0.7f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::env3R, 1 }, "E3 Release", envRange, 0.3f, seconds));

    // v0.4: arpeggiator
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::arpMode, 1 }, "Arp Mode",
        StringArray { "Off", "Up", "Down", "Up-Down", "Random", "As Played" }, 0));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::arpRate, 1 }, "Arp Rate",
        StringArray { "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 5));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::arpOct, 1 }, "Arp Octaves", 1, 4, 1));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::arpGate, 1 }, "Arp Gate",
        NormalisableRange<float>(0.05f, 1.0f), 0.7f, percent));

    // v0.4: modulation matrix
    const StringArray modSources { "None", "LFO 1", "LFO 2", "Filter Env", "Env 3",
                                   "Velocity", "Mod Wheel", "Pressure", "Random" };
    const StringArray modDests { "None", "Pitch", "Cutoff", "Resonance", "Vowel",
                                 "Osc 1 Morph", "Osc 2 Morph", "Osc 1 PW", "Osc 2 PW",
                                 "FM Amount", "Noise Level", "RM Freq", "Trem Depth",
                                 "Delay Mix", "Reverb Mix", "Blizzard Lvl" };
    for (int s = 0; s < GaldrVoice::numModSlots; ++s)
    {
        add(std::make_unique<AudioParameterChoice>(ParameterID { pid::modSrc[s], 1 },
            "Mod " + String(s + 1) + " Source", modSources, 0));
        add(std::make_unique<AudioParameterChoice>(ParameterID { pid::modDst[s], 1 },
            "Mod " + String(s + 1) + " Dest", modDests, 0));
        add(std::make_unique<AudioParameterFloat>(ParameterID { pid::modAmt[s], 1 },
            "Mod " + String(s + 1) + " Amount", NormalisableRange<float>(-1.0f, 1.0f), 0.0f, percent));
    }

    // LFOs
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::lfo1Shape, 1 }, "LFO 1 Shape", lfoShapes, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::lfo1Rate, 1 }, "LFO 1 Rate",
        NormalisableRange<float>(0.05f, 20.0f, 0.0f, 0.5f), 5.0f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::lfo1Depth, 1 }, "Vibrato", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::lfo2Shape, 1 }, "LFO 2 Shape", lfoShapes, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::lfo2Rate, 1 }, "LFO 2 Rate",
        NormalisableRange<float>(0.02f, 20.0f, 0.0f, 0.5f), 0.5f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::lfo2Depth, 1 }, "To Cutoff", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::lfo1Sync, 1 }, "LFO 1 Sync", syncNames, 0));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::lfo2Sync, 1 }, "LFO 2 Sync", syncNames, 0));

    // FX
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::distType, 1 }, "Dist Type",
        StringArray { "Soft", "Hard", "Fold", "Grim" }, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::distDrive, 1 }, "Drive", zeroOne, 0.3f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::distTone, 1 }, "Tone",
        NormalisableRange<float>(500.0f, 20000.0f, 0.0f, 0.3f), 8000.0f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::distMix, 1 }, "Dist Mix", zeroOne, 1.0f, percent));

    add(std::make_unique<AudioParameterInt>(ParameterID { pid::crushBits, 1 }, "Bits", 2, 16, 16));
    add(std::make_unique<AudioParameterInt>(ParameterID { pid::crushRate, 1 }, "Downsample", 1, 50, 1));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::crushMix, 1 }, "Crush Mix", zeroOne, 1.0f, percent));

    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::chorusRate, 1 }, "Chorus Rate",
        NormalisableRange<float>(0.1f, 10.0f, 0.0f, 0.5f), 1.2f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::chorusDepth, 1 }, "Chorus Depth", zeroOne, 0.25f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::chorusMix, 1 }, "Chorus Mix", zeroOne, 0.0f, percent));

    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::tremShape, 1 }, "Trem Shape",
        StringArray { "Square", "Sine" }, 0));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::tremSync, 1 }, "Trem Sync", syncNames, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::tremRate, 1 }, "Trem Rate",
        NormalisableRange<float>(0.5f, 30.0f, 0.0f, 0.5f), 13.0f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::tremDepth, 1 }, "Trem Depth", zeroOne, 0.0f, percent));

    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::delaySync, 1 }, "Delay Sync", syncNames, 0));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::delayTime, 1 }, "Delay Time",
        NormalisableRange<float>(0.02f, 1.5f, 0.0f, 0.5f), 0.45f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::delayFb, 1 }, "Feedback",
        NormalisableRange<float>(0.0f, 0.95f), 0.4f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::delayMix, 1 }, "Delay Mix", zeroOne, 0.0f, percent));

    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::revSize, 1 }, "Size", zeroOne, 0.8f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::revDamp, 1 }, "Damp", zeroOne, 0.3f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::revPre, 1 }, "Predelay",
        NormalisableRange<float>(0.0f, 0.25f, 0.0f, 0.5f), 0.02f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::revWidth, 1 }, "Width", zeroOne, 1.0f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::revShimmer, 1 }, "Shimmer", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::revMix, 1 }, "Reverb Mix", zeroOne, 0.25f, percent));

    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::bzDensity, 1 }, "Density",
        NormalisableRange<float>(0.5f, 200.0f, 0.0f, 0.4f), 20.0f, perSecond));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::bzSize, 1 }, "Grain Size",
        NormalisableRange<float>(0.01f, 0.5f, 0.0f, 0.5f), 0.08f, seconds));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::bzPitch, 1 }, "Pitch",
        NormalisableRange<float>(100.0f, 8000.0f, 0.0f, 0.3f), 900.0f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::bzSpread, 1 }, "Spread", zeroOne, 0.5f, percent));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::bzLvl, 1 }, "Level", zeroOne, 0.0f, percent));
    add(std::make_unique<AudioParameterChoice>(ParameterID { pid::bzGate, 1 }, "Blizzard Gate",
        StringArray { "Gated", "Free" }, 0));

    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::rmFreq, 1 }, "RM Freq",
        NormalisableRange<float>(20.0f, 5000.0f, 0.0f, 0.3f), 250.0f, hertz));
    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::rmMix, 1 }, "RM Mix", zeroOne, 0.0f, percent));

    add(std::make_unique<AudioParameterFloat>(ParameterID { pid::gain, 1 }, "Gain", zeroOne, 0.8f, percent));

    return layout;
}

void GaldrAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.setCurrentPlaybackSampleRate(sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<GaldrVoice*>(synth.getVoice(i)))
            voice->prepare(sampleRate, samplesPerBlock);

    const auto channels = (juce::uint32) juce::jmax(1, getTotalNumOutputChannels());
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, channels };

    chorus.prepare(spec);
    chorus.reset();
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples((int) std::ceil(2.0 * sampleRate));
    delayLine.reset();

    darkReverb.prepare(sampleRate);
    blizzard.prepare(sampleRate);

    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        (size_t) channels, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampling->initProcessing((size_t) samplesPerBlock);
    setLatencySamples(juce::roundToInt(oversampling->getLatencyInSamples()));

    delaySamplesSm.reset(sampleRate, 0.1);
    delaySamplesSm.setCurrentAndTargetValue((float) (0.45 * sampleRate));
    gainSm.reset(sampleRate, 0.05);
    gainSm.setCurrentAndTargetValue(raw(pid::gain));

    tremCoeff = 1.0f - std::exp(-1.0f / (0.0015f * (float) sampleRate));
    tremSmooth = 1.0f;
    toneState[0] = toneState[1] = 0.0f;
    crushHeld[0] = crushHeld[1] = 0.0f;
    crushCount[0] = crushCount[1] = 0;
}

bool GaldrAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mainOut = layouts.getMainOutputChannelSet();
    return mainOut == juce::AudioChannelSet::stereo() || mainOut == juce::AudioChannelSet::mono();
}

float GaldrAudioProcessor::lfoShapeValue(int shape, float phase, float& held, bool wrapped)
{
    switch (shape)
    {
        case 0:  return std::sin(phase * juce::MathConstants<float>::twoPi);
        case 1:  return 4.0f * std::abs(phase - 0.5f) - 1.0f;
        case 2:  return 2.0f * phase - 1.0f;
        case 3:  return phase < 0.5f ? 1.0f : -1.0f;
        default:
            if (wrapped)
                held = lfoRng.nextFloat() * 2.0f - 1.0f;
            return held;
    }
}

void GaldrAudioProcessor::updateSettings(int numSamples)
{
    const int voiceMode = (int) raw(pid::voiceMode);
    if (voiceMode != prevVoiceMode)
    {
        prevVoiceMode = voiceMode;
        synth.setMode(voiceMode);
    }

    settings.osc1Wave   = (galdr::Wave) (int) raw(pid::osc1Wave);
    settings.osc1Oct    = (int) raw(pid::osc1Oct);
    settings.osc1Uni    = (int) raw(pid::osc1Uni);
    settings.osc1Det    = raw(pid::osc1Det);
    settings.osc1Spread = raw(pid::osc1Spread);
    settings.osc1PW     = raw(pid::osc1PW);
    settings.osc1Lvl    = raw(pid::osc1Lvl);

    settings.osc2Wave   = (galdr::Wave) (int) raw(pid::osc2Wave);
    settings.osc2Oct    = (int) raw(pid::osc2Oct);
    settings.osc2Semi   = (int) raw(pid::osc2Semi);
    settings.osc2Uni    = (int) raw(pid::osc2Uni);
    settings.osc2Det    = raw(pid::osc2Det);
    settings.osc2Spread = raw(pid::osc2Spread);
    settings.osc2PW     = raw(pid::osc2PW);
    settings.osc2Lvl    = raw(pid::osc2Lvl);

    settings.subWave   = (int) raw(pid::subWave);
    settings.subOct    = (int) raw(pid::subOct);
    settings.subLvl    = raw(pid::subLvl);
    settings.noiseType = (int) raw(pid::noiseType);
    settings.noiseLvl  = raw(pid::noiseLvl);

    settings.filterType  = (int) raw(pid::filterType);
    settings.cutoff      = raw(pid::cutoff);
    settings.resonance   = raw(pid::resonance);
    settings.filterDrive = raw(pid::filterDrive);
    settings.fEnvAmt     = raw(pid::fEnvAmt);
    settings.keytrack    = raw(pid::keytrack);

    settings.vowel = raw(pid::vowel);
    settings.osc1Morph = raw(pid::osc1Morph);
    settings.osc2Morph = raw(pid::osc2Morph);
    settings.bendRangeSemis = raw(pid::bendRange);
    settings.fmAmt = raw(pid::fmAmt);
    settings.oscSync = (int) raw(pid::oscSync);
    settings.driftAmt = raw(pid::driftAmt);
    settings.ampEnv  = { raw(pid::attack), raw(pid::decay), raw(pid::sustain), raw(pid::release) };
    settings.filtEnv = { raw(pid::fAttack), raw(pid::fDecay), raw(pid::fSustain), raw(pid::fRelease) };
    settings.env3    = { raw(pid::env3A), raw(pid::env3D), raw(pid::env3S), raw(pid::env3R) };
    settings.glideSeconds = raw(pid::glide);

    for (int s = 0; s < GaldrVoice::numModSlots; ++s)
    {
        settings.modSrc[s] = (int) raw(pid::modSrc[s]);
        settings.modDst[s] = (int) raw(pid::modDst[s]);
        settings.modAmt[s] = raw(pid::modAmt[s]);
    }
    settings.modWheel = modWheel;
    settings.globalPressure = globalPressure;

    const float blockDt = (float) numSamples / (float) sr;
    auto advance = [blockDt](float& phase, float rate)
    {
        phase += rate * blockDt;
        const bool wrapped = phase >= 1.0f;
        while (phase >= 1.0f)
            phase -= 1.0f;
        return wrapped;
    };

    auto lfoRate = [this](const char* rateId, const char* syncId)
    {
        const float beats = syncBeats((int) raw(syncId));
        return beats > 0.0f ? hostBpm / 60.0f / beats : raw(rateId);
    };

    const bool w1 = advance(lfo1Phase, lfoRate(pid::lfo1Rate, pid::lfo1Sync));
    const float v1 = lfoShapeValue((int) raw(pid::lfo1Shape), lfo1Phase, lfo1Held, w1);
    settings.vibratoFactor = std::exp2(v1 * raw(pid::lfo1Depth) * 100.0f / 1200.0f);

    const bool w2 = advance(lfo2Phase, lfoRate(pid::lfo2Rate, pid::lfo2Sync));
    const float v2 = lfoShapeValue((int) raw(pid::lfo2Shape), lfo2Phase, lfo2Held, w2);
    settings.lfoCutoffOctaves = v2 * raw(pid::lfo2Depth) * 3.0f;

    settings.lfo1Value = v1;
    settings.lfo2Value = v2;
    updateGlobalMods();
}

void GaldrAudioProcessor::scanMidiControllers(const juce::MidiBuffer& midi)
{
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isController())
        {
            const int cc = msg.getControllerNumber();
            if (cc == 1)
                modWheel = (float) msg.getControllerValue() / 127.0f;

            if (auto* armed = midiLearnTarget.exchange(nullptr))
                midiCCMap[cc].store(armed);
            if (auto* mapped = midiCCMap[cc].load())
                mapped->setValueNotifyingHost((float) msg.getControllerValue() / 127.0f);
        }
        else if (msg.isChannelPressure())
            globalPressure = (float) msg.getChannelPressureValue() / 127.0f;
    }
}

// Global-destination matrix slots use monophonic source values: the LFOs and
// wheel directly, envelopes/velocity/random from the most recently started voice.
void GaldrAudioProcessor::updateGlobalMods()
{
    GaldrVoice* newest = nullptr;
    juce::uint32 bestSerial = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<GaldrVoice*>(synth.getVoice(i)))
            if (voice->isVoiceActive() && voice->getStartSerial() >= bestSerial)
            {
                bestSerial = voice->getStartSerial();
                newest = voice;
            }
    if (newest != nullptr)
    {
        lastVelocity = newest->getVelocity01();
        lastRandom   = newest->getRandom01();
        lastEnv3     = newest->getEnv3Value();
        lastFiltEnv  = newest->getFiltEnvValue();
    }
    else
    {
        lastEnv3 = lastFiltEnv = 0.0f;
    }

    auto sourceValue = [this](int src) -> float
    {
        switch (src)
        {
            case GaldrVoice::srcLfo1:     return settings.lfo1Value;
            case GaldrVoice::srcLfo2:     return settings.lfo2Value;
            case GaldrVoice::srcFiltEnv:  return lastFiltEnv;
            case GaldrVoice::srcEnv3:     return lastEnv3;
            case GaldrVoice::srcVelocity: return lastVelocity;
            case GaldrVoice::srcModWheel: return modWheel;
            case GaldrVoice::srcPressure: return globalPressure;
            case GaldrVoice::srcRandom:   return lastRandom;
            default:                      return 0.0f;
        }
    };

    rmFreqModOct = tremDepthMod = delayMixMod = revMixMod = bzLvlMod = 0.0f;
    for (int s = 0; s < GaldrVoice::numModSlots; ++s)
    {
        const int dst = settings.modDst[s];
        if (dst < GaldrVoice::firstGlobalDest)
            continue;
        const float v = sourceValue(settings.modSrc[s]) * settings.modAmt[s];
        switch (dst)
        {
            case GaldrVoice::dstRmFreq:    rmFreqModOct += v * 2.0f; break;
            case GaldrVoice::dstTremDepth: tremDepthMod += v;        break;
            case GaldrVoice::dstDelayMix:  delayMixMod += v;         break;
            case GaldrVoice::dstRevMix:    revMixMod += v;           break;
            case GaldrVoice::dstBzLvl:     bzLvlMod += v;            break;
            default: break;
        }
    }
}

// Consumes held notes and emits stepped note events locked to the host tempo.
void GaldrAudioProcessor::processArp(juce::MidiBuffer& midi, int numSamples)
{
    const int mode = (int) raw(pid::arpMode);

    if (mode == 0)
    {
        if (arp.lastNote >= 0)
        {
            midi.addEvent(juce::MidiMessage::noteOff(1, arp.lastNote), 0);
            arp.lastNote = -1;
            arp.gateSamplesLeft = -1;
        }
        arp.held.clear();
        arp.heldOrder.clearQuick();
        return;
    }

    juce::MidiBuffer passthrough;
    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            const int note = msg.getNoteNumber();
            arp.held.add(note);
            arp.heldOrder.addIfNotAlreadyThere(note);
            arp.velocities[note] = msg.getFloatVelocity();
        }
        else if (msg.isNoteOff())
        {
            arp.held.removeValue(msg.getNoteNumber());
            arp.heldOrder.removeFirstMatchingValue(msg.getNoteNumber());
        }
        else
        {
            passthrough.addEvent(msg, metadata.samplePosition);
        }
    }
    midi.swapWith(passthrough);

    const float beats = arpBeats((int) raw(pid::arpRate));
    const int interval = juce::jmax(64, (int) (beats * 60.0f / hostBpm * (float) sr));
    const int gateLen = juce::jmax(32, (int) ((float) interval * raw(pid::arpGate)));
    const int octaves = (int) raw(pid::arpOct);

    auto emitNoteOff = [this, &midi](int position)
    {
        midi.addEvent(juce::MidiMessage::noteOff(1, arp.lastNote), position);
        arp.lastNote = -1;
        arp.gateSamplesLeft = -1;
    };

    int pos = 0;
    while (pos < numSamples)
    {
        int next = numSamples;
        if (arp.gateSamplesLeft >= 0)
            next = juce::jmin(next, pos + arp.gateSamplesLeft);
        next = juce::jmin(next, pos + arp.samplesToNext);

        const int advanceBy = next - pos;
        if (arp.gateSamplesLeft >= 0)
            arp.gateSamplesLeft -= advanceBy;
        arp.samplesToNext -= advanceBy;
        pos = next;

        const int eventPos = juce::jmin(pos, numSamples - 1);

        if (arp.gateSamplesLeft == 0 && arp.lastNote >= 0)
            emitNoteOff(eventPos);

        if (arp.samplesToNext <= 0)
        {
            if (arp.lastNote >= 0)
                emitNoteOff(eventPos);

            if (! arp.held.isEmpty())
            {
                // expand held notes across octaves in the requested order
                juce::Array<int> pattern;
                auto addExpanded = [&pattern, octaves](const juce::Array<int>& base)
                {
                    for (int oct = 0; oct < octaves; ++oct)
                        for (int note : base)
                            pattern.add(juce::jmin(127, note + 12 * oct));
                };

                juce::Array<int> ascending;
                for (int i = 0; i < arp.held.size(); ++i)
                    ascending.add(arp.held[i]);

                switch (mode)
                {
                    case 1: addExpanded(ascending); break;                       // up
                    case 2:
                    {
                        juce::Array<int> descending(ascending);
                        std::reverse(descending.begin(), descending.end());
                        addExpanded(descending);
                        break;                                                   // down
                    }
                    case 3:
                    {
                        addExpanded(ascending);
                        juce::Array<int> back(pattern);
                        std::reverse(back.begin(), back.end());
                        for (int i = 1; i < back.size() - 1; ++i)
                            pattern.add(back[i]);
                        break;                                                   // up-down
                    }
                    case 5: addExpanded(arp.heldOrder); break;                   // as played
                    default: addExpanded(ascending); break;                      // random picks below
                }

                if (! pattern.isEmpty())
                {
                    int index;
                    if (mode == 4)
                        index = arp.rng.nextInt(pattern.size());
                    else
                        index = arp.step % pattern.size();
                    ++arp.step;

                    const int note = pattern[index];
                    const float vel = arp.velocities[juce::jlimit(0, 127, note % 128) % 128] > 0.0f
                                          ? arp.velocities[juce::jlimit(0, 127, note) % 128]
                                          : 0.8f;
                    midi.addEvent(juce::MidiMessage::noteOn(1, note,
                                      vel > 0.0f ? vel : 0.8f), eventPos);
                    arp.lastNote = note;
                    arp.gateSamplesLeft = gateLen;
                }
            }
            arp.samplesToNext = interval;
        }
    }
}

void GaldrAudioProcessor::applyDistortion(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int type = (int) raw(pid::distType);
    const float drive = raw(pid::distDrive);
    const float mix = raw(pid::distMix);
    if (mix < 0.001f)
        return;

    const float toneCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi
                                            * raw(pid::distTone) / (float) sampleRate);

    for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float z = toneState[ch];
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float x = data[i];
            float wet = galdr::distort(type, x, drive) * 0.8f;
            z += toneCoeff * (wet - z);
            data[i] = x * (1.0f - mix) + z * mix;
        }
        toneState[ch] = z;
    }
}

void GaldrAudioProcessor::applyCrusher(juce::AudioBuffer<float>& buffer, int osFactor)
{
    const int bits = (int) raw(pid::crushBits);
    const int rate = (int) raw(pid::crushRate) * osFactor;
    const float mix = raw(pid::crushMix);
    if ((bits >= 16 && rate <= osFactor) || mix < 0.001f)
        return;

    const float quant = std::exp2((float) (bits - 1));

    for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float held = crushHeld[ch];
        int count = crushCount[ch];
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (count++ % rate == 0)
                held = std::round(data[i] * quant) / quant;
            data[i] = data[i] * (1.0f - mix) + held * mix;
        }
        crushHeld[ch] = held;
        crushCount[ch] = count % juce::jmax(1, rate);
    }
}

void GaldrAudioProcessor::applyRingMod(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const float mix = raw(pid::rmMix);
    const float inc = raw(pid::rmFreq) * std::exp2(rmFreqModOct) / (float) sampleRate;
    const int n = buffer.getNumSamples();

    if (mix < 0.001f)
    {
        rmPhase = std::fmod(rmPhase + inc * (float) n, 1.0f);
        return;
    }

    const int channels = buffer.getNumChannels();
    for (int i = 0; i < n; ++i)
    {
        rmPhase += inc;
        if (rmPhase >= 1.0f)
            rmPhase -= 1.0f;
        const float carrier = std::sin(rmPhase * juce::MathConstants<float>::twoPi);
        for (int ch = 0; ch < channels; ++ch)
        {
            auto& s = buffer.getWritePointer(ch)[i];
            s = s * (1.0f - mix) + s * carrier * mix;
        }
    }
}

void GaldrAudioProcessor::applyTremolo(juce::AudioBuffer<float>& buffer)
{
    const float depth = juce::jlimit(0.0f, 1.0f, raw(pid::tremDepth) + tremDepthMod);
    const float tremBeats = syncBeats((int) raw(pid::tremSync));
    const float rate = tremBeats > 0.0f ? hostBpm / 60.0f / tremBeats : raw(pid::tremRate);
    const bool sine = (int) raw(pid::tremShape) == 1;
    const float inc = rate / (float) sr;

    if (depth < 0.001f)
    {
        tremPhase = std::fmod(tremPhase + inc * (float) buffer.getNumSamples(), 1.0f);
        tremSmooth = 1.0f;
        return;
    }

    const int channels = buffer.getNumChannels();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        tremPhase += inc;
        if (tremPhase >= 1.0f)
            tremPhase -= 1.0f;

        const float target = sine
            ? 1.0f - depth * (0.5f - 0.5f * std::cos(tremPhase * juce::MathConstants<float>::twoPi))
            : (tremPhase < 0.5f ? 1.0f : 1.0f - depth);
        tremSmooth += tremCoeff * (target - tremSmooth);

        for (int ch = 0; ch < channels; ++ch)
            buffer.getWritePointer(ch)[i] *= tremSmooth;
    }
}

void GaldrAudioProcessor::applyDelay(juce::AudioBuffer<float>& buffer)
{
    const float mix = juce::jlimit(0.0f, 1.0f, raw(pid::delayMix) + delayMixMod);
    const float fb = raw(pid::delayFb);
    const float delayBeats = syncBeats((int) raw(pid::delaySync));
    const float timeSeconds = delayBeats > 0.0f
        ? juce::jlimit(0.02f, 2.0f, delayBeats * 60.0f / hostBpm)
        : raw(pid::delayTime);
    delaySamplesSm.setTargetValue(timeSeconds * (float) sr);

    const int channels = juce::jmin(2, buffer.getNumChannels());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float d = delaySamplesSm.getNextValue();
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            const float in = data[i];
            const float rd = delayLine.popSample(ch, d, true);
            delayLine.pushSample(ch, in + rd * fb);
            data[i] = in + rd * mix;
        }
    }
}

void GaldrAudioProcessor::applyReverbAndGain(juce::AudioBuffer<float>& buffer)
{
    const float mix = juce::jlimit(0.0f, 1.0f, raw(pid::revMix) + revMixMod);
    darkReverb.setParams(raw(pid::revSize), raw(pid::revDamp), raw(pid::revWidth),
                         raw(pid::revPre), raw(pid::revShimmer));
    if (mix > 0.001f)
        darkReverb.process(buffer, mix);

    const int n = buffer.getNumSamples();
    gainSm.setTargetValue(raw(pid::gain));
    for (int i = 0; i < n; ++i)
    {
        const float g = gainSm.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto& s = buffer.getWritePointer(ch)[i];
            s = std::tanh(s * g);
        }
    }
}

void GaldrAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    if (auto* head = getPlayHead())
        if (auto position = head->getPosition())
            if (auto bpm = position->getBpm())
                hostBpm = juce::jlimit(20.0f, 999.0f, (float) *bpm);

    keyboardState.processNextMidiBuffer(midiMessages, 0, numSamples, true);
    buffer.clear();

    scanMidiControllers(midiMessages);
    updateSettings(numSamples);
    processArp(midiMessages, numSamples);
    synth.renderNextBlock(buffer, midiMessages, 0, numSamples);

    // Gated mode: the snowstorm only blows while notes are sounding, with a
    // soft ramp so it breathes with the tails instead of cutting off.
    {
        bool anyVoiceActive = false;
        for (int i = 0; i < synth.getNumVoices() && ! anyVoiceActive; ++i)
            anyVoiceActive = synth.getVoice(i)->isVoiceActive();

        const bool freeRunning = (int) raw(pid::bzGate) == 1;
        const float target = (freeRunning || anyVoiceActive) ? 1.0f : 0.0f;
        const float tau = target > bzGateEnv ? 0.04f : 0.25f;
        bzGateEnv += (1.0f - std::exp(-(float) numSamples / ((float) sr * tau)))
                     * (target - bzGateEnv);

        blizzard.process(buffer, raw(pid::bzDensity), raw(pid::bzSize),
                         raw(pid::bzPitch), raw(pid::bzSpread),
                         juce::jlimit(0.0f, 1.0f, raw(pid::bzLvl) + bzLvlMod) * bzGateEnv);
    }

    // The nonlinear stages run oversampled at 2x to keep aliasing down.
    {
        juce::dsp::AudioBlock<float> block(buffer);
        auto osBlock = oversampling->processSamplesUp(block);
        float* chans[2] = { osBlock.getChannelPointer(0),
                            osBlock.getNumChannels() > 1 ? osBlock.getChannelPointer(1)
                                                         : osBlock.getChannelPointer(0) };
        juce::AudioBuffer<float> osBuffer(chans, (int) osBlock.getNumChannels(),
                                          (int) osBlock.getNumSamples());
        const double osRate = sr * 2.0;
        applyDistortion(osBuffer, osRate);
        applyCrusher(osBuffer, 2);
        applyRingMod(osBuffer, osRate);
        oversampling->processSamplesDown(block);
    }

    {
        chorus.setRate(raw(pid::chorusRate));
        chorus.setDepth(raw(pid::chorusDepth));
        chorus.setMix(raw(pid::chorusMix));
        juce::dsp::AudioBlock<float> block(buffer);
        chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    applyTremolo(buffer);
    applyDelay(buffer);
    applyReverbAndGain(buffer);

    const auto* readL = buffer.getReadPointer(0);
    const auto* readR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : readL;
    scopeFifo.push(readL, readR, numSamples, buffer.getNumChannels());
    spectrumFifo.push(readL, readR, numSamples, buffer.getNumChannels());
}

void GaldrAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream(destData, false);
    captureFullState().writeToStream(stream);
}

void GaldrAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto tree = juce::ValueTree::readFromData(data, (size_t) sizeInBytes); tree.isValid())
        applyStateTree(tree);
}

juce::ValueTree GaldrAudioProcessor::capturePresetState()
{
    auto tree = apvts.copyState();
    tree.setProperty("stateVersion", stateVersion, nullptr);
    if (auto map = tree.getChildWithName("MIDIMAP"); map.isValid())
        tree.removeChild(map, nullptr);
    return tree;
}

juce::ValueTree GaldrAudioProcessor::captureFullState()
{
    auto tree = capturePresetState();
    juce::ValueTree map("MIDIMAP");
    for (int cc = 0; cc < 128; ++cc)
        if (auto* p = midiCCMap[cc].load())
            map.appendChild(juce::ValueTree("MAP", { { "cc", cc }, { "param", p->paramID } }), nullptr);
    if (map.getNumChildren() > 0)
        tree.appendChild(map, nullptr);
    return tree;
}

// Migrations for states written by older builds. Old v0 states (no version
// property) are already compatible parameter-wise; the hook exists so future
// parameter renames or range changes never break saved sessions or presets.
void GaldrAudioProcessor::migrateState(juce::ValueTree& state, int fromVersion)
{
    juce::ignoreUnused(state, fromVersion);
}

void GaldrAudioProcessor::applyStateTree(juce::ValueTree tree)
{
    if (! tree.isValid())
        return;

    migrateState(tree, (int) tree.getProperty("stateVersion", 0));

    // Restore CC mappings only when the state carries them (host sessions do,
    // preset files don't: loading a sound must not clobber the controller setup).
    if (auto map = tree.getChildWithName("MIDIMAP"); map.isValid())
    {
        for (auto& slot : midiCCMap)
            slot.store(nullptr);
        for (const auto& m : map)
        {
            const int cc = (int) m.getProperty("cc", -1);
            if (cc >= 0 && cc < 128)
                midiCCMap[cc].store(apvts.getParameter(m.getProperty("param").toString()));
        }
        tree.removeChild(map, nullptr);
    }
    midiLearnTarget.store(nullptr);

    apvts.replaceState(tree);

    const auto tuningData = apvts.state.getProperty("tuningData").toString();
    const auto tuningName = apvts.state.getProperty("tuningName").toString();
    const auto tuningPath = apvts.state.getProperty("tuningFile").toString();
    const bool embedded = tuningData.isNotEmpty()
        && tuning.loadSclText(tuningData, tuningName.isNotEmpty() ? tuningName : "Custom");
    if (! embedded)
    {
        if (tuningPath.isNotEmpty() && juce::File(tuningPath).existsAsFile())
            tuning.loadScl(juce::File(tuningPath)); // legacy pre-v1 states stored the path only
        else
            tuning.reset();
    }

    presetDirty.store(false);
    undoManager.clearUndoHistory();
}

bool GaldrAudioProcessor::loadTuning(const juce::File& sclFile)
{
    galdr::Tuning candidate;
    const auto text = sclFile.loadFileAsString();
    if (! candidate.loadSclText(text, sclFile.getFileNameWithoutExtension()))
        return false;
    tuning = candidate;
    apvts.state.setProperty("tuningFile", sclFile.getFullPathName(), nullptr);
    apvts.state.setProperty("tuningData", text, nullptr);
    apvts.state.setProperty("tuningName", tuning.name, nullptr);
    return true;
}

void GaldrAudioProcessor::resetTuning()
{
    tuning.reset();
    apvts.state.removeProperty("tuningFile", nullptr);
    apvts.state.removeProperty("tuningData", nullptr);
    apvts.state.removeProperty("tuningName", nullptr);
}

void GaldrAudioProcessor::armMidiLearn(const juce::String& paramID)
{
    midiLearnTarget.store(apvts.getParameter(paramID));
}

int GaldrAudioProcessor::midiCCFor(const juce::String& paramID) const
{
    for (int cc = 0; cc < 128; ++cc)
        if (auto* p = midiCCMap[cc].load(); p != nullptr && p->paramID == paramID)
            return cc;
    return -1;
}

void GaldrAudioProcessor::clearMidiCC(const juce::String& paramID)
{
    for (int cc = 0; cc < 128; ++cc)
        if (auto* p = midiCCMap[cc].load(); p != nullptr && p->paramID == paramID)
            midiCCMap[cc].store(nullptr);
}

juce::AudioProcessorEditor* GaldrAudioProcessor::createEditor()
{
    return new GaldrAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GaldrAudioProcessor();
}

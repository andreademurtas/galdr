// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "GaldrDSP.h"

struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class GaldrVoice : public juce::SynthesiserVoice
{
public:
    // Written by the processor once per block, read by every voice.
    struct Settings
    {
        galdr::Wave osc1Wave = galdr::Wave::saw, osc2Wave = galdr::Wave::saw;
        int   osc1Oct = 0, osc2Oct = 0, osc2Semi = 0;
        int   osc1Uni = 1, osc2Uni = 1;
        float osc1Det = 0.0f, osc2Det = 0.0f;
        float osc1Spread = 0.0f, osc2Spread = 0.0f;
        float osc1PW = 0.5f, osc2PW = 0.5f;
        float osc1Lvl = 0.8f, osc2Lvl = 0.0f;
        int   subWave = 0, subOct = 0;
        float subLvl = 0.0f;
        int   noiseType = 0;
        float noiseLvl = 0.0f;
        float osc1Morph = 0.0f, osc2Morph = 0.0f; // wavetable position

        const float* noteFreqs = nullptr;  // 128-entry tuning table (nullptr = 12-TET)
        float bendRangeSemis = 2.0f;

        int   filterType = 0;
        float cutoff = 12000.0f, resonance = 0.2f, filterDrive = 0.0f;
        float fEnvAmt = 0.0f, keytrack = 0.0f;
        float vowel = 0.0f; // formant mode: 0..1 morphs A-E-I-O-U

        juce::ADSR::Parameters ampEnv, filtEnv;
        float glideSeconds = 0.0f;

        float vibratoFactor = 1.0f;    // pitch factor from LFO1, updated per block
        float lfoCutoffOctaves = 0.0f; // cutoff offset from LFO2, updated per block
    };

    explicit GaldrVoice(const Settings& s) : settings(s) {}

    void prepare(double sampleRate, int maxBlockSize)
    {
        voiceBuffer.setSize(2, maxBlockSize);
        const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 2 };
        filter1.prepare(spec);
        filter2.prepare(spec);
        filter3.prepare(spec);
    }

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SynthSound*>(s) != nullptr;
    }

    float noteFrequency(int midiNote) const
    {
        if (settings.noteFreqs != nullptr)
            return settings.noteFreqs[juce::jlimit(0, 127, midiNote)];
        return (float) juce::MidiMessage::getMidiNoteInHertz(midiNote);
    }

    // Legato pitch change: retune without retriggering the envelopes.
    void slideTo(int midiNote)
    {
        note = midiNote;
        targetFreq = noteFrequency(midiNote);
    }

    void startNote(int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        note = midiNote;
        level = 0.1f + velocity * 0.15f;
        pressure = 0.0f;
        targetFreq = noteFrequency(midiNote);
        if (currentFreq <= 0.0f || settings.glideSeconds <= 0.0001f)
            currentFreq = targetFreq;

        osc1.randomisePhases(rng);
        osc2.randomisePhases(rng);
        subOsc.phase = 0.0f;

        const auto sr = getSampleRate();
        ampAdsr.setSampleRate(sr);
        ampAdsr.setParameters(settings.ampEnv);
        filtAdsr.setSampleRate(sr);
        filtAdsr.setParameters(settings.filtEnv);
        ampAdsr.noteOn();
        filtAdsr.noteOn();

        filter1.reset();
        filter2.reset();
        filter3.reset();
        updateFilterType();
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            ampAdsr.noteOff();
            filtAdsr.noteOff();
        }
        else
        {
            ampAdsr.reset();
            filtAdsr.reset();
            clearCurrentNote();
        }
    }

    // MPE-friendly: per-channel bend and pressure reach the voices playing
    // that channel, so MPE controllers get per-note expression.
    void pitchWheelMoved(int value) override
    {
        bendFactor = std::exp2((float) (value - 8192) / 8192.0f * settings.bendRangeSemis / 12.0f);
    }

    void channelPressureChanged(int value) override { pressure = (float) value / 127.0f; }
    void aftertouchChanged(int value) override      { pressure = (float) value / 127.0f; }
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        const float sr = (float) getSampleRate();
        updateFilterType();

        const float glideCoeff = settings.glideSeconds > 0.0001f
                                     ? std::exp(-1.0f / (settings.glideSeconds * sr))
                                     : 0.0f;
        const float driveGain = 1.0f + settings.filterDrive * 7.0f;

        voiceBuffer.clear(0, 0, numSamples);
        voiceBuffer.clear(1, 0, numSamples);
        auto* L = voiceBuffer.getWritePointer(0);
        auto* R = voiceBuffer.getWritePointer(1);

        const float o1Mult  = std::exp2((float) settings.osc1Oct) * settings.vibratoFactor * bendFactor;
        const float o2Mult  = std::exp2((float) settings.osc2Oct + (float) settings.osc2Semi / 12.0f)
                              * settings.vibratoFactor * bendFactor;
        const float subMult = std::exp2((float) (-1 - settings.subOct)) * bendFactor;
        const auto* wavetable = &galdr::Wavetable::global();

        int n = 0;
        bool finished = false;

        for (; n < numSamples; ++n)
        {
            currentFreq = targetFreq + (currentFreq - targetFreq) * glideCoeff;

            const float fEnv = filtAdsr.getNextSample();
            const float aEnv = ampAdsr.getNextSample();

            if ((n & 15) == 0)
                updateCutoff(fEnv, sr);

            float l = 0.0f, r = 0.0f, sl = 0.0f, srr = 0.0f;

            if (settings.osc1Lvl > 0.0001f)
            {
                osc1.next(settings.osc1Wave, settings.osc1Uni, settings.osc1Det,
                          settings.osc1Spread, settings.osc1PW,
                          currentFreq * o1Mult, sr, wavetable, settings.osc1Morph, sl, srr);
                l += sl * settings.osc1Lvl;
                r += srr * settings.osc1Lvl;
            }

            if (settings.osc2Lvl > 0.0001f)
            {
                osc2.next(settings.osc2Wave, settings.osc2Uni, settings.osc2Det,
                          settings.osc2Spread, settings.osc2PW,
                          currentFreq * o2Mult, sr, wavetable, settings.osc2Morph, sl, srr);
                l += sl * settings.osc2Lvl;
                r += srr * settings.osc2Lvl;
            }

            if (settings.subLvl > 0.0001f)
            {
                const float dt = juce::jmin(0.45f, currentFreq * subMult / sr);
                const float s = subOsc.next(settings.subWave == 0 ? galdr::Wave::sine
                                                                  : galdr::Wave::square,
                                            dt, 0.5f) * settings.subLvl;
                l += s;
                r += s;
            }

            if (settings.noiseLvl > 0.0001f)
            {
                float w = rng.nextFloat() * 2.0f - 1.0f;
                if (settings.noiseType == 1)
                    w = pink.next(w);
                w *= settings.noiseLvl * 0.7f;
                l += w;
                r += w;
            }

            l = std::tanh(l * driveGain);
            r = std::tanh(r * driveGain);
            if (formantMode)
            {
                const float inL = l, inR = r;
                l = 1.5f * (filter1.processSample(0, inL)
                            + 0.5f * filter2.processSample(0, inL)
                            + 0.25f * filter3.processSample(0, inL));
                r = 1.5f * (filter1.processSample(1, inR)
                            + 0.5f * filter2.processSample(1, inR)
                            + 0.25f * filter3.processSample(1, inR));
            }
            else
            {
                l = filter1.processSample(0, l);
                r = filter1.processSample(1, r);
                if (use24dB)
                {
                    l = filter2.processSample(0, l);
                    r = filter2.processSample(1, r);
                }
            }

            const float pressGain = 1.0f + pressure * 0.4f;
            L[n] = l * level * aEnv * pressGain;
            R[n] = r * level * aEnv * pressGain;

            if (! ampAdsr.isActive())
            {
                finished = true;
                ++n;
                break;
            }
        }

        if (output.getNumChannels() >= 2)
        {
            output.addFrom(0, startSample, voiceBuffer, 0, 0, n);
            output.addFrom(1, startSample, voiceBuffer, 1, 0, n);
        }
        else
        {
            auto* out = output.getWritePointer(0) + startSample;
            for (int i = 0; i < n; ++i)
                out[i] += 0.5f * (L[i] + R[i]);
        }

        if (finished)
            clearCurrentNote();
    }

private:
    void updateFilterType()
    {
        using T = juce::dsp::StateVariableTPTFilterType;
        formantMode = settings.filterType == 4;
        use24dB = settings.filterType == 0;
        T t = T::lowpass;
        if (formantMode || settings.filterType == 3) t = T::bandpass;
        else if (settings.filterType == 2)           t = T::highpass;
        filter1.setType(t);
        filter2.setType(t);
        filter3.setType(T::bandpass);
    }

    void updateCutoff(float fEnv, float sr)
    {
        const float modOctaves = fEnv * settings.fEnvAmt * 4.0f
                                 + settings.lfoCutoffOctaves
                                 + pressure * 1.2f
                                 + (float) (note - 60) / 12.0f * settings.keytrack;
        const float maxFreq = juce::jmin(20000.0f, sr * 0.49f);

        if (formantMode)
        {
            // First three formants of A / E / I / O / U, morphed by the vowel param.
            static constexpr float formantFreq[5][3] = {
                { 800.0f, 1150.0f, 2900.0f },   // A
                { 400.0f, 1600.0f, 2700.0f },   // E
                { 350.0f, 1700.0f, 2700.0f },   // I
                { 450.0f,  800.0f, 2830.0f },   // O
                { 325.0f,  700.0f, 2700.0f } }; // U

            const float pos = juce::jlimit(0.0f, 1.0f, settings.vowel) * 4.0f;
            const int i0 = juce::jmin(3, (int) pos);
            const float frac = pos - (float) i0;

            // The cutoff knob shifts the whole formant set (1 kHz = neutral).
            const float shift = (settings.cutoff / 1000.0f) * std::exp2(modOctaves);
            const float q = 4.0f + settings.resonance * 8.0f;

            juce::dsp::StateVariableTPTFilter<float>* filters[3] { &filter1, &filter2, &filter3 };
            for (int k = 0; k < 3; ++k)
            {
                const float f = juce::jlimit(20.0f, maxFreq,
                    (formantFreq[i0][k] + frac * (formantFreq[i0 + 1][k] - formantFreq[i0][k])) * shift);
                filters[k]->setCutoffFrequency(f);
                filters[k]->setResonance(q);
            }
            return;
        }

        const float c = juce::jlimit(20.0f, maxFreq, settings.cutoff * std::exp2(modOctaves));
        const float q = 0.5f + settings.resonance * 7.5f;
        filter1.setCutoffFrequency(c);
        filter1.setResonance(q);
        if (use24dB)
        {
            filter2.setCutoffFrequency(c);
            filter2.setResonance(q);
        }
    }

    const Settings& settings;

    galdr::UnisonOsc osc1, osc2;
    galdr::BlepOsc subOsc;
    galdr::PinkFilter pink;
    juce::Random rng;

    juce::ADSR ampAdsr, filtAdsr;
    juce::dsp::StateVariableTPTFilter<float> filter1, filter2, filter3;
    juce::AudioBuffer<float> voiceBuffer;

    int note = 60;
    float level = 0.0f;
    float currentFreq = 0.0f, targetFreq = 440.0f;
    float bendFactor = 1.0f, pressure = 0.0f;
    bool use24dB = true;
    bool formantMode = false;
};

// Synthesiser with poly / mono / legato modes. Mono keeps a note stack with
// last-note priority; legato retunes the running voice instead of retriggering.
class GaldrSynth : public juce::Synthesiser
{
public:
    enum Mode { poly = 0, mono, legato };

    void setMode(int newMode)
    {
        const juce::ScopedLock sl(lock);
        if (mode != newMode)
        {
            mode = newMode;
            heldNotes.clearQuick();
            allNotesOff(0, true);
        }
    }

    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override
    {
        if (mode == poly)
        {
            juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
            return;
        }
        const juce::ScopedLock sl(lock);
        heldNotes.add({ midiNoteNumber, velocity });
        trigger(midiChannel, midiNoteNumber, velocity, heldNotes.size() > 1);
    }

    void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override
    {
        if (mode == poly)
        {
            juce::Synthesiser::noteOff(midiChannel, midiNoteNumber, velocity, allowTailOff);
            return;
        }
        const juce::ScopedLock sl(lock);
        for (int i = heldNotes.size(); --i >= 0;)
            if (heldNotes.getReference(i).note == midiNoteNumber)
                heldNotes.remove(i);

        if (auto* voice = dynamic_cast<GaldrVoice*>(getVoice(0)))
        {
            if (heldNotes.isEmpty())
                voice->stopNote(velocity, allowTailOff);
            else
                trigger(midiChannel, heldNotes.getLast().note, heldNotes.getLast().velocity, true);
        }
    }

private:
    struct Held
    {
        int note;
        float velocity;
    };

    void trigger(int midiChannel, int note, float velocity, bool canLegato)
    {
        auto* voice = dynamic_cast<GaldrVoice*>(getVoice(0));
        if (voice == nullptr || getNumSounds() == 0)
            return;
        if (mode == legato && canLegato && voice->isVoiceActive())
            voice->slideTo(note);
        else
            startVoice(voice, getSound(0).get(), midiChannel, note, velocity);
    }

    int mode = poly;
    juce::Array<Held> heldNotes;
};

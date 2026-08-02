// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "GaldrDSP.h"

struct SynthSound : public juce::SynthesiserSound
{
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

class GaldrVoice : public juce::SynthesiserVoice
{
public:
    // Mod matrix source / destination indices (order matches the choice params).
    enum ModSource { srcNone = 0, srcLfo1, srcLfo2, srcFiltEnv, srcEnv3,
                     srcVelocity, srcModWheel, srcPressure, srcRandom };
    enum ModDest { dstNone = 0, dstPitch, dstCutoff, dstReso, dstVowel,
                   dstMorph1, dstMorph2, dstPW1, dstPW2, dstFM, dstNoise,
                   dstRmFreq, dstTremDepth, dstDelayMix, dstRevMix, dstBzLvl };
    static constexpr int numModSlots = 8;
    static constexpr int firstGlobalDest = dstRmFreq;

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

        float fmAmt = 0.0f;   // osc2 -> osc1 frequency modulation
        int   oscSync = 0;    // hard-sync osc1 to osc2's cycle
        float driftAmt = 0.0f;

        const float* noteFreqs = nullptr;  // 128-entry tuning table (nullptr = 12-TET)
        float bendRangeSemis = 2.0f;

        int   filterType = 0;
        float cutoff = 12000.0f, resonance = 0.2f, filterDrive = 0.0f;
        float fEnvAmt = 0.0f, keytrack = 0.0f;
        float vowel = 0.0f; // formant mode: 0..1 morphs A-E-I-O-U

        juce::ADSR::Parameters ampEnv, filtEnv, env3;
        float glideSeconds = 0.0f;

        float vibratoFactor = 1.0f;    // pitch factor from LFO1, updated per block
        float lfoCutoffOctaves = 0.0f; // cutoff offset from LFO2, updated per block

        // mod matrix state, refreshed per block by the processor
        int   modSrc[numModSlots] {};
        int   modDst[numModSlots] {};
        float modAmt[numModSlots] {};
        float lfo1Value = 0.0f, lfo2Value = 0.0f;
        float modWheel = 0.0f, globalPressure = 0.0f;

        std::atomic<juce::uint32>* noteCounter = nullptr;
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
        velocity01 = velocity;
        level = 0.1f + velocity * 0.15f;
        pressure = 0.0f;
        random01 = rng.nextFloat() * 2.0f - 1.0f;
        if (settings.noteCounter != nullptr)
            startSerial = ++(*settings.noteCounter);

        targetFreq = noteFrequency(midiNote);
        if (currentFreq <= 0.0f || settings.glideSeconds <= 0.0001f)
            currentFreq = targetFreq;

        osc1.randomisePhases(rng);
        osc2.randomisePhases(rng);
        subOsc.phase = 0.0f;
        fmModOsc.phase = 0.0f;
        prevModPhase = 0.0f;

        const auto sr = getSampleRate();
        ampAdsr.setSampleRate(sr);
        ampAdsr.setParameters(settings.ampEnv);
        filtAdsr.setSampleRate(sr);
        filtAdsr.setParameters(settings.filtEnv);
        env3Adsr.setSampleRate(sr);
        env3Adsr.setParameters(settings.env3);
        ampAdsr.noteOn();
        filtAdsr.noteOn();
        env3Adsr.noteOn();

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
            env3Adsr.noteOff();
        }
        else
        {
            ampAdsr.reset();
            filtAdsr.reset();
            env3Adsr.reset();
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

    juce::uint32 getStartSerial() const { return startSerial; }
    float getVelocity01() const { return velocity01; }
    float getRandom01() const { return random01; }
    float getEnv3Value() const { return env3Last; }
    float getFiltEnvValue() const { return filtEnvLast; }

    void renderNextBlock(juce::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        const float sr = (float) getSampleRate();
        updateFilterType();
        computeMods();

        const float glideCoeff = settings.glideSeconds > 0.0001f
                                     ? std::exp(-1.0f / (settings.glideSeconds * sr))
                                     : 0.0f;
        const float driveGain = 1.0f + settings.filterDrive * 7.0f;

        voiceBuffer.clear(0, 0, numSamples);
        voiceBuffer.clear(1, 0, numSamples);
        auto* L = voiceBuffer.getWritePointer(0);
        auto* R = voiceBuffer.getWritePointer(1);

        const float pitchMods = settings.vibratoFactor * bendFactor * matrixPitchFactor * driftFactor;
        const float o1Mult  = std::exp2((float) settings.osc1Oct) * pitchMods;
        const float o2Mult  = std::exp2((float) settings.osc2Oct + (float) settings.osc2Semi / 12.0f)
                              * pitchMods;
        const float subMult = std::exp2((float) (-1 - settings.subOct)) * bendFactor
                              * matrixPitchFactor * driftFactor;
        const auto* wavetable = &galdr::Wavetable::global();

        const bool useFmOsc = effFm > 0.0001f || settings.oscSync == 1;
        const bool syncOn = settings.oscSync == 1;

        int n = 0;
        bool finished = false;

        for (; n < numSamples; ++n)
        {
            currentFreq = targetFreq + (currentFreq - targetFreq) * glideCoeff;

            const float fEnv = filtAdsr.getNextSample();
            const float aEnv = ampAdsr.getNextSample();
            env3Last = env3Adsr.getNextSample();
            filtEnvLast = fEnv;

            if ((n & 15) == 0)
                updateCutoff(fEnv, sr);

            float l = 0.0f, r = 0.0f, sl = 0.0f, srr = 0.0f;

            float fmFactor = 1.0f;
            if (useFmOsc)
            {
                const float dtM = juce::jmin(0.45f, currentFreq * o2Mult / sr);
                const float modSample = fmModOsc.next(settings.osc2Wave, dtM, effPW2,
                                                      wavetable, effMorph2);
                const bool wrapped = fmModOsc.phase < prevModPhase;
                prevModPhase = fmModOsc.phase;
                if (syncOn && wrapped)
                    osc1.resetPhases();
                fmFactor = juce::jmax(0.02f, 1.0f + 3.0f * effFm * modSample);
            }

            if (settings.osc1Lvl > 0.0001f)
            {
                osc1.next(settings.osc1Wave, settings.osc1Uni, settings.osc1Det,
                          settings.osc1Spread, effPW1,
                          currentFreq * o1Mult * fmFactor, sr, wavetable, effMorph1, sl, srr);
                l += sl * settings.osc1Lvl;
                r += srr * settings.osc1Lvl;
            }

            if (settings.osc2Lvl > 0.0001f)
            {
                osc2.next(settings.osc2Wave, settings.osc2Uni, settings.osc2Det,
                          settings.osc2Spread, effPW2,
                          currentFreq * o2Mult, sr, wavetable, effMorph2, sl, srr);
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

            if (effNoise > 0.0001f)
            {
                float w = rng.nextFloat() * 2.0f - 1.0f;
                if (settings.noiseType == 1)
                    w = pink.next(w);
                w *= effNoise * 0.7f;
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
    float sourceValue(int src) const
    {
        switch (src)
        {
            case srcLfo1:     return settings.lfo1Value;
            case srcLfo2:     return settings.lfo2Value;
            case srcFiltEnv:  return filtEnvLast;
            case srcEnv3:     return env3Last;
            case srcVelocity: return velocity01;
            case srcModWheel: return settings.modWheel;
            case srcPressure: return juce::jmax(pressure, settings.globalPressure);
            case srcRandom:   return random01;
            default:          return 0.0f;
        }
    }

    // Control-rate: evaluate the voice-scoped matrix slots and the drift walk.
    void computeMods()
    {
        float pitchSemis = 0, cutOct = 0, resoAdd = 0, vowAdd = 0;
        float m1 = 0, m2 = 0, p1 = 0, p2 = 0, fmAdd = 0, noiseAdd = 0;

        for (int s = 0; s < numModSlots; ++s)
        {
            const int dst = settings.modDst[s];
            if (dst <= dstNone || dst >= firstGlobalDest)
                continue;
            const float v = sourceValue(settings.modSrc[s]) * settings.modAmt[s];
            switch (dst)
            {
                case dstPitch:  pitchSemis += v * 12.0f; break;
                case dstCutoff: cutOct += v * 4.0f;      break;
                case dstReso:   resoAdd += v;            break;
                case dstVowel:  vowAdd += v;             break;
                case dstMorph1: m1 += v;                 break;
                case dstMorph2: m2 += v;                 break;
                case dstPW1:    p1 += v * 0.45f;         break;
                case dstPW2:    p2 += v * 0.45f;         break;
                case dstFM:     fmAdd += v;              break;
                case dstNoise:  noiseAdd += v;           break;
                default: break;
            }
        }

        matrixPitchFactor = std::exp2(pitchSemis / 12.0f);
        matrixCutoffOct = cutOct;
        effReso   = juce::jlimit(0.0f, 1.0f, settings.resonance + resoAdd);
        effVowel  = juce::jlimit(0.0f, 1.0f, settings.vowel + vowAdd);
        effMorph1 = juce::jlimit(0.0f, 1.0f, settings.osc1Morph + m1);
        effMorph2 = juce::jlimit(0.0f, 1.0f, settings.osc2Morph + m2);
        effPW1    = juce::jlimit(0.05f, 0.95f, settings.osc1PW + p1);
        effPW2    = juce::jlimit(0.05f, 0.95f, settings.osc2PW + p2);
        effFm     = juce::jlimit(0.0f, 1.0f, settings.fmAmt + fmAdd);
        effNoise  = juce::jlimit(0.0f, 1.0f, settings.noiseLvl + noiseAdd);

        // slow random pitch walk: the "old gear" instability
        driftState += 0.05f * (rng.nextFloat() * 2.0f - 1.0f - driftState);
        driftFactor = std::exp2(driftState * settings.driftAmt * 15.0f / 1200.0f);
    }

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
                                 + matrixCutoffOct
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

            const float pos = effVowel * 4.0f;
            const int i0 = juce::jmin(3, (int) pos);
            const float frac = pos - (float) i0;

            // The cutoff knob shifts the whole formant set (1 kHz = neutral).
            const float shift = (settings.cutoff / 1000.0f) * std::exp2(modOctaves);
            const float q = 4.0f + effReso * 8.0f;

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
        const float q = 0.5f + effReso * 7.5f;
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
    galdr::BlepOsc subOsc, fmModOsc;
    galdr::PinkFilter pink;
    juce::Random rng;

    juce::ADSR ampAdsr, filtAdsr, env3Adsr;
    juce::dsp::StateVariableTPTFilter<float> filter1, filter2, filter3;
    juce::AudioBuffer<float> voiceBuffer;

    int note = 60;
    float level = 0.0f, velocity01 = 0.0f, random01 = 0.0f;
    float currentFreq = 0.0f, targetFreq = 440.0f;
    float bendFactor = 1.0f, pressure = 0.0f;
    float prevModPhase = 0.0f;
    float env3Last = 0.0f, filtEnvLast = 0.0f;
    float driftState = 0.0f, driftFactor = 1.0f;
    juce::uint32 startSerial = 0;

    // per-block effective values after the mod matrix
    float matrixPitchFactor = 1.0f, matrixCutoffOct = 0.0f;
    float effReso = 0.2f, effVowel = 0.0f;
    float effMorph1 = 0.0f, effMorph2 = 0.0f;
    float effPW1 = 0.5f, effPW2 = 0.5f;
    float effFm = 0.0f, effNoise = 0.0f;

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

// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas
//
// Offline demo renderer: plays MIDI phrases through the factory presets and
// writes a single WAV plus a segments.txt with name|start|duration lines,
// used to assemble the demo video. Not part of the plugin build.

#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include "../src/PluginProcessor.h"
#include "../src/Presets.h"

namespace
{

struct NoteEvent
{
    int note;
    double start, duration;
    float velocity;
};

struct Segment
{
    juce::String name;
    int presetIndex;
    std::vector<std::pair<const char*, float>> tweaks;
    std::vector<NoteEvent> notes;
    double tailSeconds;
};

void setParam(GaldrAudioProcessor& p, const char* id, float value)
{
    if (auto* rp = p.apvts.getParameter(id))
        rp->setValueNotifyingHost(rp->convertTo0to1(value));
}

std::vector<Segment> buildSegments()
{
    // E minor throughout; note numbers are MIDI.
    return {
        { "Buzzsaw Wall", 1, {},
          { { 40, 0.0, 2.6, 0.9f }, { 47, 0.0, 2.6, 0.85f }, { 52, 0.0, 2.6, 0.8f },
            { 36, 2.8, 2.7, 0.9f }, { 43, 2.8, 2.7, 0.85f }, { 48, 2.8, 2.7, 0.8f } },
          3.0 },

        { "Frostbitten Pad", 2, {},
          { { 52, 0.0, 7.0, 0.7f }, { 55, 0.0, 7.0, 0.7f },
            { 59, 0.0, 7.0, 0.7f }, { 66, 0.0, 7.0, 0.65f } },
          5.0 },

        { "Necro Lead", 3, {},
          { { 64, 0.0, 0.45, 0.9f }, { 67, 0.5, 0.45, 0.85f }, { 69, 1.0, 0.45, 0.9f },
            { 71, 1.5, 0.95, 0.95f }, { 74, 2.5, 0.45, 0.9f }, { 71, 3.0, 0.45, 0.85f },
            { 69, 3.5, 0.45, 0.85f }, { 67, 4.0, 0.95, 0.9f }, { 64, 5.0, 1.4, 0.95f } },
          3.0 },

        { "Cavern Drone", 4, {},
          { { 28, 0.0, 7.0, 0.9f }, { 40, 0.0, 7.0, 0.8f }, { 47, 0.5, 6.5, 0.7f } },
          5.0 },

        { "Winter Sigil", 5, {},
          { { 52, 0.0, 7.0, 0.75f }, { 59, 0.0, 7.0, 0.7f },
            { 62, 0.0, 7.0, 0.7f }, { 66, 0.5, 6.5, 0.65f } },
          5.0 },

        { "Frozen Choir", 6, {},
          { { 52, 0.0, 6.0, 0.8f }, { 55, 0.0, 6.0, 0.75f }, { 59, 0.0, 6.0, 0.75f } },
          4.0 },

        { "Shrieking Gale", 7, {},
          { { 71, 0.0, 5.0, 0.9f } },
          4.0 },

        { "Arpeggiator", 0,
          { { pid::arpMode, 1 }, { pid::arpOct, 2 }, { pid::arpRate, 5 },
            { pid::osc1Uni, 5 }, { pid::osc1Det, 18 }, { pid::osc1Spread, 0.8f },
            { pid::distDrive, 0.4f },
            { pid::delayMix, 0.25f }, { pid::delaySync, 5 },
            { pid::revMix, 0.3f } },
          { { 52, 0.0, 6.0, 0.85f }, { 55, 0.0, 6.0, 0.8f }, { 59, 0.0, 6.0, 0.8f } },
          3.0 },
    };
}

} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outDir = argc > 1 ? juce::File(juce::String(argv[1]))
                                       : juce::File::getCurrentWorkingDirectory();
    outDir.createDirectory();

    const double sampleRate = 44100.0;
    const int blockSize = 512;
    const double gapSeconds = 0.8;

    auto wavFile = outDir.getChildFile("galdr-demo.wav");
    wavFile.deleteFile();
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
        new juce::FileOutputStream(wavFile), sampleRate, 2, 16, {}, 0));
    if (writer == nullptr)
    {
        std::cerr << "cannot open wav writer\n";
        return 1;
    }

    GaldrAudioProcessor processor;
    juce::String segLines;
    double cursor = 0.0;

    for (const auto& seg : buildSegments())
    {
        presets::apply(processor.apvts, seg.presetIndex);
        for (const auto& [id, value] : seg.tweaks)
            setParam(processor, id, value);
        processor.prepareToPlay(sampleRate, blockSize);

        double segLen = 0.0;
        for (const auto& n : seg.notes)
            segLen = juce::jmax(segLen, n.start + n.duration);
        segLen += seg.tailSeconds;
        const int totalSamples = (int) (segLen * sampleRate);

        segLines << "SEG|" << seg.name << "|" << juce::String(cursor, 3) << "|"
                 << juce::String(segLen, 3) << "\n";

        float peak = 0.0f;
        juce::AudioBuffer<float> buffer(2, blockSize);

        for (int pos = 0; pos < totalSamples; pos += blockSize)
        {
            const int n = juce::jmin(blockSize, totalSamples - pos);
            juce::AudioBuffer<float> slice(buffer.getArrayOfWritePointers(), 2, 0, n);
            slice.clear();

            juce::MidiBuffer midi;
            for (const auto& ev : seg.notes)
            {
                const int onAt  = (int) (ev.start * sampleRate);
                const int offAt = (int) ((ev.start + ev.duration) * sampleRate);
                if (onAt >= pos && onAt < pos + n)
                    midi.addEvent(juce::MidiMessage::noteOn(1, ev.note, ev.velocity), onAt - pos);
                if (offAt >= pos && offAt < pos + n)
                    midi.addEvent(juce::MidiMessage::noteOff(1, ev.note), offAt - pos);
            }

            processor.processBlock(slice, midi);
            writer->writeFromAudioSampleBuffer(slice, 0, n);
            peak = juce::jmax(peak, slice.getMagnitude(0, n));
        }

        std::cout << "segment " << seg.name.toRawUTF8() << " len=" << segLen
                  << "s peak=" << peak << "\n";
        cursor += segLen;

        int silence = (int) (gapSeconds * sampleRate);
        while (silence > 0)
        {
            const int n = juce::jmin(blockSize, silence);
            juce::AudioBuffer<float> slice(buffer.getArrayOfWritePointers(), 2, 0, n);
            slice.clear();
            writer->writeFromAudioSampleBuffer(slice, 0, n);
            silence -= n;
        }
        cursor += gapSeconds;
    }

    writer->flush();
    writer.reset();

    outDir.getChildFile("segments.txt").replaceWithText(segLines);
    std::cout << "wrote " << wavFile.getFullPathName().toRawUTF8()
              << " total=" << cursor << "s\n";
    return 0;
}

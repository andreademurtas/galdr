// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

namespace galdr
{

// Microtuning via Scala .scl files. Middle C (note 60) stays anchored at
// 261.6255653 Hz; the scale repeats around it in both directions.
struct Tuning
{
    float freqs[128] {};
    juce::String name;

    Tuning() { reset(); }

    void reset()
    {
        name = "12-TET";
        for (int i = 0; i < 128; ++i)
            freqs[i] = 440.0f * std::exp2(((float) i - 69.0f) / 12.0f);
    }

    bool loadScl(const juce::File& file)
    {
        juce::StringArray fileLines;
        file.readLines(fileLines);

        juce::Array<double> cents;
        int notesPerOctave = -1;
        bool haveDescription = false;

        for (const auto& rawLine : fileLines)
        {
            auto line = rawLine.trim();
            if (line.startsWith("!"))
                continue;
            if (! haveDescription)
            {
                haveDescription = true;
                continue;
            }
            if (notesPerOctave < 0)
            {
                notesPerOctave = line.getIntValue();
                if (notesPerOctave <= 0 || notesPerOctave > 128)
                    return false;
                continue;
            }

            auto token = line.replaceCharacter('\t', ' ').upToFirstOccurrenceOf(" ", false, false);
            double value = 0.0;
            if (token.contains("."))
            {
                value = token.getDoubleValue(); // cents
            }
            else if (token.contains("/"))
            {
                const double num = token.upToFirstOccurrenceOf("/", false, false).getDoubleValue();
                const double den = token.fromFirstOccurrenceOf("/", false, false).getDoubleValue();
                if (num <= 0.0 || den <= 0.0)
                    return false;
                value = 1200.0 * std::log2(num / den);
            }
            else
            {
                const double ratio = token.getDoubleValue(); // bare integer ratio
                if (ratio <= 0.0)
                    return false;
                value = 1200.0 * std::log2(ratio);
            }

            cents.add(value);
            if (cents.size() == notesPerOctave)
                break;
        }

        if (notesPerOctave <= 0 || cents.size() != notesPerOctave)
            return false;

        const double octaveCents = cents.getLast();
        if (octaveCents <= 0.0)
            return false;

        constexpr double refFreq = 261.6255653;
        for (int note = 0; note < 128; ++note)
        {
            const int k = note - 60;
            const int oct = (int) std::floor((double) k / notesPerOctave);
            const int step = k - oct * notesPerOctave;
            const double c = oct * octaveCents + (step == 0 ? 0.0 : cents[step - 1]);
            freqs[note] = (float) (refFreq * std::exp2(c / 1200.0));
        }

        name = file.getFileNameWithoutExtension();
        return true;
    }
};

} // namespace galdr

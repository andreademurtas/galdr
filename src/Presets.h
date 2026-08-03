// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Andrea De Murtas

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Params.h"

namespace presets
{

struct Preset
{
    const char* name;
    const char* category;
    std::vector<std::pair<const char*, double>> values; // plain (denormalised) values
};

// Grouped by category; the editor turns category changes into section headings.
inline const std::vector<Preset>& all()
{
    static const std::vector<Preset> list = {

        // ---------------------------------------------------------------- basics
        { "Init", "Basics", {} },

        // ---------------------------------------------------------- walls & riffs
        { "Buzzsaw Wall", "Walls & Riffs", {
            { pid::osc1Uni, 7 }, { pid::osc1Det, 35 }, { pid::osc1Spread, 0.9f }, { pid::osc1Lvl, 0.9f },
            { pid::osc2Uni, 5 }, { pid::osc2Semi, 7 }, { pid::osc2Det, 20 }, { pid::osc2Spread, 0.8f },
            { pid::osc2Lvl, 0.55f },
            { pid::noiseLvl, 0.06f }, { pid::driftAmt, 0.2f },
            { pid::cutoff, 9000 }, { pid::resonance, 0.15f }, { pid::filterDrive, 0.35f },
            { pid::attack, 0.003f }, { pid::decay, 0.1f }, { pid::sustain, 0.9f }, { pid::release, 0.12f },
            { pid::distType, 1 }, { pid::distDrive, 0.55f }, { pid::distTone, 7000 },
            { pid::tremDepth, 0.85f }, { pid::tremRate, 13 },
            { pid::delayMix, 0.12f }, { pid::delayTime, 0.38f }, { pid::delayFb, 0.35f },
            { pid::revMix, 0.25f }, { pid::revSize, 0.6f }, { pid::revPre, 0.03f },
            { pid::modSrc[0], 5 }, { pid::modDst[0], 2 }, { pid::modAmt[0], 0.3f } } }, // vel -> cutoff

        // ------------------------------------------------------------------ leads
        { "Necro Lead", "Leads", {
            { pid::osc1Wave, 1 }, { pid::osc1Lvl, 0.85f },
            { pid::subWave, 1 }, { pid::subLvl, 0.4f },
            { pid::crushBits, 6 }, { pid::crushRate, 6 },
            { pid::distType, 3 }, { pid::distDrive, 0.5f },
            { pid::filterType, 1 }, { pid::cutoff, 6000 }, { pid::resonance, 0.35f },
            { pid::delayTime, 0.3f }, { pid::delayFb, 0.45f }, { pid::delayMix, 0.3f },
            { pid::revMix, 0.3f },
            { pid::lfo1Depth, 0.25f }, { pid::lfo1Rate, 5.5f },
            { pid::voiceMode, 2 }, { pid::glide, 0.06f }, { pid::driftAmt, 0.2f },
            { pid::modSrc[0], 5 }, { pid::modDst[0], 2 }, { pid::modAmt[0], 0.35f } } },

        { "Howling Sync", "Leads", {
            { pid::voiceMode, 2 }, { pid::glide, 0.08f }, { pid::driftAmt, 0.2f },
            { pid::osc1Oct, 1 }, { pid::osc1Lvl, 0.9f },
            { pid::osc2Lvl, 0 }, { pid::oscSync, 1 }, { pid::fmAmt, 0.2f },
            { pid::cutoff, 9000 }, { pid::filterDrive, 0.3f },
            { pid::distType, 3 }, { pid::distDrive, 0.45f },
            { pid::lfo1Depth, 0.15f }, { pid::lfo1Rate, 5.5f },
            { pid::revMix, 0.35f }, { pid::revSize, 0.7f },
            { pid::env3A, 0.01f }, { pid::env3D, 0.8f }, { pid::env3S, 0.2f }, { pid::env3R, 0.4f },
            { pid::modSrc[0], 4 }, { pid::modDst[0], 9 }, { pid::modAmt[0], 0.5f } } }, // env3 -> FM

        { "Iron Bell", "Leads", {
            { pid::osc1Wave, 4 }, { pid::osc1Lvl, 0.85f },
            { pid::osc2Wave, 4 }, { pid::osc2Oct, 1 }, { pid::osc2Semi, 7 }, { pid::osc2Lvl, 0 },
            { pid::fmAmt, 0.55f }, { pid::driftAmt, 0.1f },
            { pid::attack, 0.002f }, { pid::decay, 2.5f }, { pid::sustain, 0 }, { pid::release, 2.5f },
            { pid::fAttack, 0.002f }, { pid::fDecay, 1.2f }, { pid::fSustain, 0 }, { pid::fRelease, 1 },
            { pid::cutoff, 14000 },
            { pid::delayMix, 0.2f }, { pid::delaySync, 5 }, { pid::delayFb, 0.3f },
            { pid::revMix, 0.45f }, { pid::revSize, 0.85f }, { pid::revShimmer, 0.35f },
            { pid::modSrc[0], 3 }, { pid::modDst[0], 9 }, { pid::modAmt[0], 0.45f } } }, // filt env -> FM

        { "Frost Whistle", "Leads", {
            { pid::osc1Wave, 4 }, { pid::osc1Oct, 1 }, { pid::osc1Lvl, 0.85f },
            { pid::voiceMode, 2 }, { pid::glide, 0.05f },
            { pid::noiseLvl, 0.05f },
            { pid::attack, 0.05f }, { pid::decay, 0.2f }, { pid::sustain, 0.8f }, { pid::release, 0.5f },
            { pid::lfo1Rate, 5 }, { pid::lfo1Depth, 0.3f },
            { pid::delayMix, 0.25f }, { pid::delaySync, 5 },
            { pid::revMix, 0.4f }, { pid::revSize, 0.8f } } },

        // ----------------------------------------------------------------- basses
        { "Tundra Bass", "Basses", {
            { pid::voiceMode, 1 },
            { pid::osc1Lvl, 0.9f }, { pid::subLvl, 0.8f },
            { pid::cutoff, 700 }, { pid::fEnvAmt, 0.55f },
            { pid::fAttack, 0.003f }, { pid::fDecay, 0.35f }, { pid::fSustain, 0.1f }, { pid::fRelease, 0.3f },
            { pid::filterDrive, 0.4f },
            { pid::distDrive, 0.3f },
            { pid::attack, 0.003f }, { pid::decay, 0.3f }, { pid::sustain, 0.8f }, { pid::release, 0.15f },
            { pid::revMix, 0.1f }, { pid::gain, 0.85f } } },

        { "Blackened Bass", "Basses", {
            { pid::voiceMode, 1 },
            { pid::osc1Oct, -1 }, { pid::osc1Lvl, 0.9f }, { pid::fmAmt, 0.5f },
            { pid::crushBits, 10 },
            { pid::distType, 3 }, { pid::distDrive, 0.5f },
            { pid::cutoff, 1800 },
            { pid::attack, 0.004f }, { pid::sustain, 0.9f }, { pid::release, 0.2f },
            { pid::env3A, 0.01f }, { pid::env3D, 0.5f }, { pid::env3S, 0.3f }, { pid::env3R, 0.2f },
            { pid::modSrc[0], 4 }, { pid::modDst[0], 9 }, { pid::modAmt[0], 0.6f },  // env3 -> FM
            { pid::modSrc[1], 6 }, { pid::modDst[1], 2 }, { pid::modAmt[1], 0.5f },  // wheel -> cutoff
            { pid::gain, 0.8f } } },

        // ----------------------------------------------------------- keys & bells
        { "Bone Chimes", "Keys & Bells", {
            { pid::osc1Wave, 5 }, { pid::osc1Morph, 0.85f }, { pid::osc1Lvl, 0.8f },
            { pid::attack, 0.002f }, { pid::decay, 0.6f }, { pid::sustain, 0 }, { pid::release, 0.8f },
            { pid::cutoff, 9000 }, { pid::keytrack, 1 }, { pid::driftAmt, 0.15f },
            { pid::delayMix, 0.3f }, { pid::delaySync, 6 }, { pid::delayFb, 0.45f },
            { pid::revMix, 0.35f },
            { pid::modSrc[0], 8 }, { pid::modDst[0], 5 }, { pid::modAmt[0], 0.25f } } }, // random -> morph

        { "Ritual Bells", "Keys & Bells", {
            { pid::osc1Wave, 1 }, { pid::osc1Oct, 1 }, { pid::osc1Lvl, 0.8f },
            { pid::osc2Lvl, 0 }, { pid::oscSync, 1 }, { pid::fmAmt, 0.3f },
            { pid::attack, 0.002f }, { pid::decay, 1.0f }, { pid::sustain, 0 }, { pid::release, 1.2f },
            { pid::cutoff, 10000 }, { pid::keytrack, 0.6f },
            { pid::revMix, 0.5f }, { pid::revSize, 0.9f }, { pid::revShimmer, 0.5f } } },

        { "Ash Harpsichord", "Keys & Bells", {
            { pid::osc1Wave, 2 }, { pid::osc1PW, 0.12f }, { pid::osc1Lvl, 0.85f },
            { pid::attack, 0.001f }, { pid::decay, 0.35f }, { pid::sustain, 0.15f }, { pid::release, 0.25f },
            { pid::fEnvAmt, 0.4f }, { pid::fDecay, 0.25f }, { pid::fSustain, 0 },
            { pid::cutoff, 4000 }, { pid::keytrack, 0.7f },
            { pid::crushBits, 12 }, { pid::crushRate, 2 },
            { pid::chorusMix, 0.2f },
            { pid::delayMix, 0.15f }, { pid::delaySync, 5 },
            { pid::revMix, 0.3f } } },

        // ---------------------------------------------------------- pads & choirs
        { "Frostbitten Pad", "Pads & Choirs", {
            { pid::osc1Uni, 5 }, { pid::osc1Det, 18 }, { pid::osc1Spread, 1 }, { pid::osc1Lvl, 0.7f },
            { pid::osc2Wave, 2 }, { pid::osc2PW, 0.3f }, { pid::osc2Semi, 7 }, { pid::osc2Uni, 3 },
            { pid::osc2Det, 14 }, { pid::osc2Lvl, 0.5f },
            { pid::noiseType, 1 }, { pid::noiseLvl, 0.12f }, { pid::driftAmt, 0.25f },
            { pid::cutoff, 3500 }, { pid::resonance, 0.25f }, { pid::fEnvAmt, 0.3f },
            { pid::fAttack, 1.2f }, { pid::fDecay, 2 }, { pid::fSustain, 0.6f }, { pid::fRelease, 2 },
            { pid::attack, 1.5f }, { pid::decay, 1 }, { pid::sustain, 0.8f }, { pid::release, 2.5f },
            { pid::chorusMix, 0.5f }, { pid::chorusRate, 0.8f }, { pid::chorusDepth, 0.35f },
            { pid::revSize, 0.95f }, { pid::revMix, 0.5f }, { pid::revDamp, 0.4f },
            { pid::revPre, 0.05f }, { pid::revShimmer, 0.3f },
            { pid::lfo2Rate, 0.15f }, { pid::lfo2Depth, 0.2f },
            { pid::distDrive, 0.2f }, { pid::distMix, 0.6f },
            { pid::modSrc[0], 6 }, { pid::modDst[0], 2 }, { pid::modAmt[0], 0.4f },  // wheel -> cutoff
            { pid::gain, 0.7f } } },

        { "Winter Sigil", "Pads & Choirs", {
            { pid::osc1Wave, 5 }, { pid::osc1Morph, 0.65f }, { pid::osc1Uni, 5 },
            { pid::osc1Det, 15 }, { pid::osc1Spread, 1 },
            { pid::osc2Wave, 5 }, { pid::osc2Morph, 0.2f }, { pid::osc2Semi, 7 },
            { pid::osc2Lvl, 0.45f }, { pid::driftAmt, 0.2f },
            { pid::cutoff, 5000 },
            { pid::attack, 1.0f }, { pid::sustain, 0.8f }, { pid::release, 2.5f },
            { pid::chorusMix, 0.3f },
            { pid::revMix, 0.45f }, { pid::revSize, 0.8f }, { pid::revPre, 0.06f },
            { pid::revShimmer, 0.4f },
            { pid::bzLvl, 0.25f }, { pid::bzDensity, 25 }, { pid::bzSize, 0.12f },
            { pid::bzPitch, 1200 }, { pid::bzSpread, 0.7f },
            { pid::env3A, 1.5f }, { pid::env3D, 2 }, { pid::env3S, 0.6f }, { pid::env3R, 2 },
            { pid::modSrc[0], 4 }, { pid::modDst[0], 5 }, { pid::modAmt[0], 0.3f },  // env3 -> morph
            { pid::gain, 0.75f } } },

        { "Frozen Choir", "Pads & Choirs", {
            { pid::osc1Wave, 5 }, { pid::osc1Morph, 0.3f }, { pid::osc1Uni, 3 },
            { pid::osc1Det, 10 }, { pid::osc1Spread, 0.8f },
            { pid::osc2Wave, 5 }, { pid::osc2Morph, 0.8f }, { pid::osc2Semi, 7 },
            { pid::osc2Lvl, 0.5f }, { pid::driftAmt, 0.15f },
            { pid::filterType, 4 }, { pid::vowel, 0.4f }, { pid::cutoff, 1000 },
            { pid::resonance, 0.4f },
            { pid::attack, 0.8f }, { pid::sustain, 1 }, { pid::release, 2 },
            { pid::lfo2Rate, 0.1f }, { pid::lfo2Depth, 0.15f },
            { pid::revMix, 0.5f }, { pid::revSize, 0.85f }, { pid::revPre, 0.06f },
            { pid::revShimmer, 0.5f },
            { pid::modSrc[0], 6 }, { pid::modDst[0], 4 }, { pid::modAmt[0], 0.5f },  // wheel -> vowel
            { pid::gain, 0.7f } } },

        { "Aurora Veil", "Pads & Choirs", {
            { pid::osc1Wave, 5 }, { pid::osc1Morph, 0.3f }, { pid::osc1Uni, 5 },
            { pid::osc1Det, 14 }, { pid::osc1Spread, 1 },
            { pid::osc2Wave, 5 }, { pid::osc2Morph, 0.7f }, { pid::osc2Oct, 1 }, { pid::osc2Lvl, 0.4f },
            { pid::driftAmt, 0.2f },
            { pid::attack, 2 }, { pid::sustain, 0.9f }, { pid::release, 3.5f },
            { pid::lfo2Shape, 0 }, { pid::lfo2Rate, 0.07f },
            { pid::chorusMix, 0.4f },
            { pid::revMix, 0.55f }, { pid::revSize, 0.95f }, { pid::revShimmer, 0.45f },
            { pid::modSrc[0], 2 }, { pid::modDst[0], 5 }, { pid::modAmt[0], 0.35f }, // lfo2 -> morph
            { pid::modSrc[1], 1 }, { pid::modDst[1], 7 }, { pid::modAmt[1], 0.2f },  // lfo1 -> pw
            { pid::gain, 0.7f } } },

        { "Funeral Organ", "Pads & Choirs", {
            { pid::osc1Wave, 1 }, { pid::osc1Lvl, 0.7f },
            { pid::osc2Wave, 2 }, { pid::osc2PW, 0.3f }, { pid::osc2Oct, 1 }, { pid::osc2Lvl, 0.45f },
            { pid::subWave, 1 }, { pid::subLvl, 0.5f },
            { pid::attack, 0.05f }, { pid::sustain, 1 }, { pid::release, 0.4f },
            { pid::cutoff, 5000 }, { pid::keytrack, 0.3f },
            { pid::chorusMix, 0.25f }, { pid::chorusRate, 0.7f },
            { pid::revMix, 0.5f }, { pid::revSize, 0.9f }, { pid::revPre, 0.05f },
            { pid::gain, 0.75f } } },

        { "Spectral Mass", "Pads & Choirs", {
            { pid::osc1Uni, 5 }, { pid::osc1Det, 12 }, { pid::osc1Spread, 0.9f },
            { pid::noiseType, 1 }, { pid::noiseLvl, 0.1f },
            { pid::filterType, 4 }, { pid::vowel, 0.55f }, { pid::cutoff, 1000 },
            { pid::attack, 1.2f }, { pid::sustain, 1 }, { pid::release, 2.2f },
            { pid::lfo2Rate, 0.1f },
            { pid::revMix, 0.55f }, { pid::revShimmer, 0.6f }, { pid::revSize, 0.9f },
            { pid::modSrc[0], 6 }, { pid::modDst[0], 4 }, { pid::modAmt[0], 0.6f },  // wheel -> vowel
            { pid::modSrc[1], 2 }, { pid::modDst[1], 4 }, { pid::modAmt[1], 0.15f }, // lfo2 -> vowel
            { pid::gain, 0.7f } } },

        // ----------------------------------------------------------------- drones
        { "Cavern Drone", "Drones", {
            { pid::osc1Oct, -1 }, { pid::osc1Uni, 7 }, { pid::osc1Det, 25 }, { pid::osc1Spread, 1 },
            { pid::osc1Lvl, 0.8f },
            { pid::osc2Oct, -1 }, { pid::osc2Semi, 7 }, { pid::osc2Uni, 5 }, { pid::osc2Det, 18 },
            { pid::osc2Lvl, 0.6f },
            { pid::subLvl, 0.8f }, { pid::driftAmt, 0.35f },
            { pid::cutoff, 900 }, { pid::resonance, 0.3f },
            { pid::attack, 2 }, { pid::decay, 1 }, { pid::sustain, 1 }, { pid::release, 3 },
            { pid::distType, 2 }, { pid::distDrive, 0.35f }, { pid::distMix, 0.8f },
            { pid::revSize, 1 }, { pid::revMix, 0.6f }, { pid::revDamp, 0.2f },
            { pid::revPre, 0.08f }, { pid::revShimmer, 0.25f },
            { pid::lfo2Rate, 0.08f }, { pid::lfo2Depth, 0.35f },
            { pid::bzLvl, 0.12f }, { pid::bzDensity, 10 }, { pid::bzPitch, 500 },
            { pid::gain, 0.75f } } },

        { "Permafrost", "Drones", {
            { pid::osc1Wave, 5 }, { pid::osc1Morph, 1 }, { pid::osc1Oct, -1 },
            { pid::osc1Uni, 7 }, { pid::osc1Det, 20 }, { pid::osc1Spread, 1 },
            { pid::subLvl, 0.6f }, { pid::driftAmt, 0.5f },
            { pid::cutoff, 1500 },
            { pid::attack, 3 }, { pid::sustain, 1 }, { pid::release, 4 },
            { pid::distType, 2 }, { pid::distDrive, 0.3f }, { pid::distMix, 0.7f },
            { pid::revMix, 0.6f }, { pid::revSize, 1 }, { pid::revDamp, 0.15f }, { pid::revPre, 0.08f },
            { pid::bzGate, 1 }, { pid::bzLvl, 0.18f }, { pid::bzDensity, 12 }, { pid::bzPitch, 700 },
            { pid::gain, 0.7f } } },

        { "Iron Throat", "Drones", {
            { pid::voiceMode, 1 },
            { pid::osc1Oct, -2 }, { pid::osc1Uni, 3 }, { pid::osc1Det, 8 }, { pid::osc1Lvl, 0.85f },
            { pid::subLvl, 0.9f }, { pid::subOct, 1 },
            { pid::rmMix, 0.3f }, { pid::rmFreq, 55 },
            { pid::distType, 3 }, { pid::distDrive, 0.4f },
            { pid::cutoff, 900 },
            { pid::attack, 1.5f }, { pid::sustain, 1 }, { pid::release, 2 },
            { pid::revMix, 0.35f },
            { pid::gain, 0.8f } } },

        // --------------------------------------------------------------- textures
        { "Shrieking Gale", "Textures", {
            { pid::noiseLvl, 0.9f },
            { pid::osc1Lvl, 0.15f },
            { pid::filterType, 2 }, { pid::cutoff, 2500 }, { pid::resonance, 0.7f },
            { pid::lfo2Shape, 4 }, { pid::lfo2Rate, 8 }, { pid::lfo2Depth, 0.5f },
            { pid::distType, 1 }, { pid::distDrive, 0.45f },
            { pid::rmMix, 0.15f }, { pid::rmFreq, 1800 },
            { pid::revMix, 0.45f }, { pid::revSize, 0.85f }, { pid::revShimmer, 0.3f },
            { pid::attack, 0.4f }, { pid::sustain, 1 }, { pid::release, 1.2f },
            { pid::tremDepth, 0.3f }, { pid::tremRate, 9 } } },

        { "Whiteout", "Textures", {
            { pid::osc1Lvl, 0 },
            { pid::noiseType, 1 }, { pid::noiseLvl, 0.3f },
            { pid::bzLvl, 0.8f }, { pid::bzDensity, 80 }, { pid::bzSize, 0.25f },
            { pid::bzPitch, 2500 }, { pid::bzSpread, 0.9f },
            { pid::filterType, 2 }, { pid::cutoff, 900 },
            { pid::lfo2Rate, 0.06f }, { pid::lfo2Depth, 0.3f },
            { pid::attack, 2 }, { pid::sustain, 1 }, { pid::release, 3 },
            { pid::revMix, 0.5f }, { pid::revSize, 0.95f } } },

        { "Wolves in the Walls", "Textures", {
            { pid::osc1Uni, 3 }, { pid::osc1Det, 10 }, { pid::osc1Lvl, 0.6f },
            { pid::noiseType, 1 }, { pid::noiseLvl, 0.2f },
            { pid::filterType, 4 }, { pid::vowel, 0.3f }, { pid::cutoff, 1000 },
            { pid::resonance, 0.5f },
            { pid::lfo2Shape, 4 }, { pid::lfo2Rate, 2.5f },
            { pid::rmMix, 0.15f }, { pid::rmFreq, 90 },
            { pid::attack, 0.4f }, { pid::sustain, 1 }, { pid::release, 1.5f },
            { pid::revMix, 0.45f }, { pid::revPre, 0.04f },
            { pid::modSrc[0], 2 }, { pid::modDst[0], 4 }, { pid::modAmt[0], 0.5f },  // s&h lfo2 -> vowel
            { pid::modSrc[1], 8 }, { pid::modDst[1], 1 }, { pid::modAmt[1], 0.03f } } }, // random -> pitch

        { "Frost Static", "Textures", {
            { pid::osc1Wave, 1 }, { pid::osc1Oct, -1 }, { pid::osc1Lvl, 0.3f },
            { pid::noiseLvl, 0.5f },
            { pid::crushBits, 5 }, { pid::crushRate, 14 },
            { pid::rmMix, 0.4f }, { pid::rmFreq, 1200 },
            { pid::filterType, 2 }, { pid::cutoff, 500 },
            { pid::lfo2Shape, 4 }, { pid::lfo2Rate, 6 },
            { pid::attack, 0.01f }, { pid::sustain, 1 }, { pid::release, 0.8f },
            { pid::delayMix, 0.3f }, { pid::delayFb, 0.6f }, { pid::delaySync, 7 },
            { pid::revMix, 0.25f },
            { pid::modSrc[0], 2 }, { pid::modDst[0], 11 }, { pid::modAmt[0], 0.4f } } }, // lfo2 -> rm freq

        // -------------------------------------------------------------- arpeggios
        { "Court of Icicles", "Arpeggios", {
            { pid::arpMode, 1 }, { pid::arpOct, 2 }, { pid::arpRate, 5 }, { pid::arpGate, 0.55f },
            { pid::osc1Wave, 5 }, { pid::osc1Morph, 0.6f }, { pid::osc1Lvl, 0.85f },
            { pid::attack, 0.002f }, { pid::decay, 0.3f }, { pid::sustain, 0.2f }, { pid::release, 0.3f },
            { pid::cutoff, 8000 }, { pid::keytrack, 0.8f }, { pid::driftAmt, 0.1f },
            { pid::delayMix, 0.3f }, { pid::delaySync, 6 }, { pid::delayFb, 0.45f },
            { pid::revMix, 0.35f }, { pid::revShimmer, 0.3f } } },

        { "Blast Furnace", "Arpeggios", {
            { pid::arpMode, 4 }, { pid::arpOct, 3 }, { pid::arpRate, 7 }, { pid::arpGate, 0.9f },
            { pid::osc1Uni, 5 }, { pid::osc1Det, 25 }, { pid::osc1Spread, 0.8f }, { pid::osc1Lvl, 0.9f },
            { pid::distType, 1 }, { pid::distDrive, 0.6f },
            { pid::cutoff, 6000 }, { pid::fEnvAmt, 0.3f },
            { pid::fDecay, 0.12f }, { pid::fSustain, 0.2f },
            { pid::attack, 0.001f }, { pid::decay, 0.15f }, { pid::sustain, 0.6f }, { pid::release, 0.1f },
            { pid::revMix, 0.2f },
            { pid::gain, 0.78f } } },

        { "Doom Procession", "Arpeggios", {
            { pid::arpMode, 5 }, { pid::arpRate, 2 }, { pid::arpGate, 0.95f },
            { pid::osc1Oct, -1 }, { pid::osc1Uni, 5 }, { pid::osc1Det, 15 }, { pid::osc1Lvl, 0.9f },
            { pid::subLvl, 0.5f },
            { pid::distDrive, 0.35f },
            { pid::cutoff, 2500 },
            { pid::attack, 0.01f }, { pid::sustain, 0.9f }, { pid::release, 0.4f },
            { pid::tremDepth, 0.4f }, { pid::tremSync, 7 },
            { pid::revMix, 0.4f }, { pid::revSize, 0.8f },
            { pid::gain, 0.78f } } },
    };
    return list;
}

inline int indexOf(const juce::String& name)
{
    const auto& list = all();
    for (int i = 0; i < (int) list.size(); ++i)
        if (name == list[(size_t) i].name)
            return i;
    return -1;
}

inline void apply(juce::AudioProcessorValueTreeState& apvts, int index)
{
    if (index < 0 || index >= (int) all().size())
        return;

    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(p))
            rp->setValueNotifyingHost(rp->getDefaultValue());

    for (const auto& [id, value] : all()[(size_t) index].values)
        if (auto* rp = apvts.getParameter(id))
            rp->setValueNotifyingHost(rp->convertTo0to1((float) value));
}

} // namespace presets

cmake_minimum_required(VERSION 3.22)

project(Galdr VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

# GUI fonts, embedded into the binary. Both are licensed under the SIL OFL
# (see assets/fonts/OFL-*.txt, which must ship alongside any distribution).
juce_add_binary_data(GaldrAssets
    SOURCES
        assets/fonts/UnifrakturMaguntia.ttf
        assets/fonts/IMFellEnglish.ttf)

# PLUGIN_MANUFACTURER_CODE and PLUGIN_CODE must never change after the first
# public release, or saved DAW sessions will lose the plugin.
juce_add_plugin(Galdr
    COMPANY_NAME "Andrea De Murtas"
    BUNDLE_ID com.andreademurtas.galdr
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Admu
    PLUGIN_CODE Gldr
    VST3_CATEGORIES Instrument Synth
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Galdr")

target_sources(Galdr PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp)

target_compile_definitions(Galdr PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    # Disabling the splash screen is permitted when JUCE is used under the (A)GPL.
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Galdr
    PRIVATE
        GaldrAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# CLAP export (MIT-licensed wrapper around the JUCE plugin target)
add_subdirectory(modules/clap-juce-extensions EXCLUDE_FROM_ALL)
clap_juce_extensions_plugin(TARGET Galdr
    CLAP_ID "com.andreademurtas.galdr"
    CLAP_FEATURES instrument synthesizer)

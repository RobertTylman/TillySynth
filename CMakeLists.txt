cmake_minimum_required(VERSION 3.22)
project(TillySynth VERSION 1.0.0 LANGUAGES C CXX OBJC OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON
)

juce_add_plugin(TillySynth
    COMPANY_NAME "Robbie Tylman"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE RbTy
    PLUGIN_CODE TlSy
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "TillySynth"
    BUNDLE_ID "com.robbietylman.tillysynth"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

set(TILLYSYNTH_SHARED_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/PresetManager.cpp
    Source/PresetVariantGenerator.cpp
    Source/DSP/Oscillator.cpp
    Source/DSP/NoiseOscillator.cpp
    Source/DSP/Envelope.cpp
    Source/DSP/SubtractiveFilter.cpp
    Source/DSP/LFO.cpp
    Source/DSP/ChorusEngine.cpp
    Source/DSP/AnalogueDriftEngine.cpp
    Source/DSP/DriftSensorReader.mm
    Source/DSP/SynthVoice.cpp
    Source/DSP/VoiceManager.cpp
    Source/UI/TillySynthLookAndFeel.cpp
)

add_library(TillySynthCore STATIC
    ${TILLYSYNTH_SHARED_SOURCES}
)

target_include_directories(TillySynthCore PRIVATE Source)

target_compile_definitions(TillySynthCore PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(TillySynthCore PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_core
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_data_structures
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_link_libraries(TillySynth PRIVATE TillySynthCore)

juce_add_gui_app(TillyPresetReview
    PRODUCT_NAME "TillySynth Preset Review"
)

target_sources(TillyPresetReview PRIVATE
    Source/PresetReviewApp.cpp
)

target_include_directories(TillyPresetReview PRIVATE Source)
target_link_libraries(TillyPresetReview PRIVATE
    TillySynthCore
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

if(APPLE)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREMOTION_FRAMEWORK CoreMotion)

    if(IOKIT_FRAMEWORK)
        target_link_libraries(TillySynthCore PRIVATE ${IOKIT_FRAMEWORK})
    endif()
    if(COREMOTION_FRAMEWORK)
        target_link_libraries(TillySynthCore PRIVATE ${COREMOTION_FRAMEWORK})
    endif()
endif()

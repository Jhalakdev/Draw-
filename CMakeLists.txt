cmake_minimum_required(VERSION 3.22)
project(PitchFollowEQ VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PitchFollowEQ
    BUNDLE_ID               "com.Curvex.DrawEQ"
    COMPANY_NAME           "Curvex"
    PLUGIN_MANUFACTURER_CODE Draw
    PLUGIN_CODE            DrEQ
    FORMATS                AU VST3 Standalone
    PRODUCT_NAME           "Draw EQ"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_AU_EXPORT_PREFIX PitchFollowEQAU
    VST3_CATEGORIES        Audio Effects
)

target_sources(PitchFollowEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/Equalizer.cpp
        Source/DSP/PitchDetector.cpp
        Source/DSP/PitchFollowEngine.cpp
        Source/DSP/AnalogCharacter.cpp
        Source/UI/EQGraphComponent.cpp
        Source/UI/SpectrumAnalyzer.cpp
)

juce_generate_juce_header(PitchFollowEQ)

target_compile_definitions(PitchFollowEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(PitchFollowEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
)

set_target_properties(PitchFollowEQ PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.curvex.DrawEQ"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
    XCODE_ATTRIBUTE_BUILD_LIBRARY_FOR_DISTRIBUTION "YES"
)

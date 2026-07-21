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

# ============================================================
# SHARED DSP files (used by multiple targets)
# ============================================================
set(SHARED_DSP_SOURCES
    Source/DSP/AnalogCharacter.cpp
)

set(SHARED_UI_SOURCES
    Source/UI/EQGraphComponent.cpp
)

# ============================================================
# 1. Draw EQ Pro (existing)
# ============================================================
juce_add_plugin(DrawEQ
    BUNDLE_ID               "com.Curvex.DrawEQPro"
    COMPANY_NAME           "Curvex"
    PLUGIN_MANUFACTURER_CODE Draw
    PLUGIN_CODE            DrEP
    FORMATS                AU VST3 Standalone
    PRODUCT_NAME           "Draw EQ Pro"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_AU_EXPORT_PREFIX PitchFollowEQAU
    VST3_CATEGORIES        Audio Effects
)

target_sources(DrawEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/Equalizer.cpp
        Source/DSP/PitchDetector.cpp
        Source/DSP/PitchFollowEngine.cpp
        Source/DSP/FFTAnalyzer.cpp
        ${SHARED_DSP_SOURCES}
        ${SHARED_UI_SOURCES}
)

target_compile_definitions(DrawEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(DrawEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
)

set_target_properties(DrawEQ PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.curvex.DrawEQPro"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
    XCODE_ATTRIBUTE_BUILD_LIBRARY_FOR_DISTRIBUTION "YES"
)

juce_generate_juce_header(DrawEQ)

# ============================================================
# 2. Draw EQ Simple (no saturation)
# ============================================================
juce_add_plugin(DrawEQSimple
    BUNDLE_ID               "com.Curvex.DrawEQSimple"
    COMPANY_NAME           "Curvex"
    PLUGIN_MANUFACTURER_CODE Draw
    PLUGIN_CODE            DrES
    FORMATS                AU VST3 Standalone
    PRODUCT_NAME           "Draw EQ"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_AU_EXPORT_PREFIX DrawEQSimpleAU
    VST3_CATEGORIES        Audio Effects
)

target_sources(DrawEQSimple
    PRIVATE
        DrawEQ/Source/PluginProcessor.cpp
        DrawEQ/Source/PluginEditor.cpp
        Source/DSP/Equalizer.cpp
        Source/DSP/PitchDetector.cpp
        Source/DSP/PitchFollowEngine.cpp
        Source/DSP/FFTAnalyzer.cpp
        ${SHARED_DSP_SOURCES}
        ${SHARED_UI_SOURCES}
)

target_compile_definitions(DrawEQSimple
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(DrawEQSimple
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
)

set_target_properties(DrawEQSimple PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.curvex.DrawEQSimple"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
)

juce_generate_juce_header(DrawEQSimple)

# ============================================================
# 3. Crystal Gain
# ============================================================
juce_add_plugin(CrystalGain
    BUNDLE_ID               "com.Curvex.CrystalGain"
    COMPANY_NAME           "Curvex"
    PLUGIN_MANUFACTURER_CODE Draw
    PLUGIN_CODE            CrGn
    FORMATS                AU VST3 Standalone
    PRODUCT_NAME           "Crystal Gain"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_AU_EXPORT_PREFIX CrystalGainAU
    VST3_CATEGORIES        Audio Effects
)

target_sources(CrystalGain
    PRIVATE
        CrystalGain/Source/PluginProcessor.cpp
        CrystalGain/Source/PluginEditor.cpp
)

target_compile_definitions(CrystalGain
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(CrystalGain
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
)

set_target_properties(CrystalGain PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.curvex.CrystalGain"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
)

juce_generate_juce_header(CrystalGain)

# ============================================================
# 4. Crystal Gain Pro
# ============================================================
juce_add_plugin(CrystalGainPro
    BUNDLE_ID               "com.Curvex.CrystalGainPro"
    COMPANY_NAME           "Curvex"
    PLUGIN_MANUFACTURER_CODE Draw
    PLUGIN_CODE            CrGP
    FORMATS                AU VST3 Standalone
    PRODUCT_NAME           "Crystal Gain Pro"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_AU_EXPORT_PREFIX CrystalGainProAU
    VST3_CATEGORIES        Audio Effects
)

target_sources(CrystalGainPro
    PRIVATE
        CrystalGainPro/Source/PluginProcessor.cpp
        CrystalGainPro/Source/PluginEditor.cpp
        ${SHARED_DSP_SOURCES}
)

target_compile_definitions(CrystalGainPro
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(CrystalGainPro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
)

set_target_properties(CrystalGainPro PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.curvex.CrystalGainPro"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
)

juce_generate_juce_header(CrystalGainPro)

# ============================================================
# 5. Cat's Hair
# ============================================================
juce_add_plugin(CatsHair
    BUNDLE_ID               "com.Curvex.CatsHair"
    COMPANY_NAME           "Curvex"
    PLUGIN_MANUFACTURER_CODE Draw
    PLUGIN_CODE            CtHr
    FORMATS                AU VST3 Standalone
    PRODUCT_NAME           "Cat's Hair"
    IS_SYNTH               FALSE
    NEEDS_MIDI_INPUT       FALSE
    NEEDS_MIDI_OUTPUT      FALSE
    IS_MIDI_EFFECT         FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_AU_EXPORT_PREFIX CatsHairAU
    VST3_CATEGORIES        Audio Effects
)

target_sources(CatsHair
    PRIVATE
        CatsHair/Source/PluginProcessor.cpp
        CatsHair/Source/PluginEditor.cpp
)

target_compile_definitions(CatsHair
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(CatsHair
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
)

set_target_properties(CatsHair PROPERTIES
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.curvex.CatsHair"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
)

juce_generate_juce_header(CatsHair)

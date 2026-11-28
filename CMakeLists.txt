cmake_minimum_required(VERSION 3.22)

# Apple Silicon only (set before project()).
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

project(ProTunezFXToys VERSION 0.3.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PTFX_BUILD_PLUGIN "Build the plugin (needs JUCE)" ON)
option(PTFX_BUILD_TESTS "Build the sound-engine tests" ON)



if(PTFX_BUILD_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)

    if(APPLE)
        set(PTFX_FORMATS AU VST3 Standalone)
    else()
        set(PTFX_FORMATS VST3 Standalone) # for test-compiling only
    endif()

    juce_add_plugin(ProTunezFXToys
        PRODUCT_NAME "ProTunez FX Toys"
        COMPANY_NAME "The Revivalists Media"
        BUNDLE_ID "com.revivalistsmedia.protunezfxtoys"
        PLUGIN_MANUFACTURER_CODE Rvmd
        PLUGIN_CODE PzFx
        FORMATS ${PTFX_FORMATS}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        AU_MAIN_TYPE kAudioUnitType_Effect
        VST3_CATEGORIES Fx
        COPY_PLUGIN_AFTER_BUILD FALSE)

    target_sources(ProTunezFXToys PRIVATE
        Source/PluginProcessor.cpp
        Source/PresetManager.cpp
        Source/FactoryPresets.cpp
        Source/UI/Theme.cpp
        Source/UI/Widgets.cpp
        Source/UI/EffectView.cpp
        Source/UI/Header.cpp
        Source/UI/Views.cpp
        Source/UI/HeatView.cpp
        Source/UI/EchoView.cpp
        Source/UI/PlateView.cpp
        Source/PluginEditor.cpp)

    target_compile_definitions(ProTunezFXToys PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

    target_link_libraries(ProTunezFXToys
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endif()

if(PTFX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

if(PTFX_BUILD_PLUGIN AND NOT APPLE)
    # Linux-only helper: renders each screen to PNG for checking.
    juce_add_console_app(Screenshots PRODUCT_NAME "Screenshots")
    get_target_property(PTFX_SOURCES ProTunezFXToys SOURCES)
    target_sources(Screenshots PRIVATE tools/Screenshots.cpp ${PTFX_SOURCES})
    target_compile_definitions(Screenshots PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(Screenshots PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()

if(PTFX_BUILD_PLUGIN AND PTFX_BUILD_TESTS)
    # JUCE-based tests: presets and the whole processor.
    juce_add_console_app(PluginTests PRODUCT_NAME "PluginTests")
    get_target_property(PTFX_TEST_SOURCES ProTunezFXToys SOURCES)
    target_sources(PluginTests PRIVATE Tests/PluginTests.cpp ${PTFX_TEST_SOURCES})
    target_compile_definitions(PluginTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(PluginTests PRIVATE juce::juce_audio_utils juce::juce_dsp Catch2::Catch2WithMain juce::juce_recommended_config_flags)
    add_test(NAME PluginTests COMMAND PluginTests)
endif()

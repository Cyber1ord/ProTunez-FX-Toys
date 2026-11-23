include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(DSPTests DSPTests.cpp)
target_compile_features(DSPTests PRIVATE cxx_std_17)
target_link_libraries(DSPTests PRIVATE Catch2::Catch2WithMain)

include(CTest)
add_test(NAME DSPTests COMMAND DSPTests)

if(PTFX_BUILD_PLUGIN)
    juce_add_console_app(PluginTests PRODUCT_NAME "PluginTests")
    target_sources(PluginTests PRIVATE PluginTests.cpp ../Source/PresetManager.cpp ../Source/FactoryPresets.cpp)
    target_compile_definitions(PluginTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1)
    target_link_libraries(PluginTests PRIVATE juce::juce_audio_processors Catch2::Catch2WithMain juce::juce_recommended_config_flags)
    add_test(NAME PluginTests COMMAND PluginTests)
endif()

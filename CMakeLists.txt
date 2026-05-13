cmake_minimum_required(VERSION 3.22)
project(RudeHype VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

juce_add_plugin(RudeHype
    COMPANY_NAME "NaughtyDream"
    PLUGIN_MANUFACTURER_CODE Ndre
    PLUGIN_CODE Rhyp
    FORMATS VST3
    PRODUCT_NAME "RUDE HYPE"
)

juce_generate_juce_header(RudeHype)

target_sources(RudeHype PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(RudeHype PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RudeHype PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_graphics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

cmake_minimum_required(VERSION 3.22)
project(BuckRage VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

juce_add_plugin(BuckRage
    COMPANY_NAME "NaughtyDream"
    PLUGIN_MANUFACTURER_CODE Ndre
    PLUGIN_CODE Bkrg
    FORMATS VST3
    PRODUCT_NAME "BUCK RAGE"
)

juce_add_binary_data(BuckRageAssets
    SOURCES
        Resources/background_buck_rage.png
        Resources/knob_base.png
)

target_sources(BuckRage PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/KnobLookAndFeel.cpp
)

target_compile_definitions(BuckRage PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BuckRage PRIVATE
    BuckRageAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_graphics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

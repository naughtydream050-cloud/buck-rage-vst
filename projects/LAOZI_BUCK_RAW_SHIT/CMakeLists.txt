cmake_minimum_required(VERSION 3.22)
project(LaoziBuckRawShit VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../JUCE/CMakeLists.txt")
    add_subdirectory(../../JUCE JUCE)
else()
    message(FATAL_ERROR "JUCE source directory was not found.")
endif()

juce_add_plugin(LaoziBuckRawShit
    COMPANY_NAME "NaughtyDream"
    PLUGIN_MANUFACTURER_CODE Ndre
    PLUGIN_CODE Lbrs
    FORMATS VST3
    PRODUCT_NAME "老子-BUCK RAW SHIT-"
    BUNDLE_ID "com.naughtydream.laozibuckrawshit"
)

juce_generate_juce_header(LaoziBuckRawShit)

juce_add_binary_data(LaoziBuckRawShitAssets
    SOURCES
        Resources/faceplate_laozi_buck_raw_shit.png
        Resources/knob_pressure.png
        Resources/knob_kick.png
        Resources/knob_aura.png
        Resources/knob_glue.png
        Resources/knob_output.png
)

target_sources(LaoziBuckRawShit PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ImageKnob.cpp
    Source/StereoMeter.cpp
)

target_compile_definitions(LaoziBuckRawShit PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LaoziBuckRawShit PRIVATE
    LaoziBuckRawShitAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_graphics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

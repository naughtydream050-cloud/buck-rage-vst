cmake_minimum_required(VERSION 3.22)
project(VintageRawness VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../BUCK_RAGE/JUCE/CMakeLists.txt")
    add_subdirectory(../BUCK_RAGE/JUCE JUCE)
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../JUCE/CMakeLists.txt")
    add_subdirectory(../../JUCE JUCE)
else()
    message(FATAL_ERROR "JUCE source directory was not found.")
endif()

set(VINTAGE_RAWNESS_FORMATS VST3)
if(APPLE)
    list(APPEND VINTAGE_RAWNESS_FORMATS AU)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

juce_add_plugin(VintageRawness
    COMPANY_NAME "NaughtyDream"
    PLUGIN_MANUFACTURER_CODE Ndre
    PLUGIN_CODE Vraw
    FORMATS ${VINTAGE_RAWNESS_FORMATS}
    PRODUCT_NAME "VINTAGE RAWNESS"
    BUNDLE_ID "com.naughtydream.vintagerawness"
)

juce_generate_juce_header(VintageRawness)

juce_add_binary_data(VintageRawnessAssets
    SOURCES
        Resources/faceplate_vintage_rawness.png
        Resources/knob_dirt.png
        Resources/knob_crush.png
        Resources/knob_wobble.png
)

target_sources(VintageRawness PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(VintageRawness PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VintageRawness PRIVATE
    VintageRawnessAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_graphics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

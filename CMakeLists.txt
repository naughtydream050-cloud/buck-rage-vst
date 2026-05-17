cmake_minimum_required(VERSION 3.22)
project(RudeHype VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

add_subdirectory(JUCE)

set(RUDE_HYPE_FORMATS VST3)
if(APPLE)
    list(APPEND RUDE_HYPE_FORMATS AU)
endif()

juce_add_plugin(RudeHype
    COMPANY_NAME "NaughtyDream"
    BUNDLE_ID "com.naughtydream.rudehype"
    PLUGIN_MANUFACTURER_CODE Ndre
    PLUGIN_CODE Rhyp
    FORMATS ${RUDE_HYPE_FORMATS}
    PRODUCT_NAME "RUDE HYPE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

juce_generate_juce_header(RudeHype)

target_sources(RudeHype PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

set(RUDE_HYPE_RESOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/faceplate_rude_hype.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/knob_shout.png
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources/knob_burn.png
)

set(RUDE_HYPE_HAS_ALL_RESOURCES TRUE)
foreach(resource_file IN LISTS RUDE_HYPE_RESOURCE_FILES)
    if(NOT EXISTS "${resource_file}")
        set(RUDE_HYPE_HAS_ALL_RESOURCES FALSE)
        message(WARNING "RUDE HYPE distribution resource missing: ${resource_file}")
    endif()
endforeach()

if(RUDE_HYPE_HAS_ALL_RESOURCES)
    juce_add_binary_data(RudeHypeAssets SOURCES ${RUDE_HYPE_RESOURCE_FILES})
    target_link_libraries(RudeHype PRIVATE RudeHypeAssets)
    target_compile_definitions(RudeHype PUBLIC RUDE_HYPE_EMBEDDED_ASSETS=1)
else()
    target_compile_definitions(RudeHype PUBLIC RUDE_HYPE_EMBEDDED_ASSETS=0)
endif()

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

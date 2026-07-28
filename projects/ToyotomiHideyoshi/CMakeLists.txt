cmake_minimum_required(VERSION 3.22)
project(ToyotomiHideyoshi VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TOYOTOMI_JUCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../JUCE" CACHE PATH
    "Path to the complete in-repository JUCE 7 source tree")

if(NOT EXISTS "${TOYOTOMI_JUCE_DIR}/CMakeLists.txt"
   OR NOT EXISTS "${TOYOTOMI_JUCE_DIR}/modules/juce_core"
   OR NOT EXISTS "${TOYOTOMI_JUCE_DIR}/modules/juce_audio_processors"
   OR NOT EXISTS "${TOYOTOMI_JUCE_DIR}/modules/juce_gui_basics")
    message(FATAL_ERROR "TOYOTOMI_JUCE_DIR must reference a complete JUCE source tree.")
endif()

add_subdirectory("${TOYOTOMI_JUCE_DIR}" JUCE)

juce_add_plugin(ToyotomiHideyoshi
    COMPANY_NAME "RAZOR FACE COMPANY"
    PLUGIN_MANUFACTURER_CODE Rzfc
    PLUGIN_CODE TyHd
    FORMATS VST3
    VST3_CATEGORIES Fx
    PRODUCT_NAME "Toyotomi Hideyoshi"
    BUNDLE_ID "com.razorfacecompany.toyotomihideyoshi"
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(ToyotomiHideyoshi)

juce_add_binary_data(ToyotomiHideyoshiAssets
    SOURCES
        Resources/reference_ui.jpg
)

target_sources(ToyotomiHideyoshi PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/UIComponents.cpp
    Source/UiSpec.cpp
    Source/PluginStateModel.cpp
)

option(TOYOTOMI_BUILD_STATE_TESTS "Build local state-model tests" ON)
if (TOYOTOMI_BUILD_STATE_TESTS)
    juce_add_console_app(ToyotomiHideyoshiStateTests PRODUCT_NAME "Toyotomi Hideyoshi State Tests")
    juce_generate_juce_header(ToyotomiHideyoshiStateTests)
    target_sources(ToyotomiHideyoshiStateTests PRIVATE Source/PluginStateModel.cpp tests/StateModelTests.cpp)
    target_include_directories(ToyotomiHideyoshiStateTests PRIVATE Source)
    target_link_libraries(ToyotomiHideyoshiStateTests PRIVATE juce::juce_audio_utils)
endif()

target_compile_definitions(ToyotomiHideyoshi PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ToyotomiHideyoshi PRIVATE
    ToyotomiHideyoshiAssets
    juce::juce_audio_utils
    juce::juce_graphics
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

cmake_minimum_required(VERSION 3.22)
project(ToyotomiHideyoshi VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_toyotomi_default_juce_dir "${CMAKE_CURRENT_LIST_DIR}/JUCE")
if (NOT EXISTS "${_toyotomi_default_juce_dir}/CMakeLists.txt")
    # Local 01_PLUGINS development keeps JUCE shared at this location; the
    # standalone CI repository uses the tracked JUCE submodule above instead.
    set(_toyotomi_default_juce_dir "${CMAKE_CURRENT_LIST_DIR}/../../JUCE")
endif()
set(TOYOTOMI_JUCE_DIR "${_toyotomi_default_juce_dir}" CACHE PATH
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
    # The JUCE 7 manifest helper must load the freshly-linked VST3. Keep that
    # scan out of this build step; bundle validation and the runtime smoke test
    # are the authoritative checks in this CI workflow.
    VST3_AUTO_MANIFEST FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(ToyotomiHideyoshi)

file (GLOB_RECURSE TOYOTOMI_RUNTIME_1024_ASSETS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/Resources/ui-v2/runtime-1024/*.png")

juce_add_binary_data(ToyotomiHideyoshiAssets
    SOURCES
        ${TOYOTOMI_RUNTIME_1024_ASSETS}
        ui/v2/runtime-manifest.json
        ui/v2/visual_acceptance_manifest.json
        Resources/ui-v2/reference/final-master-reference-1024x683.png
)

target_sources(ToyotomiHideyoshi PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditorV2.cpp
    Source/PluginStateModel.cpp
)

option(TOYOTOMI_BUILD_STATE_TESTS "Build local state-model tests" ON)
if (TOYOTOMI_BUILD_STATE_TESTS)
    juce_add_console_app(ToyotomiHideyoshiStateTests PRODUCT_NAME "Toyotomi Hideyoshi State Tests")
    juce_generate_juce_header(ToyotomiHideyoshiStateTests)
    target_sources(ToyotomiHideyoshiStateTests PRIVATE Source/PluginStateModel.cpp tests/StateModelTests.cpp)
    target_include_directories(ToyotomiHideyoshiStateTests PRIVATE Source)
    target_link_libraries(ToyotomiHideyoshiStateTests PRIVATE juce::juce_audio_utils)

    juce_add_console_app(ToyotomiHideyoshiRuntimeSmoke PRODUCT_NAME "Toyotomi Hideyoshi Runtime Smoke")
    juce_generate_juce_header(ToyotomiHideyoshiRuntimeSmoke)
    target_sources(ToyotomiHideyoshiRuntimeSmoke PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditorV2.cpp
        Source/PluginStateModel.cpp
        tests/RuntimeSmokeTests.cpp)
    target_include_directories(ToyotomiHideyoshiRuntimeSmoke PRIVATE Source)
    target_compile_definitions(ToyotomiHideyoshiRuntimeSmoke PRIVATE
        "JucePlugin_Name=\"Toyotomi Hideyoshi\"")
    target_link_libraries(ToyotomiHideyoshiRuntimeSmoke PRIVATE
        ToyotomiHideyoshiAssets
        juce::juce_audio_utils
        juce::juce_graphics)
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

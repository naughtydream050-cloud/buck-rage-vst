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
        Resources/ui/knob_ring_67.png
        Resources/ui/knob_pointer.png
        Resources/ui-cuts/bar_normal_frame.png
        Resources/ui-cuts/bar_selected_frame.png
        Resources/ui-cuts/bar_playing_frame.png
        Resources/ui-bar-map/bar_cell_base_normal.png
        Resources/ui-bar-map/bar_cell_base_selected.png
        Resources/ui-bar-map/bar_cell_base_playing.png
        Resources/ui-bar-map/bar_cell_base_selected_and_playing.png
        Resources/ui-bar-map/bar_playing_badge.png
        Resources/ui-bar-map/bar-labels/bar_label_01.png
        Resources/ui-bar-map/bar-labels/bar_label_02.png
        Resources/ui-bar-map/bar-labels/bar_label_03.png
        Resources/ui-bar-map/bar-labels/bar_label_04.png
        Resources/ui-bar-map/bar-labels/bar_label_05.png
        Resources/ui-bar-map/bar-labels/bar_label_06.png
        Resources/ui-bar-map/bar-labels/bar_label_07.png
        Resources/ui-bar-map/bar-labels/bar_label_08.png
        Resources/ui-bar-map/bar-labels/bar_label_09.png
        Resources/ui-bar-map/bar-labels/bar_label_10.png
        Resources/ui-bar-map/bar-labels/bar_label_11.png
        Resources/ui-bar-map/bar-labels/bar_label_12.png
        Resources/ui-bar-map/bar-labels/bar_label_13.png
        Resources/ui-bar-map/bar-labels/bar_label_14.png
        Resources/ui-bar-map/bar-labels/bar_label_15.png
        Resources/ui-bar-map/bar-labels/bar_label_16.png
        Resources/ui-bar-map/bar-labels/bar_label_17.png
        Resources/ui-bar-map/bar-labels/bar_label_18.png
        Resources/ui-bar-map/bar-labels/bar_label_19.png
        Resources/ui-bar-map/bar-labels/bar_label_20.png
        Resources/ui-bar-map/bar-labels/bar_label_21.png
        Resources/ui-bar-map/bar-labels/bar_label_22.png
        Resources/ui-bar-map/bar-labels/bar_label_23.png
        Resources/ui-bar-map/bar-labels/bar_label_24.png
        Resources/ui-bar-map/bar-labels/bar_label_25.png
        Resources/ui-bar-map/bar-labels/bar_label_26.png
        Resources/ui-bar-map/bar-labels/bar_label_27.png
        Resources/ui-bar-map/bar-labels/bar_label_28.png
        Resources/ui-bar-map/bar-labels/bar_label_29.png
        Resources/ui-bar-map/bar-labels/bar_label_30.png
        Resources/ui-bar-map/bar-labels/bar_label_31.png
        Resources/ui-bar-map/bar-labels/bar_label_32.png
        Resources/ui-bar-map/bar-labels/bar_label_33.png
        Resources/ui-bar-map/bar-labels/bar_label_34.png
        Resources/ui-bar-map/bar-labels/bar_label_35.png
        Resources/ui-bar-map/bar-labels/bar_label_36.png
        Resources/ui-bar-map/bar-labels/bar_label_37.png
        Resources/ui-bar-map/bar-labels/bar_label_38.png
        Resources/ui-bar-map/bar-labels/bar_label_39.png
        Resources/ui-bar-map/bar-labels/bar_label_40.png
        Resources/ui-bar-map/bar-labels/bar_label_41.png
        Resources/ui-bar-map/bar-labels/bar_label_42.png
        Resources/ui-bar-map/bar-labels/bar_label_43.png
        Resources/ui-bar-map/bar-labels/bar_label_44.png
        Resources/ui-bar-map/bar-labels/bar_label_45.png
        Resources/ui-bar-map/bar-labels/bar_label_46.png
        Resources/ui-bar-map/bar-labels/bar_label_47.png
        Resources/ui-bar-map/bar-labels/bar_label_48.png
        Resources/ui-bar-map/bar-labels/bar_label_49.png
        Resources/ui-bar-map/bar-labels/bar_label_50.png
        Resources/ui-bar-map/bar-labels/bar_label_51.png
        Resources/ui-bar-map/bar-labels/bar_label_52.png
        Resources/ui-bar-map/bar-labels/bar_label_53.png
        Resources/ui-bar-map/bar-labels/bar_label_54.png
        Resources/ui-bar-map/bar-labels/bar_label_55.png
        Resources/ui-bar-map/bar-labels/bar_label_56.png
        Resources/ui-bar-map/bar-labels/bar_label_57.png
        Resources/ui-bar-map/bar-labels/bar_label_58.png
        Resources/ui-bar-map/bar-labels/bar_label_59.png
        Resources/ui-bar-map/bar-labels/bar_label_60.png
        Resources/ui-bar-map/bar-labels/bar_label_61.png
        Resources/ui-bar-map/bar-labels/bar_label_62.png
        Resources/ui-bar-map/bar-labels/bar_label_63.png
        Resources/ui-bar-map/bar-labels/bar_label_64.png
        Resources/ui-cuts/count_normal_frame.png
        Resources/ui-cuts/count_selected_frame.png
        Resources/ui-cuts/preset_normal_frame.png
        Resources/ui-cuts/preset_selected_frame.png
        Resources/ui-cuts/length_normal_frame.png
        Resources/ui-cuts/length_selected_frame.png
        Resources/ui-cuts/tab_normal_frame.png
        Resources/ui-cuts/tab_selected_frame.png
        Resources/ui-cuts/tab_selected_fill.png
        Resources/ui-tabs-final/tab_strip_selected_1_16.png
        Resources/ui-tabs-final/tab_strip_selected_17_32.png
        Resources/ui-tabs-final/tab_strip_selected_33_48.png
        Resources/ui-tabs-final/tab_strip_selected_49_64.png
        Resources/ui-cuts/output_meter_reference.png
        Resources/ui-cuts/meter_led_strip.png
        Resources/ui-cuts/bypass_off.png
        ui/spec/ui-spec.json
        ui/spec/assets-manifest.json
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

    juce_add_console_app(ToyotomiHideyoshiRuntimeSmoke PRODUCT_NAME "Toyotomi Hideyoshi Runtime Smoke")
    juce_generate_juce_header(ToyotomiHideyoshiRuntimeSmoke)
    target_sources(ToyotomiHideyoshiRuntimeSmoke PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/UIComponents.cpp
        Source/UiSpec.cpp
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

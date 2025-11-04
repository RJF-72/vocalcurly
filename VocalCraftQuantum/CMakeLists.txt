cmake_minimum_required(VERSION 3.19)
project(VocalCraftQuantum VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE setup: use FetchContent or local path if provided
include(FetchContent)
if(NOT DEFINED JUCE_DIR)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.5
    )
    FetchContent_MakeAvailable(JUCE)
else()
    add_subdirectory(${JUCE_DIR} JUCE-build)
endif()

# Sources
set(SRC
    Source/Plugin/PluginProcessor.cpp
    Source/GUI/PluginEditor.cpp
    Source/GUI/PluginEditor.h
    Source/GUI/SpectralDisplay.cpp
    Source/GUI/SpectralDisplay.h
    Source/GUI/ParameterControls.h
    Source/GUI/ParameterControls.cpp
    Source/DSP/SpectralAnalyzer.h
    Source/DSP/SpectralAnalyzer.cpp
    Source/Core/QuantumParameters.h
    Source/AI/AIModelInterface.h
    Source/AI/AIModelInterface.cpp
)

add_executable(VocalCraftQuantum_Standalone ${SRC})
target_compile_definitions(VocalCraftQuantum_Standalone PRIVATE
    JUCE_APPLICATION_NAME_STRING="VocalCraftQuantum"
    JUCE_APPLICATION_VERSION_STRING="0.1.0"
)
target_link_libraries(VocalCraftQuantum_Standalone PRIVATE
    juce::juce_gui_basics
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
)

# Torch & ONNX Runtime (optional, guarded)
option(ENABLE_TORCH "Enable LibTorch integration" ON)
option(ENABLE_ONNX "Enable ONNX Runtime integration" ON)

if(ENABLE_TORCH)
    find_package(Torch REQUIRED)
    target_link_libraries(VocalCraftQuantum_Standalone PRIVATE ${TORCH_LIBRARIES})
    target_compile_definitions(VocalCraftQuantum_Standalone PRIVATE ENABLE_TORCH)
endif()

if(ENABLE_ONNX)
    find_package(ONNXRuntime REQUIRED)
    target_link_libraries(VocalCraftQuantum_Standalone PRIVATE ONNXRuntime::ONNXRuntime)
    target_compile_definitions(VocalCraftQuantum_Standalone PRIVATE ENABLE_ONNX)
endif()

# Windows subsystem tweaks
if(WIN32)
    target_compile_definitions(VocalCraftQuantum_Standalone PRIVATE JUCE_WIN_PER_MONITOR_DPI_AWARE=1)
endif()

# JUCE Plugin target (VST3)
juce_add_plugin(VocalCraftQuantum_Plugin
    COMPANY_NAME "VocalCurly"
    PLUGIN_NAME  "VocalCraftQuantum"
    PLUGIN_CODE  VcQt
    FORMATS      VST3
    PRODUCT_NAME "VocalCraftQuantum"
    IS_SYNTH     FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    SOURCES ${SRC}
)

target_link_libraries(VocalCraftQuantum_Plugin PRIVATE
    juce::juce_gui_basics
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
)

if(ENABLE_TORCH)
    target_link_libraries(VocalCraftQuantum_Plugin PRIVATE ${TORCH_LIBRARIES})
    target_compile_definitions(VocalCraftQuantum_Plugin PRIVATE ENABLE_TORCH)
endif()

if(ENABLE_ONNX)
    target_link_libraries(VocalCraftQuantum_Plugin PRIVATE ONNXRuntime::ONNXRuntime)
    target_compile_definitions(VocalCraftQuantum_Plugin PRIVATE ENABLE_ONNX)
endif()
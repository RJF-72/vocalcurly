cmake_minimum_required(VERSION 3.19)
project(TitanVocal VERSION 0.1.0)

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
    Source/Plugin/CreateFilter.cpp
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

# Standalone application will be provided by the JUCE plugin wrapper when including the Standalone format.

# Torch & ONNX Runtime (optional, guarded)
option(ENABLE_TORCH "Enable LibTorch integration" ON)
option(ENABLE_ONNX "Enable ONNX Runtime integration" ON)

# JUCE Plugin target (VST3)
juce_add_plugin(TitanVocal_Plugin
    COMPANY_NAME "TitanVocal"
    PLUGIN_NAME  "TitanVocal"
    PLUGIN_CODE  TiVc
    FORMATS      VST3 Standalone
    PRODUCT_NAME "TitanVocal"
    IS_SYNTH     FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    SOURCES ${SRC}
)
juce_generate_juce_header(TitanVocal_Plugin)

target_link_libraries(TitanVocal_Plugin PRIVATE
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
)

# Explicitly add project sources to the shared code target to ensure they are compiled and linked
target_sources(TitanVocal_Plugin PRIVATE
    ${SRC}
)

if(ENABLE_TORCH)
    find_package(Torch REQUIRED)
    target_link_libraries(TitanVocal_Plugin PRIVATE ${TORCH_LIBRARIES})
    target_compile_definitions(TitanVocal_Plugin PRIVATE ENABLE_TORCH)

    # Copy Torch runtime DLLs next to the Standalone executable on Windows
    if(WIN32)
        # Derive LibTorch root from Torch_DIR
        get_filename_component(_TORCH_PARENT "${Torch_DIR}" DIRECTORY)        # share/cmake
        get_filename_component(_TORCH_PARENT2 "${_TORCH_PARENT}" DIRECTORY)   # share
        get_filename_component(TORCH_ROOT "${_TORCH_PARENT2}" DIRECTORY)      # LibTorch root
        set(TORCH_BIN_DIR "${TORCH_ROOT}/bin")
        set(TORCH_LIB_DIR "${TORCH_ROOT}/lib")
        # JUCE generates a Standalone target alongside the plugin shared code
        add_custom_command(TARGET TitanVocal_Plugin_Standalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${TORCH_BIN_DIR}" "$<TARGET_FILE_DIR:TitanVocal_Plugin_Standalone>"
            COMMAND ${CMAKE_COMMAND} -E copy_directory "${TORCH_LIB_DIR}" "$<TARGET_FILE_DIR:TitanVocal_Plugin_Standalone>"
        )
    endif()
endif()

if(ENABLE_ONNX)
    find_package(ONNXRuntime REQUIRED)
    target_link_libraries(TitanVocal_Plugin PRIVATE ONNXRuntime::ONNXRuntime)
    target_compile_definitions(TitanVocal_Plugin PRIVATE ENABLE_ONNX)
endif()

# Windows subsystem tweaks
if(WIN32)
    target_compile_definitions(TitanVocal_Plugin PRIVATE JUCE_WIN_PER_MONITOR_DPI_AWARE=1)
endif()

## (Removed duplicate Torch/ONNX linking/definitions to avoid redundancy)
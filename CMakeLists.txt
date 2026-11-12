cmake_minimum_required(VERSION 3.22)

project(Oregana VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Oregana has no plug-in yet: this builds the JUCE-free engine, its offline
# tools and its tests.
option(OREGANA_BUILD_TOOLS "Build the offline renderers and pipe tools" ON)
option(OREGANA_ENABLE_SANITIZERS "Instrument development builds with ASan and UBSan" OFF)
option(OREGANA_BUILD_FUZZER "Build OreganaDspFuzz as a Clang libFuzzer target" OFF)
option(OREGANA_REQUIRE_PYTHON_TESTS "Fail configuration when the Python checks cannot run" OFF)

include(CTest)

if(OREGANA_ENABLE_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "OREGANA_ENABLE_SANITIZERS requires Clang or GCC")
    endif()
    add_compile_options(-fsanitize=address,undefined,float-cast-overflow
                        -fno-sanitize-recover=all -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined,float-cast-overflow)
endif()

set(OREGANA_DSP_SOURCES Source/DSP/OreganaEngine.cpp)
set(OREGANA_DSP_HEADERS
    Source/DSP/DspMath.h
    Source/DSP/FluePipe.h
    Source/DSP/Mechanics.h
    Source/DSP/Nave.h
    Source/DSP/OreganaControls.h
    Source/DSP/OreganaEngine.h
    Source/DSP/OreganaInstrument.h
    Source/DSP/OreganaPresets.h
    Source/DSP/OutputStage.h
    Source/DSP/PipeRegulation.h
    Source/DSP/ReedPipe.h
    Source/DSP/Stops.h
    Source/DSP/Temperament.h
    Source/DSP/Wind.h)

if(MSVC)
    set(OREGANA_WARNINGS /W4 /permissive-)
else()
    set(OREGANA_WARNINGS -Wall -Wextra -Wpedantic -Wshadow)
endif()

add_library(OreganaDSP STATIC ${OREGANA_DSP_SOURCES} ${OREGANA_DSP_HEADERS})
target_include_directories(OreganaDSP PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_compile_features(OreganaDSP PUBLIC cxx_std_20)
set_target_properties(OreganaDSP PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(OreganaDSP PRIVATE ${OREGANA_WARNINGS})

# The Rack Extension toolchain is C++17 with -fno-builtin. Compiling the
# shipping engine that way here keeps Source/DSP portable for a Rack adapter.
if(NOT MSVC)
    # Tests/InstrumentTests.cpp is there for the header-only playing layer.
    add_library(OreganaDSPCxx17 OBJECT ${OREGANA_DSP_SOURCES} Tests/InstrumentTests.cpp)
    target_include_directories(OreganaDSPCxx17 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    set_target_properties(OreganaDSPCxx17 PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
    target_compile_options(OreganaDSPCxx17 PRIVATE ${OREGANA_WARNINGS} -fno-builtin -fno-exceptions)
endif()

find_package(Threads REQUIRED)

# Writes and checks Source/DSP/PipeRegulation.h by sounding every pipe. The
# engine never runs it; it is built for the PipeRegulation check.
add_executable(GeneratePipeRegulation Tools/GeneratePipeRegulation.cpp)
target_include_directories(GeneratePipeRegulation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source"
                                                          "${CMAKE_CURRENT_SOURCE_DIR}/Tools")
target_link_libraries(GeneratePipeRegulation PRIVATE Threads::Threads)
target_compile_options(GeneratePipeRegulation PRIVATE ${OREGANA_WARNINGS})

if(OREGANA_BUILD_TOOLS)
    # A MIDI file through one registration with any control changed, and each
    # registration's level on a reference chord.
    add_executable(RenderOregana Tools/RenderOregana.cpp)
    target_link_libraries(RenderOregana PRIVATE OreganaDSP Threads::Threads)
    target_compile_options(RenderOregana PRIVATE ${OREGANA_WARNINGS})

    # One pipe of any rank alone: its speech, pitch, level and spectrum.
    add_executable(ProbePipe Tools/ProbePipe.cpp)
    target_include_directories(ProbePipe PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
    target_compile_options(ProbePipe PRIVATE ${OREGANA_WARNINGS})

    # The README demos from Docs/audio/midi (Tools/PresetDemoRenderer.h is
    # the shared portfolio copy; do not edit it here).
    add_executable(RenderOreganaPresetDemos Tools/RenderPresetDemos.cpp)
    target_link_libraries(RenderOreganaPresetDemos PRIVATE OreganaDSP Threads::Threads)
    target_compile_options(RenderOreganaPresetDemos PRIVATE ${OREGANA_WARNINGS})
endif()

if(BUILD_TESTING)
    if(OREGANA_REQUIRE_PYTHON_TESTS)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    else()
        find_package(Python3 COMPONENTS Interpreter)
        if(NOT Python3_Interpreter_FOUND)
            message(WARNING "Python 3 not found: Oregana.DemoMidiSources is not registered")
        endif()
    endif()

    add_executable(OreganaEngineTests Tests/EngineTests.cpp)
    target_link_libraries(OreganaEngineTests PRIVATE OreganaDSP)
    target_compile_options(OreganaEngineTests PRIVATE ${OREGANA_WARNINGS})
    add_test(NAME Oregana.Engine COMMAND OreganaEngineTests)
    set_tests_properties(Oregana.Engine PROPERTIES TIMEOUT 600)

    # The playing layer the front ends share (OreganaInstrument.h).
    add_executable(OreganaInstrumentTests Tests/InstrumentTests.cpp)
    target_link_libraries(OreganaInstrumentTests PRIVATE OreganaDSP)
    target_compile_options(OreganaInstrumentTests PRIVATE ${OREGANA_WARNINGS})
    add_test(NAME Oregana.Instrument COMMAND OreganaInstrumentTests)
    set_tests_properties(Oregana.Instrument PROPERTIES TIMEOUT 600)

    # The committed regulation table is what sounding every pipe afresh gives.
    add_test(NAME Oregana.PipeRegulation
             COMMAND GeneratePipeRegulation --check "${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP/PipeRegulation.h")
    set_tests_properties(Oregana.PipeRegulation PROPERTIES TIMEOUT 900)

    # U4 input fuzzing. OreganaDspFuzzReplay is the harness with
    # FuzzHarness.h's main, replaying Tests/Fixtures/fuzz/ and then seeded
    # inputs; with OREGANA_BUILD_FUZZER (Clang) OreganaDspFuzz is a libFuzzer
    # target over coverage-instrumented DSP.
    file(GLOB OREGANA_FUZZ_REGRESSIONS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Fixtures/fuzz/*")
    add_executable(OreganaDspFuzzReplay Tests/DspFuzz.cpp)
    target_link_libraries(OreganaDspFuzzReplay PRIVATE OreganaDSP)
    target_compile_definitions(OreganaDspFuzzReplay PRIVATE PROTOCODUS_FUZZ_MAIN)
    add_test(NAME Oregana.DspFuzzReplay COMMAND OreganaDspFuzzReplay -seed=1 -runs=300 ${OREGANA_FUZZ_REGRESSIONS})
    set_tests_properties(Oregana.DspFuzzReplay PROPERTIES TIMEOUT 600)
    if(OREGANA_BUILD_FUZZER)
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR "OREGANA_BUILD_FUZZER requires Clang")
        endif()
        add_executable(OreganaDspFuzz Tests/DspFuzz.cpp ${OREGANA_DSP_SOURCES})
        target_include_directories(OreganaDspFuzz PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
        target_compile_options(OreganaDspFuzz PRIVATE
            -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(OreganaDspFuzz PRIVATE -fsanitize=fuzzer,address,undefined)
        add_test(NAME Oregana.DspFuzz COMMAND OreganaDspFuzz -seed=1 -max_total_time=60 ${OREGANA_FUZZ_REGRESSIONS})
        set_tests_properties(Oregana.DspFuzz PROPERTIES TIMEOUT 600 LABELS fuzz)
    endif()

    if(Python3_Interpreter_FOUND)
        # The committed demo phrases are what the writer writes (G3a).
        add_test(NAME Oregana.DemoMidiSources
                 COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/Tools/write_preset_midi.py" --check)
    endif()

    if(OREGANA_BUILD_TOOLS)
        # Every demo renders finite, audible, under the ceiling, 8-12 s long
        # and bit-identical on a second render.
        add_test(NAME Oregana.AudioDemos
                 COMMAND RenderOreganaPresetDemos --jobs 4 --check "${CMAKE_CURRENT_SOURCE_DIR}/Docs/audio/midi")
        set_tests_properties(Oregana.AudioDemos PROPERTIES TIMEOUT 600 LABELS audio)

        # U7: the demos' realtime factor on one thread against the budget in
        # Tools/RenderPresetDemos.cpp. Timing means nothing in an unoptimised
        # or instrumented build, so only Release without sanitizers has it.
        if(CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT OREGANA_ENABLE_SANITIZERS)
            add_test(NAME Oregana.CpuBudget
                     COMMAND RenderOreganaPresetDemos --cpu --json "${CMAKE_CURRENT_BINARY_DIR}/cpu.json"
                             "${CMAKE_CURRENT_SOURCE_DIR}/Docs/audio/midi")
            set_tests_properties(Oregana.CpuBudget PROPERTIES TIMEOUT 900 LABELS perf RUN_SERIAL TRUE)
        endif()
    endif()

    # Every test carries exactly one tier label so CI selects tiers with
    # `ctest -L`; tests not labelled above are unit contracts.
    get_property(OREGANA_TESTS DIRECTORY PROPERTY TESTS)
    foreach(test IN LISTS OREGANA_TESTS)
        get_test_property(${test} LABELS test_labels)
        if(NOT test_labels)
            set_tests_properties(${test} PROPERTIES LABELS unit)
        endif()
    endforeach()
endif()

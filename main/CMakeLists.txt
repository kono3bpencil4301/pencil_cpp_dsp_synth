cmake_minimum_required(VERSION 3.20)

project(pencil_mini_audio_lab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main
    src/main.cpp
    src/audio/wav_and_pcm.cpp
    src/synth_renderer.cpp
    src/config/config_manager.cpp
    src/midi/midi_file_reader.cpp
    src/midi/midi_json_writer.cpp)
target_include_directories(main PRIVATE
    src src/audio src/config src/filters src/midi src/synth)

if(MSVC)
    target_compile_options(main PRIVATE /utf-8)
endif()

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(nlohmann_json)

target_link_libraries(main nlohmann_json::nlohmann_json)

include(CTest)
if(BUILD_TESTING)
    add_executable(biquad_test tests/biquad_test.cpp)
    target_include_directories(biquad_test PRIVATE
        src src/filters src/synth)
    add_test(NAME biquad_test COMMAND biquad_test)

    add_executable(midi_file_reader_test
        tests/midi_file_reader_test.cpp
        src/midi/midi_file_reader.cpp
        src/midi/midi_json_writer.cpp
        src/config/config_manager.cpp)
    target_include_directories(midi_file_reader_test PRIVATE
        src src/audio src/config src/filters src/midi src/synth)
    add_test(NAME midi_file_reader_test COMMAND midi_file_reader_test)
endif()

cmake_minimum_required(VERSION 3.20)

project(pencil_mini_audio_lab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main src/main.cpp src/wav_and_pcm.cpp src/synth_renderer.cpp src/config_manager.cpp)

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

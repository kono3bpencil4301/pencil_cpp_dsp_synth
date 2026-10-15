cmake_minimum_required(VERSION 3.20)

project(pencil_mini_audio_lab)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main src/main.cpp src/wav_and_pcm.cpp src/synth_renderer.cpp src/config_manager.cpp)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(main nlohmann_json::nlohmann_json)

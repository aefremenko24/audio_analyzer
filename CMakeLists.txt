cmake_minimum_required(VERSION 3.26)
project(audio_analyzer)

set(CMAKE_CXX_STANDARD 23)

execute_process(COMMAND "make install-deps")


include_directories(${CMAKE_SOURCE_DIR}/libs/portaudio/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/fftw-3.3.10/api)

add_executable(audio_analyzer main.cpp)

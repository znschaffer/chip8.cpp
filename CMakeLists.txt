cmake_minimum_required(VERSION 3.31)
project(chip8cpp)

set(CMAKE_CXX_STANDARD 20)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})



add_executable(chip8cpp main.cpp Emulator.cpp)
target_link_libraries(chip8cpp ${SDL2_LIBRARIES})

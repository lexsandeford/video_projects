cmake_minimum_required(VERSION 3.10)

# Force correct architecture
set(CMAKE_OSX_ARCHITECTURES arm64)

project(SDLExample)
set(CMAKE_CXX_STANDARD 11)

# Use Homebrew's SDL2 paths on Apple Silicon
set(SDL2_ROOT /opt/homebrew/opt/sdl2)
set(CMAKE_PREFIX_PATH ${SDL2_ROOT})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(main main.cpp)
target_link_libraries(main ${SDL2_LIBRARIES})
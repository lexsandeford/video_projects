cmake_minimum_required(VERSION 3.10)

# Force correct architecture (Apple Silicon)
set(CMAKE_OSX_ARCHITECTURES arm64)

project(SDLExample)
set(CMAKE_CXX_STANDARD 17)

# -----------------------------
# SDL2 Setup (Homebrew arm64)
# -----------------------------
set(SDL2_ROOT /opt/homebrew/opt/sdl2)
set(CMAKE_PREFIX_PATH ${SDL2_ROOT})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# -----------------------------
# FFmpeg Setup (force /opt/homebrew)
# -----------------------------
set(FFMPEG_ROOT /opt/homebrew/opt/ffmpeg)

find_package(PkgConfig REQUIRED)

# Ensure pkg-config searches only arm64 Homebrew
set(ENV{PKG_CONFIG_PATH} "${FFMPEG_ROOT}/lib/pkgconfig")

pkg_check_modules(FFMPEG REQUIRED
    libavformat
    libavcodec
    libavutil
    libswscale
)

include_directories(${FFMPEG_INCLUDE_DIRS})
link_directories(${FFMPEG_LIBRARY_DIRS})

# -----------------------------
# Build executable
# -----------------------------
add_executable(main main.cpp)

target_link_libraries(main
    ${SDL2_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    "-framework CoreVideo"
    "-framework VideoToolbox" # HW decoding
)
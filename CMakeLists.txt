project(AUDIOSPLIT)
cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
option(AUDIOSPLIT_DISABLE_YASM "Disable YASM when building FFMPEG" FALSE)

set(FFMPEG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/ffmpeg)
set(FFMPEG_CONFIGURE_OPTIONS)
if(AUDIOSPLIT_DISABLE_YASM)
    set(FFMPEG_CONFIGURE_OPTIONS ${FFMPEG_CONFIGURE_OPTIONS} --disable-yasm)
endif()
set(FFMPEG_INSTALL_DIR ${PROJECT_BINARY_DIR}/ffmpeg)

include(ExternalProject)
ExternalProject_Add(ffmpeg
    SOURCE_DIR ${FFMPEG_SOURCE_DIR}
    CONFIGURE_COMMAND ${FFMPEG_SOURCE_DIR}/configure
        --prefix=${FFMPEG_INSTALL_DIR} ${FFMPEG_CONFIGURE_OPTIONS}
    BUILD_COMMAND make -j8
    INSTALL_DIR ${FFMPEG_INSTALL_DIR})
ExternalProject_Add_step(ffmpeg create_ffmpeg_install_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${FFMPEG_INSTALL_DIR}
    DEPENDERS install
    ALWAYS OFF)

# TODO: Disable ffmpeg build warnings:
#   -Wno-unused-function
#   -Wno-sometimes-uninitialized
#   -Wno-deprecated-declarations
#   -Wno-incompatible-pointer-types-discards-qualifiers

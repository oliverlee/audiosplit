project(AUDIOSPLIT)
cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
option(AUDIOSPLIT_DISABLE_YASM "Disable YASM when building FFMPEG" FALSE)

set(FFMPEG_SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/ffmpeg)
set(FFMPEG_CONFIGURE_OPTIONS --disable-programs --disable-doc
    --disable-static --enable-shared)
if(AUDIOSPLIT_DISABLE_YASM)
    set(FFMPEG_CONFIGURE_OPTIONS ${FFMPEG_CONFIGURE_OPTIONS} --disable-yasm)
endif()

set(FFMPEG_INSTALL_DIR ${PROJECT_BINARY_DIR}/ffmpeg)
set(FFMPEG_INCLUDE_DIR ${PROJECT_BINARY_DIR}/ffmpeg/include)
set(FFMPEG_LIBRARY_DIR ${PROJECT_BINARY_DIR}/ffmpeg/lib)

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

set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${FFMPEG_INSTALL_DIR}/lib/pkgconfig")
pkg_search_module(AVCODEC REQUIRED libavcodec)
pkg_search_module(AVFORMAT REQUIRED libavformat)
pkg_search_module(AVUTIL REQUIRED libavutil)
set(FFMPEG_LIBRARIES ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${AVUTIL_LIBRARIES})

include_directories(${FFMPEG_INCLUDE_DIR})
link_directories(${FFMPEG_LIBRARY_DIR})

include_directories(inc)
add_executable(audiosplit src/main.cc src/decoder.cc)
add_dependencies(audiosplit ffmpeg)
target_link_libraries(audiosplit ${FFMPEG_LIBRARIES})

# TODO: Disable ffmpeg build warnings:
#   -Wno-unused-function
#   -Wno-sometimes-uninitialized
#   -Wno-deprecated-declarations
#   -Wno-incompatible-pointer-types-discards-qualifiers

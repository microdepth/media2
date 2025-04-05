cmake_minimum_required(VERSION 3.26)
project(media2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Release)

add_executable(media2 main.cpp
        stb_image_write.h)

find_package(FFmpeg REQUIRED COMPONENTS avformat avcodec avutil swscale swresample)
target_include_directories(media2 PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(media2 PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(media2 PRIVATE ${FFMPEG_LIBRARIES})

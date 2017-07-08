cmake_minimum_required(VERSION 3.7)
project(stream)

set(CMAKE_C_STANDARD 99)

link_libraries(ws2_32.lib)
aux_source_directory(src STREAM_SOURCE)

add_executable(stream ${STREAM_SOURCE})

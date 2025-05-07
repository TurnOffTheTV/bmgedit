set(CMAKE_CXX_STANDARD 23)
cmake_minimum_required(VERSION 3.31)
project(bmgedit VERSION 1.0.0 LANGUAGES C CXX)

file(GLOB_RECURSE SRCS *.c*)

add_executable(bmgedit ${SRCS})

include("../cmake/FileEmbed.cmake")
FileEmbedSetup()
FileEmbedAdd(${CMAKE_SOURCE_DIR}/empty.bmg)
target_link_libraries(bmgedit file_embed)

target_include_directories(bmgedit PUBLIC "${CMAKE_BINARY_DIR}" PUBLIC "../include")
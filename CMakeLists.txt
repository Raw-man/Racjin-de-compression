cmake_minimum_required(VERSION 3.5)

project(racjin-decompression LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include/)

add_executable(racjin-decompression src/main.cpp src/decode.cpp src/encode.cpp)

target_compile_options(racjin-decompression PUBLIC -static)

target_link_libraries(racjin-decompression -static-libgcc -static-libstdc++)

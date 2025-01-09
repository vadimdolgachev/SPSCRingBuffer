cmake_minimum_required(VERSION 3.30)
project(SPSC_RingBuffer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")

add_executable(SPSC_RingBuffer main.cpp
        SPSCRingBuffer.h)
target_compile_options(SPSC_RingBuffer PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-interference-size)

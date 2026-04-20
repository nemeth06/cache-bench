cmake_minimum_required(VERSION 3.20)
project(cache-bench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR
        "[cache-bench] This project requires Linux (perf_event_open). "
        "Detected: ${CMAKE_SYSTEM_NAME}")
endif()

# Compiler flags
# -O2: fast enough to measure cache effects, but avoids -O3 auto-vectorisation
add_compile_options(
    -O2
    -Wall
    -Wextra
    -Wpedantic
)

# Sources
# Benchmarks are header-only plugin structs included by main.cpp.
# Only the two translation units below need to be compiled.
add_executable(cache-bench
    src/main.cpp
    src/counters.cpp
)

target_include_directories(cache-bench PRIVATE include .)

cmake_minimum_required(VERSION 3.28)
project(proj_cmake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(proj_cmake main.cpp)

target_compile_options(proj_cmake PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /W4 /permissive- /std:c++latest>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -std=c++23>
)

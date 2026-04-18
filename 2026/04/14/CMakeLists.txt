cmake_minimum_required(VERSION 3.20)
project(MyProject)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 10.1.1
)

FetchContent_MakeAvailable(fmt)

add_executable(binary1 src/main1.cpp)
add_executable(binary2 src/main2.cpp)
add_executable(binary3 src/main3.cpp)
add_executable(binary4 src/main4.cpp)
add_executable(binary5 src/main5.cpp)

target_link_libraries(binary2 fmt::fmt)
target_link_libraries(binary5 fmt::fmt-header-only)

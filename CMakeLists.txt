cmake_minimum_required(VERSION 3.8)
project(rmc_parser)

set(CMAKE_CXX_STANDARD 17)

include(external/external_cmake)

set(SOURCE_FILES main.cpp)
add_definitions(-Wno-deprecated )
add_executable(sample ${SOURCE_FILES} )

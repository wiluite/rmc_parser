cmake_minimum_required(VERSION 3.8)
project(rmc_parser)

set(CMAKE_CXX_STANDARD 17)

set (BOOST_TEST_COMPONENTS  unit_test_framework )
find_package (Boost REQUIRED COMPONENTS ${BOOST_TEST_COMPONENTS} )
include_directories(include ${Boost_INCLUDE_DIRS})
if (UNIX)
    add_definitions( -DBOOST_TEST_DYN_LINK )
endif()

include(external/external_cmake)



set(SOURCE_FILES main.cpp include/machine.h include/states.h include/mach_mem.h include/parser.h)
add_definitions(-Wno-deprecated )
add_executable(sample ${SOURCE_FILES})

add_definitions(-Wno-unused-result )
add_executable(test_app test.cpp include/machine.h include/states.h include/mach_mem.h include/parser.h)
target_link_libraries (test_app ${Boost_LIBRARIES}  )
add_test (test_app test_app)


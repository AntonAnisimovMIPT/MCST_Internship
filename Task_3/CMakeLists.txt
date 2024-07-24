cmake_minimum_required(VERSION 3.10)

project(sequence_formation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.82 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(sequence_formation sequence_formation.cpp)

target_link_libraries(sequence_formation ${Boost_LIBRARIES})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_target_properties(sequence_formation PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc -static")
endif()

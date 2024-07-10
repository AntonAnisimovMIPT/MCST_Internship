cmake_minimum_required(VERSION 3.10)

project(Pseudorandom_machine_generator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.82 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(pseudorandom_machine_generator pseudorandom_machine_generator.cpp)

target_link_libraries(pseudorandom_machine_generator ${Boost_LIBRARIES})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_target_properties(pseudorandom_machine_generator PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc -static")
endif()

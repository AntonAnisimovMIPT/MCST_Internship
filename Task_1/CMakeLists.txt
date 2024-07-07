cmake_minimum_required(VERSION 3.10)

project(Converter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Boost 1.82 REQUIRED COMPONENTS program_options)

include_directories(${Boost_INCLUDE_DIRS})
add_executable(converter converter.cpp)

target_link_libraries(converter ${Boost_LIBRARIES})

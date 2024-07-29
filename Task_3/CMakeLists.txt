cmake_minimum_required(VERSION 3.10)

project(sequence_formation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#для отладки, если не надо, то потом убрать
#[[
set(CMAKE_BUILD_TYPE Debug) 
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
endif()
]]

include_directories(${CMAKE_SOURCE_DIR}/include)
set(SOURCES
    src/utility_functions.cpp
    src/sequence_formation.cpp
)

add_executable(sequence_formation ${SOURCES})

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.82 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(sequence_formation ${Boost_LIBRARIES})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_target_properties(sequence_formation PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc -static")
endif()

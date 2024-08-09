cmake_minimum_required(VERSION 3.10)

project(associative_cache_memory)

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
    src/associative_cache_memory.cpp
    src/functions.cpp
)

add_executable(associative_cache_memory ${SOURCES})

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.82 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(associative_cache_memory ${Boost_LIBRARIES})

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_target_properties(associative_cache_memory PROPERTIES LINK_FLAGS "-static-libstdc++ -static-libgcc -static")
endif()

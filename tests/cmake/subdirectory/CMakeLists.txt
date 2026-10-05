# Embeds mhda with add_subdirectory, the way FetchContent does. The parent
# sets no build type, and mhda must not choose one for it, nor add its
# tests, examples or install rules. Configure with -DMHDA_SOURCE_DIR=<repo>.
cmake_minimum_required(VERSION 3.14)
project(mhda_subdirectory_consumer LANGUAGES CXX)

add_subdirectory(${MHDA_SOURCE_DIR} mhda)

if(CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "mhda set the parent's build type to '${CMAKE_BUILD_TYPE}'")
endif()
foreach(t mhda_tests mhda_basic mhda_by_key)
    if(TARGET ${t})
        message(FATAL_ERROR "mhda added its target '${t}' to the parent build")
    endif()
endforeach()

add_executable(consumer ../consumer.cpp)
target_link_libraries(consumer PRIVATE mhda::mhda)

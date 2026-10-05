# Consumes an installed mhda through find_package. The project asks for
# C++14: the mhda::mhda target must raise its consumers to C++17 itself.
cmake_minimum_required(VERSION 3.14)
project(mhda_find_package_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
find_package(mhda REQUIRED)
add_executable(consumer ../consumer.cpp)
target_link_libraries(consumer PRIVATE mhda::mhda)

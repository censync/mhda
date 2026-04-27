cmake_minimum_required(VERSION 3.14)

project(mhda
    VERSION 0.1.0
    DESCRIPTION "MultiChain Hierarchical Deterministic Address (MHDA) — C++ port of go-mhda"
    LANGUAGES CXX
)

option(MHDA_BUILD_TESTS    "Build the mhda test suite"          ON)
option(MHDA_BUILD_EXAMPLES "Build the mhda example programs"    ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

add_library(mhda
    src/algorithm.cpp
    src/address.cpp
    src/chain.cpp
    src/compatibility.cpp
    src/derivation_path.cpp
    src/derivation_type.cpp
    src/detail.cpp
    src/error.cpp
    src/format.cpp
    src/hash.cpp
    src/network_type.cpp
    src/nss_parser.cpp
    src/parser.cpp
)
add_library(mhda::mhda ALIAS mhda)

target_include_directories(mhda
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(mhda PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

if(MHDA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MHDA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
install(TARGETS mhda
    EXPORT  mhdaTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT mhdaTargets
    FILE        mhdaTargets.cmake
    NAMESPACE   mhda::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mhda
)

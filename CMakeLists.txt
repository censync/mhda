cmake_minimum_required(VERSION 3.14)

project(mhda
    VERSION 1.2.0
    DESCRIPTION "MultiChain Hierarchical Deterministic Address (MHDA) — C++ port of go-mhda"
    LANGUAGES CXX
)

# Built on its own, or embedded in another project (add_subdirectory,
# FetchContent)? Only a top-level build picks a build type and, by default,
# builds the tests and examples and generates install rules; an embedding
# project keeps its own settings and can opt in.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(MHDA_IS_TOP_LEVEL ON)
else()
    set(MHDA_IS_TOP_LEVEL OFF)
endif()

option(MHDA_BUILD_TESTS    "Build the mhda test suite"          ${MHDA_IS_TOP_LEVEL})
option(MHDA_BUILD_EXAMPLES "Build the mhda example programs"    ${MHDA_IS_TOP_LEVEL})
option(MHDA_INSTALL        "Generate the mhda install rules"    ${MHDA_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MHDA_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

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
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# The public headers need C++17 (std::optional, inline variables): a
# consumer that asks for an older standard is raised to C++17.
target_compile_features(mhda PUBLIC cxx_std_17)

set_target_properties(mhda PROPERTIES POSITION_INDEPENDENT_CODE ON)
get_target_property(MHDA_LIBRARY_TYPE mhda TYPE)
if(MHDA_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    # There are no export macros: a shared mhda exports every symbol (and
    # needs WINDOWS_EXPORT_ALL_SYMBOLS for a DLL).
    set_target_properties(mhda PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    # A static mhda linked into a consumer's shared object stays private to it.
    set_target_properties(mhda PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

if(MHDA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MHDA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(MHDA_INSTALL)
    include(CMakePackageConfigHelpers)

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

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/mhdaConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/mhdaConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mhda
    )
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/mhdaConfigVersion.cmake"
        VERSION       ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/mhdaConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/mhdaConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mhda
    )
endif()

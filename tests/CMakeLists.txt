add_executable(mhda_tests
    test_main.cpp
    test_chain.cpp
    test_derivation_path.cpp
    test_parse.cpp
    test_validate.cpp
    test_hash.cpp
    test_misc.cpp
    test_fuzz.cpp
    test_edge.cpp
)
target_link_libraries(mhda_tests PRIVATE mhda)
target_include_directories(mhda_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME mhda_tests COMMAND mhda_tests)

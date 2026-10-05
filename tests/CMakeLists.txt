add_executable(mhda_tests
    test_main.cpp
    test_chain.cpp
    test_derivation_path.cpp
    test_parse.cpp
    test_validate.cpp
    test_wallet.cpp
    test_hash.cpp
    test_misc.cpp
    test_fuzz.cpp
    test_edge.cpp
    test_boundary.cpp
    test_hardening.cpp
    test_index_range.cpp
    test_static_init.cpp
    test_urn_conformance.cpp
    test_state.cpp
)
target_link_libraries(mhda_tests PRIVATE mhda)
target_include_directories(mhda_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# The conformance tables shared with go-mhda.
target_compile_definitions(mhda_tests PRIVATE
    MHDA_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_test(NAME mhda_tests COMMAND mhda_tests)

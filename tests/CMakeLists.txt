find_package(GTest REQUIRED)

set(TEST_SOURCES
    softioc_runner.cpp
    softioc_fixture.cpp
    actions/gtest_print_node.cpp
    epics/gtest_ca_pv.cpp
    epics/gtest_ca_pv_manager.cpp
)

add_executable(unit_tests ${TEST_SOURCES})

target_include_directories(unit_tests PUBLIC include)

target_link_libraries(unit_tests
    PRIVATE
        bchtree
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests
    DISCOVERY_MODE PRE_TEST            # discover right before execution
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    PROPERTIES TIMEOUT 30             # default timeout per test case
)

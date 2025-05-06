add_executable(bbtape_tests
    balance_ram_test.cpp
)

target_link_libraries(bbtape_tests
    PRIVATE
    bbtape
    gtest
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(bbtape_tests)

enable_testing()
add_executable(test_strategy tests_strategy.cpp)
target_link_libraries(test_strategy PRIVATE patterns)
add_test(NAME strategy_basic COMMAND test_strategy)

include(GoogleTest)

add_executable(unit_tests
	test_utils.cpp
)

target_link_libraries(unit_tests
	PRIVATE
	GTest::gtest_main
	warehouse_common
	m
)

gtest_discover_tests(unit_tests)

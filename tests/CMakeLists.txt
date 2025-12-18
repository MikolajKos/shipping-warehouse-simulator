include(GoogleTest)

add_executable(unit_tests test_utils.cpp)
add_executable(worker_express_tests test_worker_express.cpp)
add_executable(worker_std_tests test_worker_std.cpp)

target_link_libraries(worker_std_tests
	PRIVATE
	GTest::gtest_main
	warehouse_common
	pthread
)

target_link_libraries(unit_tests
	PRIVATE
	GTest::gtest_main
	warehouse_common
	m
)

target_link_libraries(worker_express_tests
	PRIVATE
	GTest::gtest_main
	warehouse_common
	pthread
)

gtest_discover_tests(
	unit_tests
	worker_express_tests
)

# Define common files
set(COMMON_SOURCES utils.c)

# Executables for each process
add_executable(warehouse_dispatcher main.c ${COMMON_SOURCES})
add_executable(worker_std worker_std.c ${COMMON_SOURCES})
add_executable(worker_express worker_express.c ${COMMON_SOURCES})
add_executable(truck truck.c ${COMMON_SOURCES})

# Linking libraries
target_link_libraries(warehouse_dispatcher m)
target_link_libraries(worker_std m)
target_link_libraries(worker_express m)
target_link_libraries(truck m)

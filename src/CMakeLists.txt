# --- Subdirectory with static library ---
add_subdirectory(common)

# --- Executables for each process ---
add_executable(warehouse_dispatcher main.c ${COMMON_SOURCES})
add_executable(worker_std worker_std.c ${COMMON_SOURCES})
add_executable(worker_express worker_express.c ${COMMON_SOURCES})
add_executable(truck truck.c ${COMMON_SOURCES})

# --- Linking libraries ---
foreach(TARGET warehouse_dispatcher worker_std worker_express truck)
	       target_link_libraries(${TARGET} warehouse_common m)
endforeach()

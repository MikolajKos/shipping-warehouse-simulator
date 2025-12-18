# --- Create static library with common tools ---
add_library(warehouse_common STATIC
			     utils.c
			     shm_wrapper.c
			     sem_wrapper.c
)

# --- Share current catalog (.) ---
target_include_directories(warehouse_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

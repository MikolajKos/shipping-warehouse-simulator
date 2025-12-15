cmake_minimum_required(VERSION 3.25)
project(ShippingWarehouse C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=700 -Wall -Wextra -pthread")

set(CMAKE_CXX_STANDARD 14) # Required by GTest

# --- Installing GoogleTest ---
include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
)

# --- Avoids installing GTest within system ---
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

# --- Doxygen configuration ---
find_package(Doxygen)

if (DOXYGEN_FOUND)
   # Set output / input
   set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
   set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

   # Get settings from Doxyfile.in
   configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

   # Set up "make doc" for docs build
   add_custom_target(doc
	COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen..."
        VERBATIM)

	message(STATUS "Doxygen found. Use 'make doc' to generate documentation.")
else()
	message(WARNING "Doxygen not found. Documentation target 'doc' will not be available.")
endif()

add_subdirectory(src)

# --- Tests ---
enable_testing()
add_subdirectory(tests)

cmake_minimum_required(VERSION 3.25)
project(ShippingWarehouse C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_XOPEN_SOURCE=700 -Wall -Wextra -pthread")

add_subdirectory(src)

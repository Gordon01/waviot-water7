cmake_minimum_required(VERSION 3.1)

project(water7_lib)

include_directories(lib)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

add_executable(main main.c lib/WVT_Water7.c)
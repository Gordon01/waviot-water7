cmake_minimum_required(VERSION 3.8)

project(water7_lib)
enable_testing()
include_directories(../lib)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -fstrict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdouble-promotion -Wswitch-enum -Wfloat-equal -Wundef")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wsign-promo")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-conversion -Wcast-align -Wmissing-declarations")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wtype-limits -Wzero-as-null-pointer-constant -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wsign-promo -Wold-style-cast")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -Wextra -Werror -pedantic")
#set_source_files_properties(../lib/WVT_Water7.c PROPERTIES LANGUAGE CXX)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -g -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -g -O0")
set(LCOV_REMOVE_EXTRA "'test/*'")

add_executable(tests main.cpp UT_Water7.cpp ../lib/WVT_Water7.c)

set_property(TARGET tests PROPERTY C_STANDARD 99)
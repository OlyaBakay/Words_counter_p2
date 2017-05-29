cmake_minimum_required(VERSION 3.6)
project(Words_counter_p2)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp measuring_time.h)
add_executable(Words_counter_p2 ${SOURCE_FILES})

find_package(Threads REQUIRED)
target_link_libraries(Words_counter_p2 Threads::Threads)

cmake_minimum_required(VERSION 2.8)

project(cxx-coroutines)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME} "main.cpp" "coroutine.cpp" "scheduler.cpp")


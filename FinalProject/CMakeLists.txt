cmake_minimum_required(VERSION 3.15)
project(FinalProject)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS -pthread)

add_executable(FinalProject main.cpp include/skippy.cpp include/skippy.h)
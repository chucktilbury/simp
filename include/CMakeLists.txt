cmake_minimum_required(VERSION 3.16)
project(simp_headers LANGUAGES CXX)

add_library(simp_headers INTERFACE)
target_include_directories(simp_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

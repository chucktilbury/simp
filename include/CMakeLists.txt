cmake_minimum_required(VERSION 3.16)
project(cwhip_headers LANGUAGES CXX)

add_library(cwhip_headers INTERFACE)
target_include_directories(cwhip_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

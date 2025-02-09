cmake_minimum_required(VERSION 3.10)
project(GameProject)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  message("CMAKE_BUILD_TYPE was empty... setting to debug")
  set(CMAKE_BUILD_TYPE Debug)
  message("game: CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
endif()

# Include the source subdirectories
add_subdirectory(src/dll_boundary)
add_subdirectory(src/engine)
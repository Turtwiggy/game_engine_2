cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_VERBOSE_MAKEFILE on)
if(EMSCRIPTEN)
  message("Compiling for emscripten")

else()
  message("Not compiling for emscripten - adding vcpkg ${CMAKE_SOURCE_DIR}")
  message("${CMAKE_SOURCE_DIR}")
  message("VCPKG TRIPLET: ${VCPKG_TARGET_TRIPLET}")
  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/thirdparty/vcpkg/scripts/buildsystems/vcpkg.cmake)
endif()

project(subprojects VERSION 0.1.0)

# projects
# add_subdirectory(thirdparty/box2d ${CMAKE_SOURCE_DIR}/build/box2d)
add_subdirectory(engine)
add_subdirectory(game)

if(EMSCRIPTEN)
else()
  enable_testing()

  # add_subdirectory(game_tests)
endif()

#
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
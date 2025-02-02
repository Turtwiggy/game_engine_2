cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-clang.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-emscripten.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-gcc.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-msvc.cmake)

project(game_tests VERSION 0.1.0)
message("game_tests...")

include(${CMAKE_SOURCE_DIR}/cmake/packages.cmake)

find_packages()
find_package(GTest CONFIG REQUIRED)

include(${CMAKE_SOURCE_DIR}/cmake/imgui.cmake)

file(GLOB_RECURSE TEST_SRC_FILES
  ${IMGUI_SOURCE}

  # do not inlude main()
  ${CMAKE_SOURCE_DIR}/game/src/modules/*.cpp
  ${CMAKE_SOURCE_DIR}/game/src/resources/*.cpp
  ${CMAKE_SOURCE_DIR}/game/src/game_state.cpp
  ${CMAKE_SOURCE_DIR}/game/src/game.cpp
  ${CMAKE_SOURCE_DIR}/game_tests/src/*.cpp
)

# Test executable
add_executable(game_tests ${TEST_SRC_FILES})

link_libs(game_tests)
target_link_libraries(game_tests PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)

target_include_directories(game_tests PRIVATE
  ${IMGUI_INCLUDES}
  ${VCPKG_INCLUDES}
  ${CMAKE_SOURCE_DIR}/game/src
  ${CMAKE_SOURCE_DIR}/game_tests/src
  ${CMAKE_SOURCE_DIR}/thirdparty/entt/single_include
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(VCPKG_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/installed/x64-windows/include)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(VCPKG_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/installed/arm64-osx/include)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(VCPKG_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/installed/x64-linux/include)
endif()

# Code coverage
if(CMAKE_CXX_COMPILER_ID MATCHES GNU AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("setting code coverage options...")
  target_compile_options(game_tests PRIVATE --coverage) # -O0 and -g should already be set
  target_link_options(game_tests PRIVATE --coverage)

  # find_program("" lcov)
  # C:/Software/msys64/ucrt64/bin/lcov

  # if(NOT LCOV_PATH)
  # message(FATAL_ERROR "lcov not found!")
  # else()
  # message("lcov found")
  # endif()

  # find_program(GENHTML_PATH genhtml)

  # if(NOT GENHTML_PATH)
  # message(FATAL_ERROR "genhtml not found!")
  # endif()

  # set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
  # generatelcovfile
  # add_custom_target(lcov_generate_coverage
  # COMMAND ${LCOV_PATH} --capture --directory .
  # COMMAND ${LCOV_PATH} --remove lcov.info '/usr/*' --output-file lcov.info
  # COMMAND ${LCOV_PATH} --list lcov.info
  # # COMMAND ${GENHTML_PATH} lcov.info --output-directory ${COVERAGE_OUTPUT_DIR}
  # COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated at ${COVERAGE_OUTPUT_DIR}/index.html"
  # WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  # COMMENT "Generating code coverage report..."
  # VERBATIM
  # )

  # # cleanup lcov file
  # add_custom_target(lcov_remove_coverage
  # COMMAND ${LCOV_PATH} --directory . --zerocounters
  # COMMAND ${CMAKE_COMMAND} -E remove lcov.info
  # COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_OUTPUT_DIR}
  # WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  # COMMENT "Cleaning up code coverage data..."
  # VERBATIM
  # )
endif()

message("creating symlink...")
set(src ${CMAKE_SOURCE_DIR}/game_defence/assets)
set(dst ${CMAKE_CURRENT_BINARY_DIR}/assets)
message("creating symlink src... ${src}")
message("creating symlink dst... ${dst}")
add_custom_command(
  TARGET ${PROJECT_NAME} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dst}
  DEPENDS ${dst}
  COMMENT "symbolic link resources folder from ${src} => ${dst}"
)

# If COMMAND specifies an executable target (created by add_executable)
# it will automatically be replaced by the location of the executable
# created at build time
message("adding test to ctest..")
add_test(NAME all_game_tests COMMAND game_tests)

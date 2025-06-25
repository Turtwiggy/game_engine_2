cmake_minimum_required(VERSION 3.30.0)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(game VERSION 0.1.0)
message("game: CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
message("game: CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
message("game: C: ${CMAKE_C_COMPILER_ID}")
message("game: CXX: ${CMAKE_CXX_COMPILER_ID}")
message("game: CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("game: CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message("game: CMAKE_LINKER: ${CMAKE_LINKER}")
message("game: current_list_dir: ${CMAKE_CURRENT_LIST_DIR}")
message("game: source_dir: ${CMAKE_SOURCE_DIR}")
message("game: CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

# include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-clang.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-emscripten.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-msvc.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-gcc.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/packages.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/build_misc.cmake)

# TODO: compile as one static .exe when HOT_RELOAD 0
set(HOT_RELOAD 1)

find_packages()
find_package(Tracy CONFIG REQUIRED)

FetchContent_Declare(
  tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy.git
  GIT_TAG v0.12.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(tracy)

# Add source files
include(${CMAKE_SOURCE_DIR}/cmake/imgui.cmake)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "common/src/*.cpp" "common/src/**/*.cpp" "engine/src/*.cpp" "engine/src/**/*.cpp"
  ${IMGUI_SOURCE}
  ${CMAKE_SOURCE_DIR}/thirdparty/enkits/TaskScheduler.cpp
  ${CMAKE_SOURCE_DIR}/common/src/*.cpp
  ${CMAKE_SOURCE_DIR}/engine/src/*.cpp
)

# Main executable
if("${CMAKE_BUILD_TYPE}" MATCHES "Release" AND WIN32)
  # "The WIN32 argument instructs the compiler to build a
  # Windows GUI app, which doesn't show a console."
  add_executable(game WIN32 ${SRC_FILES})
elseif(WIN32)
  add_executable(game ${SRC_FILES})
else()
  add_executable(game ${SRC_FILES})
endif()

# Precompile headers
set(PCH_FILE ${CMAKE_SOURCE_DIR}/common/src/core/pch.hpp)
target_precompile_headers(game PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${PCH_FILE}>")
set(PCH_INSTANTIATE_TEMPLATES ON)

link_libs(game)
target_link_libraries(game PUBLIC TracyClient)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  set(BOX2D_LIB ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/src/box2dd.lib)
  set(BOX2D_DLL ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/bin/box2dd.dll)
else()
  set(BOX2D_LIB ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/src/box2d.lib)
  set(BOX2D_DLL ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/bin/box2d.dll)
endif()

message("searching for box2d at... ${BOX2D_LIB}")
target_link_libraries(game PRIVATE ${BOX2D_LIB})

# Build GameDLL
# note: at some point, I think my plan was to create another cmake project.
# the problem is that GameDLL.dll is always linked because I link the .lib.
# if I create all the data in one dll, and both the game and the engine link against it
# I can avoid linking to GameDLL.lib but instead DataDll.lib
# I currently hot-reload the dll via GameDLL-hot-unlocked.DLL and GameDLL-hot-locked.DLL#
# However, I cannot unload GameDLL.dll because I link to the lib????
add_dependencies(game GameDLL)
set(LIB_SRC ${CMAKE_SOURCE_DIR}/build/game/GameDLL.lib)
message("searching for .dll at... ${LIB_SRC}")
target_link_libraries(game PRIVATE ${LIB_SRC})

target_include_directories(game PRIVATE
  ${IMGUI_INCLUDES}
  ${Stb_INCLUDE_DIR}
  ${VCPKG_INCLUDES}
  ${CMAKE_SOURCE_DIR}/thirdparty/box2d/include
  ${CMAKE_SOURCE_DIR}/thirdparty/enkits/src
  ${CMAKE_SOURCE_DIR}/thirdparty/entt/src
  ${CMAKE_SOURCE_DIR}/thirdparty/sdk/public
  ${CMAKE_SOURCE_DIR}/common/src
  ${CMAKE_SOURCE_DIR}/engine/src
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LD_FLAGS}")

# rename executable
if(EMSCRIPTEN)
  # rename as index.html
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "index")
else()
  # set(GAME_EXE_NAME "game")
  # set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${GAME_EXE_NAME})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
  message("vcpkg for windows")
  set(VCPKG_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/installed/x64-windows/include)
elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
  message("vcpkg for Darwin")
  set(VCPKG_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/installed/arm64-osx/include)
elseif(${CMAKE_SYSTEM_NAME} MATCHES Linux)
  message("vcpkg for Linux")
  set(VCPKG_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/thirdparty/vcpkg/installed/x64-linux/include)
endif()

# Create symlink for assets only in debug mode.
# This is due to the way I've implemented github
# actions for building release. Github actins will
# copy over the /assets folder in the release mode,
# and would fail if this symlink was created here.
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  create_symlinks(game)
ENDIF()

# Copy dll to GameDLL-hot-unlocked.dll
# Copy dll to GameDLL.dll
if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
  message("copying game.dll...")
  set(BUILT_DLL "${CMAKE_SOURCE_DIR}/build/game/GameDLL.dll")

  copy_file_next_to_exe(game "${BUILT_DLL}" "${CMAKE_CURRENT_BINARY_DIR}/GameDLL.dll")
  copy_file_next_to_exe(game "${BUILT_DLL}" "${CMAKE_CURRENT_BINARY_DIR}/GameDLL-hot-unlocked.dll")

  # box2d.dll => next to .exe
  if(${CMAKE_BUILD_TYPE} MATCHES Debug)
    copy_file_next_to_exe(game "${BOX2D_DLL}" "${CMAKE_CURRENT_BINARY_DIR}/box2dd.dll")
  else()
    copy_file_next_to_exe(game "${BOX2D_DLL}" "${CMAKE_CURRENT_BINARY_DIR}/box2d.dll")
  endif()
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Super cool things")

# set(CPACK_GENERATOR "ZIP") # Generate zip files
include(CPack)
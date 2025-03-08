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
find_packages()
find_package(Tracy CONFIG REQUIRED)

FetchContent_Declare(
  tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy.git
  GIT_TAG v0.11.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(tracy)

# Add source files
include(${CMAKE_SOURCE_DIR}/cmake/imgui.cmake)

# include(${CMAKE_SOURCE_DIR}/cmake/optick.cmake)
file(GLOB_RECURSE SRC_FILES
  ${IMGUI_SOURCE}
  ${CMAKE_SOURCE_DIR}/thirdparty/enkits/TaskScheduler.cpp
  ${CMAKE_SOURCE_DIR}/game/src/common/common.cpp
  ${CMAKE_SOURCE_DIR}/game/src/engine/*.cpp
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

link_libs(game)
target_link_libraries(game PUBLIC TracyClient)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
  set(BOX2D_LIBRARY ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/src/Debug/box2d.lib)
else()
  set(BOX2D_LIBRARY ${CMAKE_SOURCE_DIR}/build/box2d/src/box2d.lib)
endif()

message("searching for box2d at... ${BOX2D_LIBRARY}")
target_link_libraries(game PRIVATE ${BOX2D_LIBRARY})

# Build GameDLL
add_dependencies(game GameDLL)

# Copy the game.dll
if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
  message("copying game.dll needs copying...")
  add_custom_target(do_always ALL
    COMMAND ${CMAKE_COMMAND} -E remove
    "$<TARGET_FILE_DIR:game>/$<TARGET_FILE_NAME:GameDLL>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:GameDLL>"
    "$<TARGET_FILE_DIR:game>"
  )
endif()

# Link steam api
# if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
# # Link steam_api[64].lib
# set(steam_lib ${CMAKE_SOURCE_DIR}/thirdparty/sdk/redistributable_bin/win64)
# target_link_libraries(game PRIVATE ${steam_lib}/steam_api64.lib)
# # Copy the .dll next to the .exe
# if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/steam_api64.dll")
# message("Copying steam .dll next to .exe")
# set(dst ${CMAKE_CURRENT_BINARY_DIR})
# add_custom_command(TARGET game POST_BUILD
# COMMAND ${CMAKE_COMMAND} -E copy
# ${steam_lib}/steam_api64.dll
# $<TARGET_FILE_DIR:game>
# )
# endif()
# endif()
#
target_include_directories(game PRIVATE

  # ${CMAKE_SOURCE_DIR}/thirdparty/magic_enum
  ${IMGUI_INCLUDES}
  ${Stb_INCLUDE_DIR}
  ${VCPKG_INCLUDES}
  ${CMAKE_SOURCE_DIR}/thirdparty/box2d/include
  ${CMAKE_SOURCE_DIR}/thirdparty/enkits/src
  ${CMAKE_SOURCE_DIR}/thirdparty/entt/single_include
  ${CMAKE_SOURCE_DIR}/thirdparty/sdk/public
  ${CMAKE_SOURCE_DIR}/game/src/common
  ${CMAKE_SOURCE_DIR}/game/src/engine
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

# Create symlink for assets for emscripten
IF(EMSCRIPTEN)
  set(src ${CMAKE_SOURCE_DIR}/game/assets)
  set(dst ${CMAKE_CURRENT_BINARY_DIR}/../assets)
  message("creating symlink src... ${src}")
  message("creating symlink dst... ${dst}")
  add_custom_command(
    TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dst}
    DEPENDS ${dst}
    COMMENT "symbolic link resources folder from ${src} => ${dst}"
  )
ENDIF()

# Create symlink for assets only in debug mode.
# This is due to the way I've implemented github
# actions for building release. Github actins will
# copy over the /assets folder in the release mode,
# and would fail if this symlink was created here.
if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  message("creating symlink...")
  set(src ${CMAKE_SOURCE_DIR}/game/assets)
  set(dst ${CMAKE_CURRENT_BINARY_DIR}/assets)

  message("creating symlink src... ${src}")
  message("creating symlink dst... ${dst}")

  if(UNIX)
    message("creating symlink... (unix)")
    add_custom_command(
      TARGET ${PROJECT_NAME} PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dst}
      DEPENDS ${dst_sym}
      COMMENT "symbolic link resources folder from ${src} => ${dst}"
    )
  else()
    message("creating symlink... (windows)")
    file(TO_NATIVE_PATH "${src}" _srcDir)
    file(TO_NATIVE_PATH "${dst}" _dstDir)
    message("dst: ${_dstDir}")

    if(NOT EXISTS ${_dstDir})
      # https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/mklink
      # mklink [[/d] | [/h] | [/j]] <link> <target>
      # <link>	Specifies the name of the symbolic link being created.
      # <target>	Specifies the path (relative or absolute) that the new symbolic link refers to.
      #
      add_custom_command(
        TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND cmd.exe /c mklink /D "${_dstDir}" "${_srcDir}"
        COMMENT "Creating symlink on Windows"
      )
    ELSE()
      message("creating symlink... (already exists)")
    ENDIF()
  ENDIF()
ENDIF()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Super cool things")

# set(CPACK_GENERATOR "ZIP") # Generate zip files
include(CPack)
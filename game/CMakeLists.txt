cmake_minimum_required(VERSION 3.30.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(GameDLL VERSION 0.1.0)
message("GameDLL: CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")
message("GameDLL: CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
message("GameDLL: C: ${CMAKE_C_COMPILER_ID}")
message("GameDLL: CXX: ${CMAKE_CXX_COMPILER_ID}")
message("GameDLL: CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message("GameDLL: CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message("GameDLL: CMAKE_LINKER: ${CMAKE_LINKER}")
message("GameDLL: current_list_dir: ${CMAKE_CURRENT_LIST_DIR}")
message("GameDLL: source_dir: ${CMAKE_SOURCE_DIR}")
message("index CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")

# include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-clang.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-emscripten.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-msvc.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/use-compiler-gcc.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/packages.cmake)
find_packages()

# Add source files
include(${CMAKE_SOURCE_DIR}/cmake/imgui.cmake)

# Add source files.
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "common/src/*.cpp" "common/src/**/*.cpp" "game/src/*.cpp" "game/src/**/*.cpp"
  ${IMGUI_SOURCE}
  ${CMAKE_SOURCE_DIR}/thirdparty/enkits/TaskScheduler.cpp
  ${CMAKE_SOURCE_DIR}/common/src/*.cpp
  ${CMAKE_SOURCE_DIR}/game/src/*.cpp
)

# Instead of add_executable().
add_library(GameDLL SHARED
  ${SRC_FILES}
)

# Precompile headers
set(PCH_FILE ${CMAKE_SOURCE_DIR}/common/src/core/pch.hpp)
target_precompile_headers(game PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${PCH_FILE}>")
set(PCH_INSTANTIATE_TEMPLATES ON)

# Link Libraries
link_libs(GameDLL)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
  set(BOX2D_LIB ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/src/box2dd.lib)
  set(BOX2D_DLL ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/bin/box2dd.dll)
else()
  set(BOX2D_LIB ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/src/box2d.lib)
  set(BOX2D_DLL ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/bin/box2d.dll)
endif()

target_link_libraries(GameDLL PRIVATE ${BOX2D_LIB})

# Define the export macro for the .dll
target_compile_definitions(GameDLL PRIVATE DLL_EXPORTS)

target_include_directories(GameDLL PUBLIC
  ${IMGUI_INCLUDES}
  ${Stb_INCLUDE_DIR}
  ${VCPKG_INCLUDES}
  ${CMAKE_SOURCE_DIR}/thirdparty/box2d/include
  ${CMAKE_SOURCE_DIR}/thirdparty/enkits/src
  ${CMAKE_SOURCE_DIR}/thirdparty/entt/src
  ${CMAKE_SOURCE_DIR}/thirdparty/sdk/public
  ${CMAKE_SOURCE_DIR}/common/src
  ${CMAKE_SOURCE_DIR}/game/src
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LD_FLAGS}")

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

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION "Super cool things")
include(CPack)

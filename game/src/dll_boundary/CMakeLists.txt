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

# set(BOX2D_BUILD_DIR ${CMAKE_SOURCE_DIR}/thirdparty/box2d/build/src)
# set(BOX2D_LIBRARY ${BOX2D_BUILD_DIR}/libbox2d.a)

# Add source files.
file(GLOB_RECURSE SRC_FILES
  ${CMAKE_SOURCE_DIR}/game/src/dll_boundary/*.cpp
)

# Instead of add_executable().
add_library(GameDLL SHARED
  ${SRC_FILES}
)

# Link Libraries
link_libs(GameDLL)

# target_link_libraries(GameDLL PRIVATE ${BOX2D_LIBRARY})

# Define the export macro for the .dll
target_compile_definitions(GameDLL PRIVATE
  GAMEDLL_EXPORTS
)

target_include_directories(GameDLL PUBLIC
  ${VCPKG_INCLUDES}
  ${CMAKE_SOURCE_DIR}/thirdparty/box2d/include
  ${CMAKE_SOURCE_DIR}/thirdparty/entt/single_include
  ${CMAKE_SOURCE_DIR}/src/dll_boundary
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

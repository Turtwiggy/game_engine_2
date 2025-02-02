
function(find_packages)
  message("checking compiler installed...")
  find_program(CMAKE_C_COMPILER NAMES x86_64-w64-mingw32-gcc REQUIRED)
  find_program(CMAKE_CXX_COMPILER NAMES x86_64-w64-mingw32-g++ REQUIRED)
  find_program(CMAKE_RC_COMPILER NAMES x86_64-w64-mingw32-windres windres REQUIRED)

  message("finding packages...")

  find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)

  # find_package(SDL3 CONFIG REQUIRED)
  find_package(nlohmann_json CONFIG REQUIRED)
  find_package(Tracy CONFIG REQUIRED)

  # find_package(Vulkan REQUIRED)
  # find_package(VulkanMemoryAllocator CONFIG REQUIRED)
  # find_package(Stb REQUIRED)
endfunction()

function(link_libs project)
  target_link_libraries(${project} PRIVATE SDL3::SDL3)
  target_link_libraries(${project} PRIVATE Tracy::TracyClient)
  target_link_libraries(${project} PRIVATE nlohmann_json::nlohmann_json)

  # target_link_libraries(${project} PRIVATE Vulkan::Vulkan)
  # target_link_libraries(${project} PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator)
endfunction()

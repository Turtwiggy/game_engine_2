
function(find_packages)
  # Add VCPKG packages
  if(EMSCRIPTEN)
  else()
    # packages not supported or needed by emscripten
    # find_package(GameNetworkingSockets CONFIG REQUIRED)
    find_package(SDL3 CONFIG REQUIRED)

    # find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
  endif()

  find_package(nlohmann_json CONFIG REQUIRED)
  find_package(Tracy CONFIG REQUIRED)
  find_package(Vulkan REQUIRED)
  find_package(VulkanMemoryAllocator CONFIG REQUIRED)

  # find_package(Stb REQUIRED)
endfunction()

function(link_libs project)
  if(EMSCRIPTEN)
  # target_link_libraries(${project} PRIVATE openal)
  else()
    # target_link_libraries(game_defence_tests PRIVATE GameNetworkingSockets::shared)
    target_link_libraries(${project} PRIVATE SDL3::SDL3)
    target_link_libraries(${project} PRIVATE Tracy::TracyClient)
  endif()

  target_link_libraries(${project} PRIVATE nlohmann_json::nlohmann_json)
  target_link_libraries(${project} PRIVATE Vulkan::Vulkan)
  target_link_libraries(${project} PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator)
endfunction()

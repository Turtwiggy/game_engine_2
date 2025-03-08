if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
  message("Adjusting MSVC settings...")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /fp:precise")

  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /external:I ${CMAKE_SOURCE_DIR}/thirdparty")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4275 /wd4251 /bigobj")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /permissive-")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX-") # stop treating warnings as errors

  # set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /sdl /Oi /Ot /Oy /Ob2 /Zi")
endif()
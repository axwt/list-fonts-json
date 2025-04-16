cmake_minimum_required(VERSION 3.5)
project (list-fonts-json)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_HOST_WIN32)
  set(CMAKE_GENERATOR_PLATFORM "x64")
  # Set static linking of the runtime library
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_executable(list-fonts-json src/main.cc src/FontQuery.cc ${list-fonts-json_SOURCE_DIR}/src/FontManagerWindows.cc)
  target_link_libraries(list-fonts-json Dwrite)
	target_compile_definitions(list-fonts-json PRIVATE -D_CRT_SECURE_NO_WARNINGS)

elseif(CMAKE_HOST_APPLE)
  add_executable(list-fonts-json src/main.cc src/FontQuery.cc ${list-fonts-json_SOURCE_DIR}/src/FontManagerMac.mm)
  target_link_libraries(list-fonts-json
    "-framework CoreText"
    "-framework Foundation"
  )

elseif(CMAKE_HOST_UNIX)
  add_executable(list-fonts-json src/main.cc src/FontQuery.cc ${list-fonts-json_SOURCE_DIR}/src/FontManagerLinux.cc)

  find_package(Fontconfig REQUIRED)

  target_link_libraries(list-fonts-json fontconfig)
endif()

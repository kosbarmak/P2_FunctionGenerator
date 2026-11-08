cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# A copied template takes its executable name from its directory.
get_filename_component(FIRMWARE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
project(${FIRMWARE_NAME} LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_executable(${CMAKE_PROJECT_NAME})

# Each part of the firmware owns its source and include lists.
add_subdirectory(Platform)
add_subdirectory(Drivers)
add_subdirectory(Core)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${TOOLCHAIN_LINK_LIBRARIES})

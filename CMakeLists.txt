## Copyright 2024 Stefan Zellmann
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.17)

project(anariBRDFExplorer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Dependencies
find_package(anari 0.11.0 REQUIRED)

# TSD build directory (where you built VisRTX)
# This should be the CMake build directory, e.g., /path/to/VisRTX/build
set(TSD_BUILD_DIR "" CACHE PATH "Path to TSD CMake build directory")

if(NOT TSD_BUILD_DIR)
  message(FATAL_ERROR "TSD_BUILD_DIR must be specified (e.g., path to VisRTX/build)")
endif()

if(NOT EXISTS "${TSD_BUILD_DIR}")
  message(FATAL_ERROR "TSD_BUILD_DIR does not exist: ${TSD_BUILD_DIR}")
endif()

message(STATUS "Using TSD build from: ${TSD_BUILD_DIR}")

# Find SDL3 that TSD built
find_package(SDL3 QUIET HINTS ${TSD_BUILD_DIR})
if(NOT SDL3_FOUND)
  message(STATUS "SDL3 not found via find_package, will link directly")
endif()

# Main executable
add_executable(${PROJECT_NAME}
  brdfExplorer.cpp
  ParamEditor.cpp
  PluginLoader.cpp
  material.cpp
  Window.cpp
  Viewport.cpp
)

# Link against ANARI
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    anari::anari
)

# Link against TSD build artifacts
target_link_directories(${PROJECT_NAME}
  PRIVATE
    ${TSD_BUILD_DIR}
)

# Link against ImGui and SDL3 from TSD build
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    tsd_ext_imgui_sdl
    SDL3::SDL3
)

# Include TSD headers
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${TSD_BUILD_DIR}
)

# Plugin helper library
add_library(${PROJECT_NAME}_plugin_helper material.cpp)
target_include_directories(${PROJECT_NAME}_plugin_helper PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(${PROJECT_NAME}_plugin_helper anari::anari)

add_subdirectory(plugins)

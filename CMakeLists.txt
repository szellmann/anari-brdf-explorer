## Copyright 2024 Stefan Zellmann
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

project(anariBRDFExplorer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Dependencies
find_package(anari 0.11.0 REQUIRED)

# TSD provides SDL3 + ImGui
set(TSD_DIR "" CACHE PATH "Path to TSD (NVIDIA/VisRTX) installation")
if(TSD_DIR)
  list(APPEND CMAKE_PREFIX_PATH ${TSD_DIR}/lib/cmake)
  find_package(tsd_ext_imgui_sdl REQUIRED)
  find_package(SDL3 REQUIRED)
  message(STATUS "Using TSD UI library from: ${TSD_DIR}")
else()
  message(FATAL_ERROR "TSD_DIR must be specified (path to NVIDIA/VisRTX installation)")
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

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    anari::anari
    tsd_ext_imgui_sdl
    SDL3::SDL3
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${TSD_DIR}/include
)

# Plugin helper library
add_library(${PROJECT_NAME}_plugin_helper material.cpp)
target_include_directories(${PROJECT_NAME}_plugin_helper PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(${PROJECT_NAME}_plugin_helper anari::anari)

add_subdirectory(plugins)

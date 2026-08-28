## Copyright 2024 Stefan Zellmann
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.17)

project(anariBRDFExplorer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Dependencies
find_package(anari 0.11.0 REQUIRED)

# TSD provides SDL3 + ImGui - can be used in two ways:
# 1. As part of VisRTX build (via add_subdirectory)
# 2. As installed package (if TSD was built with CMAKE_INSTALL_PREFIX)
set(TSD_DIR "" CACHE PATH "Path to TSD (NVIDIA/VisRTX) source or install directory")

if(NOT TSD_DIR)
  message(FATAL_ERROR "TSD_DIR must be specified (path to NVIDIA/VisRTX source or installation)")
endif()

# Try to find TSD as installed package first
if(EXISTS "${TSD_DIR}/lib/cmake/tsd-config.cmake")
  message(STATUS "Found TSD installation at: ${TSD_DIR}")
  list(APPEND CMAKE_PREFIX_PATH ${TSD_DIR})
  find_package(tsd REQUIRED)
elseif(EXISTS "${TSD_DIR}/tsd/CMakeLists.txt")
  # Build TSD as subdirectory (requires ANARI to be available)
  message(STATUS "Building TSD from source at: ${TSD_DIR}/tsd")
  set(TSD_STANDALONE_BUILD 0)
  set(TSD_BUILD_APPS OFF)
  set(TSD_BUILD_INTERACTIVE_APPS OFF)
  set(TSD_BUILD_UI_LIBRARY ON)
  add_subdirectory(${TSD_DIR}/tsd ${CMAKE_BINARY_DIR}/tsd)
else()
  message(FATAL_ERROR "Could not find TSD at ${TSD_DIR}")
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

# Plugin helper library
add_library(${PROJECT_NAME}_plugin_helper material.cpp)
target_include_directories(${PROJECT_NAME}_plugin_helper PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(${PROJECT_NAME}_plugin_helper anari::anari)

add_subdirectory(plugins)

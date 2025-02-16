## Copyright 2024 Stefan Zellmann
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

project(anariBRDFExplorer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(anari 0.11.0 REQUIRED COMPONENTS viewer)

add_executable(${PROJECT_NAME} brdfExplorer.cpp ParamEditor.cpp PluginLoader.cpp material.cpp)
target_link_libraries(${PROJECT_NAME} anari::anari anari::anari_viewer)

add_library(${PROJECT_NAME}_plugin_helper material.cpp)
target_include_directories(${PROJECT_NAME}_plugin_helper PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(${PROJECT_NAME}_plugin_helper anari::anari)

add_subdirectory(plugins)

## Copyright 2024 Stefan Zellmann
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

project(anariBRDFExplorer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(anari 0.11.0 REQUIRED COMPONENTS viewer)

add_executable(${PROJECT_NAME} brdfExplorer.cpp ParamEditor.cpp)
target_link_libraries(${PROJECT_NAME} anari::anari anari::anari_viewer)

set(anari_visionaray_dir "/Users/stefan/anari-visionaray")
target_include_directories(${PROJECT_NAME} PUBLIC ${anari_visionaray_dir})

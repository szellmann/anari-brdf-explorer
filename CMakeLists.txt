## Copyright 2024 Stefan Zellmann
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)

project(anariBRDFExplorer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Dependencies
find_package(anari 0.11.0 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(imgui REQUIRED)
find_package(glm REQUIRED)

# Optional: TSD denoiser support
option(WITH_TSD "Enable TSD (Temporal Super-sampling Denoiser) support" ON)
if(WITH_TSD)
  # TSD would typically be found via find_package or manually specified
  # For now, we provide a cache variable for the TSD path
  set(TSD_DIR "" CACHE PATH "Path to TSD installation")
  if(TSD_DIR)
    message(STATUS "TSD support enabled at: ${TSD_DIR}")
  else()
    message(WARNING "TSD_DIR not specified. TSD denoiser will be disabled.")
    set(WITH_TSD OFF)
  endif()
endif()

# Main executable
add_executable(${PROJECT_NAME}
  brdfExplorer.cpp
  ParamEditor.cpp
  PluginLoader.cpp
  material.cpp
  Window.cpp
  Viewport.cpp
  TSDDenoiser.cpp
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    anari::anari
    OpenGL::OpenGL
    glfw
    imgui::imgui
    glm::glm
)

if(WITH_TSD)
  target_compile_definitions(${PROJECT_NAME} PRIVATE WITH_TSD=1)
  target_include_directories(${PROJECT_NAME} PRIVATE ${TSD_DIR}/include)
  target_link_directories(${PROJECT_NAME} PRIVATE ${TSD_DIR}/lib)
  # Link against TSD library (adjust name as needed)
  # target_link_libraries(${PROJECT_NAME} PRIVATE tsd)
endif()

# Plugin helper library
add_library(${PROJECT_NAME}_plugin_helper material.cpp)
target_include_directories(${PROJECT_NAME}_plugin_helper PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)
target_link_libraries(${PROJECT_NAME}_plugin_helper anari::anari)

add_subdirectory(plugins)

## Copyright 2024 Stefan Zellmann
## SPDX-License-Identifier: Apache-2.0

add_library(visionaray_material SHARED VisionarayMaterial.cpp)

set(anari_visionaray_dir "" CACHE FILEPATH "anari-visionaray base directory")
target_include_directories(visionaray_material PUBLIC ${anari_visionaray_dir})
target_link_libraries(visionaray_material ${PROJECT_NAME}_plugin_helper)

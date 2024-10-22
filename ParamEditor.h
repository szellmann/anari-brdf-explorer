// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#pragma once

// glad
#include <glad/glad.h>
// anari
#include "anari_viewer/windows/Window.h"
// std
#include <array>
#include <functional>
#include <string>
#include <vector>
// ours
#include "material.h"

namespace windows {

using ParamUpdateCallback = std::function<void()>;

class ParamEditor : public anari_viewer::windows::Window
{
 public:
  ParamEditor(explorer::Material &mat,
              anari::math::float3 &lightDir,
              std::string &selectedMaterial,
              const char *name = "Param Editor");
  ~ParamEditor();

  void setLightUpdateCallback(ParamUpdateCallback cb);
  void setMaterialUpdateCallback(ParamUpdateCallback cb);

  void buildUI() override;

 private:
  void drawEditor();

  ParamUpdateCallback m_lightUpdateCallback;
  ParamUpdateCallback m_materialUpdateCallback;

  explorer::Material &m_material;

  anari::math::float3 &m_lightDir;

  std::string &m_selectedMaterial;
};

} // namespace windows

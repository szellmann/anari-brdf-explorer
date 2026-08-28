// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Window.h"
#include <array>
#include <functional>
#include <string>
#include <vector>
#include <anari/anari_cpp.hpp>
#include "material.h"

using ParamUpdateCallback = std::function<void()>;

/**
 * Parameter editor window replacing anari_viewer::windows::Window
 * Provides ImGui UI for editing BRDF parameters and light settings
 */
class ParamEditor : public Window
{
public:
  ParamEditor(explorer::Material &mat,
              anari::math::float3 &lightDir,
              std::string &selectedMaterial,
              const char *name = "Param Editor");
  ~ParamEditor() override;

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

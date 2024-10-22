// Copyright 2022 Jefferson Amstutz
// SPDX-License-Identifier: Apache-2.0

#include "ParamEditor.h"

namespace windows {

ParamEditor::ParamEditor(explorer::Material &mat,
                         anari::math::float3 &lightDir,
                         std::string &selectedMaterial,
                         const char *name)
  : Window(name, true)
  , m_material(mat)
  , m_lightDir(lightDir)
  , m_selectedMaterial(selectedMaterial)
{
}

ParamEditor::~ParamEditor() {}

void ParamEditor::setMaterialUpdateCallback(ParamUpdateCallback cb)
{
  m_materialUpdateCallback = cb;
}

void ParamEditor::setLightUpdateCallback(ParamUpdateCallback cb)
{
  m_lightUpdateCallback = cb;
}

void ParamEditor::buildUI()
{
  drawEditor();
}

void ParamEditor::drawEditor()
{
  bool materialUpdated = false;
  bool lightUpdated = false;

  auto subtypes = explorer::Material::querySupportedSubtypes();

  const char *selected = m_selectedMaterial.c_str();
  if (ImGui::BeginCombo("Material Type", m_selectedMaterial.c_str())) {
    for (auto &st : subtypes) {
      if (ImGui::Selectable(st.c_str(), m_selectedMaterial==st))
        selected = st.c_str();
    }
    if (std::string(selected) != m_selectedMaterial) {
      m_selectedMaterial = selected;
      m_material.setSubtype(selected);
      materialUpdated = true;
    }
    ImGui::EndCombo();
  }

  auto params = explorer::Material::querySupportedParams(selected);

  for (auto &param : params) {
    auto actualParam = m_material.getParameter(param.name);
    if (actualParam.type == explorer::DataType::Float) {
      float value = std::any_cast<float>(actualParam.value);
      bool updated = ImGui::DragFloat(actualParam.name.c_str(), &value, value, 0.f, 1.f);
      if (updated) {
        auto newParam = actualParam;
        newParam.value = value;
        m_material.setParameter(newParam);
      }
      materialUpdated |= updated;
    }
  }

  if (ImGui::DragFloat3("Light dir", (float *)&m_lightDir[0])) {
    lightUpdated = true;
  }

  if (lightUpdated) {
    m_materialUpdateCallback();
    m_lightUpdateCallback();
  }

  if (materialUpdated && !lightUpdated) {
    m_materialUpdateCallback();
  }
}

} // namespace windows

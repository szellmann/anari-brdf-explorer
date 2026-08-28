// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "Viewport.h"
#include <imgui.h>
#include <SDL3/SDL.h>

Viewport::Viewport(anari::Device device, const std::string &name)
  : Window(name), m_device(device)
{
}

Viewport::~Viewport() = default;

void Viewport::setManipulator(anari_viewer::manipulators::Orbit *manipulator)
{
  m_manipulator = manipulator;
}

void Viewport::setWorld(anari::World world)
{
  m_world = world;
}

void Viewport::resetView()
{
  // TODO: Implement camera reset logic
}

void Viewport::render()
{
  if (!m_device || !m_world)
    return;

  // TODO: Implement ANARI rendering
}

void Viewport::buildUI()
{
  if (!m_visible)
    return;

  ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
  if (ImGui::Begin(m_name.c_str(), &m_visible)) {
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x > 0 && viewportSize.y > 0) {
      m_width = (int)viewportSize.x;
      m_height = (int)viewportSize.y;
      render();
      
      // Placeholder for rendered content
      ImGui::TextWrapped("Viewport: %dx%d", m_width, m_height);
    }
  }
  ImGui::End();
}

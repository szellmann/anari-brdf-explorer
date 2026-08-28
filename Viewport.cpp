// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "Viewport.h"
#include <imgui.h>
#include <GL/gl.h>

Viewport::Viewport(anari::Device device, const std::string &name)
  : Window(name), m_device(device)
{
  // Initialize framebuffer and textures for rendering
  glGenFramebuffers(1, &m_framebuffer);
  glGenTextures(1, &m_colorTexture);
  glGenTextures(1, &m_depthTexture);
}

Viewport::~Viewport()
{
  if (m_framebuffer)
    glDeleteFramebuffers(1, &m_framebuffer);
  if (m_colorTexture)
    glDeleteTextures(1, &m_colorTexture);
  if (m_depthTexture)
    glDeleteTextures(1, &m_depthTexture);
}

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

  // Render ANARI frame to texture
  // TODO: Implement ANARI rendering to texture
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
      
      // Display rendered texture
      ImGui::Image((void *)(intptr_t)m_colorTexture,
                   ImVec2(m_width, m_height),
                   ImVec2(0, 1), ImVec2(1, 0));
    }
  }
  ImGui::End();
}

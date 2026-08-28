// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Window.h"
#include <anari/anari_cpp.hpp>
#include <memory>

namespace anari_viewer {
  namespace manipulators {
    class Orbit;
  }
}

/**
 * Viewport window for rendering ANARI scenes
 * Replaces anari_viewer::windows::Viewport
 */
class Viewport : public Window
{
public:
  Viewport(anari::Device device, const std::string &name = "Viewport");
  ~Viewport() override;

  void setManipulator(anari_viewer::manipulators::Orbit *manipulator);
  void setWorld(anari::World world);
  void resetView();
  void render();
  void buildUI() override;

private:
  anari::Device m_device{nullptr};
  anari::World m_world{nullptr};
  anari_viewer::manipulators::Orbit *m_manipulator{nullptr};
  unsigned int m_framebuffer{0};
  unsigned int m_colorTexture{0};
  unsigned int m_depthTexture{0};
  int m_width{1280};
  int m_height{720};
};

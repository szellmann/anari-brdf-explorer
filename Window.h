// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <memory>

struct GLFWwindow;

/**
 * Base window class replacing anari_viewer::windows::Window
 * Provides ImGui-based UI rendering within a GLFW window
 */
class Window
{
public:
  Window(const std::string &name = "Window", bool showByDefault = true);
  virtual ~Window();

  virtual void buildUI() = 0;
  
  const std::string &name() const { return m_name; }
  bool isVisible() const { return m_visible; }
  void setVisible(bool visible) { m_visible = visible; }
  void toggleVisibility() { m_visible = !m_visible; }

protected:
  std::string m_name;
  bool m_visible;
};

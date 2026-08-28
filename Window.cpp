// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "Window.h"

Window::Window(const std::string &name, bool showByDefault)
  : m_name(name), m_visible(showByDefault)
{
}

Window::~Window() = default;

// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "TSDDenoiser.h"
#include <iostream>

TSDDenoiser::TSDDenoiser()
{
#ifdef WITH_TSD
  // TODO: Initialize TSD denoiser
  // m_denoiser = std::make_unique<tsd::Denoiser>(...);
  m_enabled = true;
#else
  std::cout << "TSD denoiser not available (compiled without WITH_TSD)" << std::endl;
#endif
}

TSDDenoiser::~TSDDenoiser() = default;

void TSDDenoiser::setEnabled(bool enabled)
{
#ifdef WITH_TSD
  m_enabled = enabled && m_denoiser != nullptr;
#else
  m_enabled = false;
#endif
}

void TSDDenoiser::denoise(const uint8_t *inputBuffer,
                          uint8_t *outputBuffer,
                          int width,
                          int height,
                          int bytesPerPixel)
{
if (!m_enabled || !inputBuffer || !outputBuffer)
    return;

#ifdef WITH_TSD
  // TODO: Call TSD denoiser with uint8_t data
  // m_denoiser->denoise(inputBuffer, outputBuffer, width, height, bytesPerPixel);
#endif
}

void TSDDenoiser::denoise(const float *inputBuffer,
                          float *outputBuffer,
                          int width,
                          int height,
                          int bytesPerPixel)
{
  if (!m_enabled || !inputBuffer || !outputBuffer)
    return;

#ifdef WITH_TSD
  // TODO: Call TSD denoiser with float data
  // m_denoiser->denoise(inputBuffer, outputBuffer, width, height, bytesPerPixel);
#endif
}

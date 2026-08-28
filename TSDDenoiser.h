// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <string>

#ifdef WITH_TSD
// Forward declare TSD types
namespace tsd {
  class Denoiser;
}
#endif

/**
 * Wrapper for TSD (Temporal Super-sampling Denoiser) from NVIDIA/VisRTX
 */
class TSDDenoiser
{
public:
  TSDDenoiser();
  ~TSDDenoiser();

  bool isEnabled() const { return m_enabled; }
  void setEnabled(bool enabled);

  /**
   * Denoise a frame
   * @param inputBuffer Input frame data (RGBA, uint8_t or float)
   * @param outputBuffer Output denoised frame
   * @param width Frame width
   * @param height Frame height
   * @param bytesPerPixel Bytes per pixel (3 or 4 for RGB/RGBA)
   */
  void denoise(const uint8_t *inputBuffer,
               uint8_t *outputBuffer,
               int width,
               int height,
               int bytesPerPixel = 4);

  void denoise(const float *inputBuffer,
               float *outputBuffer,
               int width,
               int height,
               int bytesPerPixel = 4);

private:
  bool m_enabled{false};
#ifdef WITH_TSD
  std::unique_ptr<tsd::Denoiser> m_denoiser;
#endif
};

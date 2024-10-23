
#pragma once

// ours
#include "material.h"
// anari-visionaray
#include "renderer/common.h"

namespace explorer {

struct VisionarayMaterial : public Material
{
  visionaray::dco::Material mat;

  VisionarayMaterial(std::string_view subtype);

  anari::math::float3 eval(anari::math::float3 Ng,
                           anari::math::float3 Ns,
                           anari::math::float3 viewDir,
                           anari::math::float3 lightDir,
                           anari::math::float3 lightIntensity) const override;

  void setSubtype(std::string_view subtype) override;
  void setParameter(MaterialParam param) override;
  MaterialParam getParameter(std::string_view name) const override;

  static std::vector<std::string> querySupportedSubtypes();

  static std::vector<MaterialParam> querySupportedParams(std::string_view subtype);
};

} // namespace explorer

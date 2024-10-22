
#pragma once

#define ANARI_EXTENSION_UTILITY_IMPL
#include <anari/anari_cpp.hpp> // anari::math

#include "renderer/common.h"

namespace explorer {

struct Material
{
  visionaray::dco::Material mat;
  visionaray::dco::Sampler *samplers{nullptr};
  visionaray::float4 *attribs{nullptr};
  int primID{0};

  anari::math::float3 eval(anari::math::float3 Ng,
                           anari::math::float3 Ns,
                           anari::math::float3 viewDir,
                           anari::math::float3 lightDir,
                           anari::math::float3 lightIntensity)
  {
    visionaray::vec3 visionarayNg = cast(Ng);
    visionaray::vec3 visionarayNs = cast(Ns);
    visionaray::vec3 visionarayViewDir = cast(viewDir);
    visionaray::vec3 visionarayLightDir = cast(lightDir);
    visionaray::vec3 visionarayLightIntensity = cast(lightIntensity);
    return cast(evalMaterial(mat,samplers,attribs,primID,
                visionarayNg,
                visionarayNs,
                normalize(visionarayViewDir),
                visionarayLightDir,
                visionarayLightIntensity));
  }

  visionaray::float3 cast(anari::math::float3 v)
  {
    return visionaray::float3(v.x, v.y, v.z);
  }

  anari::math::float3 cast(visionaray::float3 v)
  {
    return anari::math::float3(v.x, v.y, v.z);
  }
};

static Material generateMaterial(std::string_view subtype = "Matte")
{
  using namespace visionaray;

  dco::Material::Type type;
  if (subtype == "Matte")
    type = dco::Material::Matte;
  else if (subtype == "PBM")
    type = dco::Material::PhysicallyBased;

  Material result;
  result.mat.type = type;

  if (type == dco::Material::Matte) {
    result.mat = dco::makeDefaultMaterial();
    result.mat.asMatte.color.rgb = {1.f,1.f,1.f};
  }
  else if (type == dco::Material::PhysicallyBased) {
    //mat.asPhysicallyBased.baseColor.rgb = {1.f,1.f,1.f};
    //mat.asPhysicallyBased.baseColor.samplerID = UINT_MAX;
    //mat.asPhysicallyBased.baseColor.attribute = dco::Attribute::None;

    //mat.asPhysicallyBased.opacity.f = 1.f;
    //mat.asPhysicallyBased.opacity.samplerID = UINT_MAX;
    //mat.asPhysicallyBased.opacity.attribute = dco::Attribute::None;

    //mat.asPhysicallyBased.metallic.f = g_metallic;
    //mat.asPhysicallyBased.metallic.samplerID = UINT_MAX;
    //mat.asPhysicallyBased.metallic.attribute = dco::Attribute::None;

    //mat.asPhysicallyBased.roughness.f = g_roughness;
    //mat.asPhysicallyBased.roughness.samplerID = UINT_MAX;
    //mat.asPhysicallyBased.roughness.attribute = dco::Attribute::None;

    //mat.asPhysicallyBased.normal.samplerID = UINT_MAX;

    //mat.asPhysicallyBased.alphaMode = dco::AlphaMode::Opaque;
    //mat.asPhysicallyBased.alphaCutoff = 0.f;

    //mat.asPhysicallyBased.clearcoat.f = g_clearcoat;
    //mat.asPhysicallyBased.clearcoat.samplerID = UINT_MAX;
    //mat.asPhysicallyBased.clearcoat.attribute = dco::Attribute::None;

    //mat.asPhysicallyBased.clearcoatRoughness.f = g_clearcoatRoughness;
    //mat.asPhysicallyBased.clearcoatRoughness.samplerID = UINT_MAX;
    //mat.asPhysicallyBased.clearcoatRoughness.attribute = dco::Attribute::None;

    //mat.asPhysicallyBased.ior = g_ior;
  }

  return result;
}

} // namespace explorer

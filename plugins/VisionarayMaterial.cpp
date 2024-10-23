// ours
#include "VisionarayMaterial.h"

namespace explorer {

inline visionaray::float3 cast(anari::math::float3 v)
{
  return visionaray::float3(v.x, v.y, v.z);
}


inline anari::math::float3 cast(visionaray::float3 v)
{
  return anari::math::float3(v.x, v.y, v.z);
}

VisionarayMaterial::VisionarayMaterial(std::string_view subtype)
{
  setSubtype(subtype);
}

anari::math::float3 VisionarayMaterial::eval(anari::math::float3 Ng,
                                             anari::math::float3 Ns,
                                             anari::math::float3 viewDir,
                                             anari::math::float3 lightDir,
                                             anari::math::float3 lightIntensity) const
{
  visionaray::dco::Sampler *samplers{nullptr};
  visionaray::float4 *attribs{nullptr};
  int primID{0};

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

void VisionarayMaterial::setSubtype(std::string_view subtype)
{
  using namespace visionaray;

  if (subtype == "Matte")
    mat.type = dco::Material::Matte;
  else if (subtype == "PBM")
    mat.type = dco::Material::PhysicallyBased;

  // Set defaults:
  if (mat.type == dco::Material::Matte) {
    mat = dco::makeDefaultMaterial();
    mat.asMatte.color.rgb = {1.f,1.f,1.f};
  }
  else if (mat.type == dco::Material::PhysicallyBased) {
    mat.asPhysicallyBased.baseColor.rgb = {1.f,1.f,1.f};
    mat.asPhysicallyBased.baseColor.samplerID = UINT_MAX;
    mat.asPhysicallyBased.baseColor.attribute = dco::Attribute::None;

    mat.asPhysicallyBased.opacity.f = 1.f;
    mat.asPhysicallyBased.opacity.samplerID = UINT_MAX;
    mat.asPhysicallyBased.opacity.attribute = dco::Attribute::None;

    mat.asPhysicallyBased.metallic.f = 0.f;
    mat.asPhysicallyBased.metallic.samplerID = UINT_MAX;
    mat.asPhysicallyBased.metallic.attribute = dco::Attribute::None;

    mat.asPhysicallyBased.roughness.f = 0.f;
    mat.asPhysicallyBased.roughness.samplerID = UINT_MAX;
    mat.asPhysicallyBased.roughness.attribute = dco::Attribute::None;

    mat.asPhysicallyBased.normal.samplerID = UINT_MAX;

    mat.asPhysicallyBased.alphaMode = dco::AlphaMode::Opaque;
    mat.asPhysicallyBased.alphaCutoff = 0.f;

    mat.asPhysicallyBased.clearcoat.f = 0.f;
    mat.asPhysicallyBased.clearcoat.samplerID = UINT_MAX;
    mat.asPhysicallyBased.clearcoat.attribute = dco::Attribute::None;

    mat.asPhysicallyBased.clearcoatRoughness.f = 0.f;
    mat.asPhysicallyBased.clearcoatRoughness.samplerID = UINT_MAX;
    mat.asPhysicallyBased.clearcoatRoughness.attribute = dco::Attribute::None;

    mat.asPhysicallyBased.ior = 1.f;
  }
}

void VisionarayMaterial::setParameter(MaterialParam param)
{
  if (param.name == "color") {
    mat.asMatte.color.rgb = cast(std::any_cast<anari::math::float3>(param.value));
  }
  else if (param.name == "baseColor") {
    mat.asPhysicallyBased.baseColor.rgb = cast(std::any_cast<anari::math::float3>(param.value));
  }
  else if (param.name == "opacity") {
    mat.asPhysicallyBased.opacity.f = std::any_cast<float>(param.value);
  }
  else if (param.name == "metallic") {
    mat.asPhysicallyBased.metallic.f = std::any_cast<float>(param.value);
  }
  else if (param.name == "roughness") {
    mat.asPhysicallyBased.roughness.f = std::any_cast<float>(param.value);
  }
  else if (param.name == "clearcoat") {
    mat.asPhysicallyBased.clearcoat.f = std::any_cast<float>(param.value);
  }
  else if (param.name == "clearcoatRoughness") {
    mat.asPhysicallyBased.clearcoatRoughness.f = std::any_cast<float>(param.value);
  }
  else if (param.name == "ior") {
    mat.asPhysicallyBased.ior = std::any_cast<float>(param.value);
  }
}

MaterialParam VisionarayMaterial::getParameter(std::string_view name) const
{
  if (name == "color") {
    return { "color", std::any(cast(mat.asMatte.color.rgb)), DataType::Float3 };
  }
  else if (name == "baseColor") {
    return { "baseColor", std::any(cast(mat.asPhysicallyBased.baseColor.rgb)), DataType::Float3 };
  }
  else if (name == "opacity") {
    return { "opacity", std::any(mat.asPhysicallyBased.opacity.f), DataType::Float };
  }
  else if (name == "metallic") {
    return { "metallic", std::any(mat.asPhysicallyBased.metallic.f), DataType::Float };
  }
  else if (name == "roughness") {
    return { "roughness", std::any(mat.asPhysicallyBased.roughness.f), DataType::Float };
  }
  else if (name == "clearcoat") {
    return { "clearcoat", std::any(mat.asPhysicallyBased.clearcoat.f), DataType::Float };
  }
  else if (name == "clearcoatRoughness") {
    return { "clearcoatRoughness", std::any(mat.asPhysicallyBased.clearcoatRoughness.f), DataType::Float };
  }
  else if (name == "ior") {
    return { "ior", std::any(mat.asPhysicallyBased.ior), DataType::Float };
  }

  return {};
}

std::vector<std::string> VisionarayMaterial::querySupportedSubtypes()
{
  std::vector<std::string> result(2);
  result[0] = "Matte";
  result[1] = "PBM";
  return result;
}

std::vector<MaterialParam> VisionarayMaterial::querySupportedParams(std::string_view subtype)
{
  using namespace anari::math;

  std::vector<MaterialParam> result;

  if (subtype == "Matte") {
    result.push_back({"color", std::any(float3(1.f, 1.f, 1.f)), DataType::Float3});
  }
  else if (subtype == "PBM") {
    result.push_back({"baseColor", std::any(float3(1.f, 1.f, 1.f)), DataType::Float3});
    result.push_back({"opacity", std::any(1.f), DataType::Float});
    result.push_back({"metallic", std::any(0.f), DataType::Float});
    result.push_back({"roughness", std::any(0.f), DataType::Float});
    result.push_back({"clearcoat", std::any(0.f), DataType::Float});
    result.push_back({"clearcoatRoughness", std::any(0.f), DataType::Float});
    result.push_back({"ior", std::any(1.f), DataType::Float});
  }

  return result;
}

Material *createMaterialInstance(std::string_view subtype)
{
  return new VisionarayMaterial(subtype);
}

std::vector<std::string> querySupportedSubtypes()
{
  return VisionarayMaterial::querySupportedSubtypes();
}

std::vector<MaterialParam> querySupportedParams(std::string_view subtypes)
{
  return VisionarayMaterial::querySupportedParams(subtypes);
}

} // namespace explorer

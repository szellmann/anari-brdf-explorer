
#include "material.h"
#include "VisionarayMaterial.h"

namespace explorer {

Material *Material::createInstance(std::string_view subtype)
{
  return new VisionarayMaterial(subtype);
}

std::vector<std::string> Material::querySupportedSubtypes()
{
  return VisionarayMaterial::querySupportedSubtypes();
}

std::vector<MaterialParam> Material::querySupportedParams(std::string_view subtype)
{
  return VisionarayMaterial::querySupportedParams(subtype);
}

} // namespace explorer

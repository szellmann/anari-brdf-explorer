
#pragma once

// std
#include <any>
#include <string>
// anari
#include <anari/anari_cpp/ext/linalg.h>

namespace explorer {

enum class DataType
{
  Float, Float2, Float3, Float4, // TODO: add to this list as the need arises!
};

struct MaterialParam
{
  std::string name;
  std::any    value;
  DataType    type;
};

struct Material
{
  virtual anari::math::float3 eval(anari::math::float3 Ng,
                                   anari::math::float3 Ns,
                                   anari::math::float3 viewDir,
                                   anari::math::float3 lightDir,
                                   anari::math::float3 lightIntensity) const = 0;

  virtual void setSubtype(std::string_view subtype) = 0;
  virtual void setParameter(MaterialParam param) = 0;
  virtual MaterialParam getParameter(std::string_view name) const = 0;

  static Material *createInstance(std::string_view subtype);

  static std::vector<std::string> querySupportedSubtypes();

  static std::vector<MaterialParam> querySupportedParams(std::string_view subtype);
};

} // namespace explorer

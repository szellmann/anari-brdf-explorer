
#include "material.h"

namespace explorer {

Plugin Material::g_materialPlugin = nullptr;

void Material::loadPlugin(std::string name)
{
  g_materialPlugin = explorer::loadPlugin(name);
}

bool Material::pluginLoaded()
{
  return g_materialPlugin != nullptr;
}

Material *Material::createInstance(std::string_view subtype)
{
  if (!g_materialPlugin)
    return nullptr;

  auto createMaterialInstanceSym = getSymbolAddress(g_materialPlugin, "createMaterialInstance");
  if (!createMaterialInstanceSym)
    return nullptr;

  auto createMaterialInstance = (Material *(*)(std::string_view))createMaterialInstanceSym;
  return createMaterialInstance(subtype);
}

std::vector<std::string> Material::querySupportedSubtypes()
{
  if (!g_materialPlugin)
    return {};

  auto querySupportedSubtypesSym = getSymbolAddress(g_materialPlugin, "querySupportedSubtypes");
  if (!querySupportedSubtypesSym)
    return {};

  auto querySupportedSubtypes = (std::vector<std::string> (*)())querySupportedSubtypesSym;
  return querySupportedSubtypes();
}

std::vector<MaterialParam> Material::querySupportedParams(std::string_view subtype)
{
  if (!g_materialPlugin)
    return {};

  auto querySupportedParamsSym = getSymbolAddress(g_materialPlugin, "querySupportedParams");
  if (!querySupportedParamsSym)
    return {};

  auto querySupportedParams = (std::vector<MaterialParam> (*)(std::string_view))querySupportedParamsSym;
  return querySupportedParams(subtype);
}

} // namespace explorer

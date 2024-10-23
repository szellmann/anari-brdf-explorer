#pragma once

// std
#include <string>
// ours
#include "material.h"

namespace explorer {

void *loadMaterialPlugin(std::string libName);

void freePlugin(void *lib);

} // namespace explorer

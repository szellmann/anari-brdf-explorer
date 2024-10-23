#pragma once

// std
#include <string>

namespace explorer {

typedef void *Plugin;

Plugin loadPlugin(std::string libName);

void freePlugin(void *lib);

void *getSymbolAddress(void *lib, const std::string &symbol);

} // namespace explorer

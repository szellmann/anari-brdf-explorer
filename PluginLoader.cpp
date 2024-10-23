#include "PluginLoader.h"

// std
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/times.h>
#endif

#include <stdexcept>
#include <vector>

#if defined(__MACOSX__) || defined(__APPLE__)
#define LIB_EXT ".dylib"
#else
#define LIB_EXT ".so"
#endif

#ifdef _WIN32
#define LOOKUP_SYM(lib, symbol) (void*)GetProcAddress((HMODULE)lib, symbol.c_str());
#define FREE_LIB(lib) FreeLibrary((HMODULE)lib);
#else
#define LOOKUP_SYM(lib, symbol) dlsym(lib, symbol.c_str());
#define FREE_LIB(lib) dlclose(lib)
#endif

namespace {

std::string library_location()
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  MEMORY_BASIC_INFORMATION mbi;
  VirtualQuery((LPCVOID)&_anari_anchor, &mbi, sizeof(mbi));
  char pathBuf[16384];
  if (!GetModuleFileNameA(
          static_cast<HMODULE>(mbi.AllocationBase), pathBuf, sizeof(pathBuf)))
    return std::string();

  std::string path = std::string(pathBuf);
  path.resize(path.rfind('\\') + 1);
#else
  const char *anchor = "_anari_anchor";
  void *handle = dlsym(RTLD_DEFAULT, anchor);
  if (!handle)
    return std::string();

  Dl_info di;
  int ret = dladdr(handle, &di);
  if (!ret || !di.dli_saddr || !di.dli_fname)
    return std::string();

  std::string path = std::string(di.dli_fname);
  path.resize(path.rfind('/') + 1);
#endif

  return path;
}

} // namespace

static void *loadLibrary(
    const std::string &libName, bool withAnchor, std::string &errorMessage)
{
  std::string file = libName;
  std::string errorMsg;
  std::string libLocation = withAnchor ? library_location() : std::string();
  void *lib = nullptr;
#ifdef _WIN32
  // Set cwd to library location, to make sure dependent libraries are found as
  // well
  constexpr int MAX_DIRSIZE = 4096;
  TCHAR currentWd[MAX_DIRSIZE];
  DWORD dwRet = 0;
  if (withAnchor)
    dwRet = GetCurrentDirectory(MAX_DIRSIZE, currentWd);

  if (dwRet > MAX_DIRSIZE)
    errorMsg = "library path larger than " + std::to_string(MAX_DIRSIZE)
        + " characters";
  else if (withAnchor && dwRet == 0)
    errorMsg = "GetCurrentDirectory() failed for unknown reason";
  else {
    SetCurrentDirectory(libLocation.c_str());

    // Load the library
    std::string fullName = libLocation + file + ".dll";
    lib = LoadLibrary(fullName.c_str());
    if (lib == nullptr) {
      DWORD err = GetLastError();
      LPTSTR lpMsgBuf;
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
              | FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          err,
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR)&lpMsgBuf,
          0,
          NULL);

      errorMsg = lpMsgBuf;

      LocalFree(lpMsgBuf);
    }

    // Change cwd back to its original value
    SetCurrentDirectory(currentWd);
  }

#else
  std::string fullName = libLocation + "lib" + file + LIB_EXT;
  lib = dlopen(fullName.c_str(), RTLD_LAZY | RTLD_LOCAL);
  if (lib == nullptr) {
    errorMsg += dlerror();
  }
#endif

  if (lib == nullptr) {
    errorMessage += " could not open library lib " + libName + ": " + errorMsg;
  }

  return lib;
}

static void *getSymbolAddress(void *lib, const std::string &symbol)
{
  return LOOKUP_SYM(lib, symbol);
}

void *loadMaterialPlugin(std::string libName)
{
  std::string errorMessage;

  void *lib = loadLibrary(libName, false, errorMessage);
  if (!lib) {
    errorMessage = "(unanchored library load attempt failed)\n";
    lib = loadLibrary(libName, true, errorMessage);
  }

  if (!lib)
    throw std::runtime_error(errorMessage);

  return lib;
}

static void freePlugin(void *lib)
{
  if (lib)
    FREE_LIB(lib);
}

namespace explorer {

} // namespace explorer
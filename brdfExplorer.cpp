// Copyright 2024 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

// GLFW/OpenGL
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// ANARI
#define ANARI_EXTENSION_UTILITY_IMPL
#include <anari/anari_cpp.hpp>

// Standard
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Ours
#include "material.h"
#include "ParamEditor.h"
#include "Viewport.h"
#include "TSDDenoiser.h"

using box3_t = std::array<anari::math::float3, 2>;
namespace anari {
  ANARI_TYPEFOR_SPECIALIZATION(box3_t, ANARI_FLOAT32_BOX3);
  ANARI_TYPEFOR_DEFINITION(box3_t);
}

using namespace anari::math;

// Global state
static bool g_verbose = false;
static bool g_enableDebug = false;
static std::string g_libraryName = "environment";
static anari::Library g_debug = nullptr;
static anari::Device g_device = nullptr;
static const char *g_traceDir = nullptr;

// Scene parameters
static float g_groundPlaneOpacity = 0.5f;
static std::string g_selectedMaterial = "Matte";
static float3 g_lightDir = {1.f, 1.f, 0.f};
static bool g_showGroundPlane = true;
static bool g_showLightDir = true;
static bool g_showAxes = true;
static box3_t g_bounds = {
    anari::math::float3{-3.f, 0.f, -3.f},
    anari::math::float3{3.f, 1.f, 3.f}};

// Denoiser
static TSDDenoiser g_denoiser;
static bool g_enableDenoiser = false;

// ============================================================================
// ANARI Setup
// ============================================================================

void statusFunc(const void *userData,
                ANARIDevice device,
                ANARIObject source,
                ANARIDataType sourceType,
                ANARIStatusSeverity severity,
                ANARIStatusCode code,
                const char *message)
{
  const bool verbose = userData ? *(const bool *)userData : false;
  if (severity == ANARI_SEVERITY_FATAL_ERROR) {
    fprintf(stderr, "[FATAL][%p] %s\n", source, message);
    std::exit(1);
  } else if (severity == ANARI_SEVERITY_ERROR)
    fprintf(stderr, "[ERROR][%p] %s\n", source, message);
  else if (severity == ANARI_SEVERITY_WARNING)
    fprintf(stderr, "[WARN ][%p] %s\n", source, message);
  else if (verbose && severity == ANARI_SEVERITY_PERFORMANCE_WARNING)
    fprintf(stderr, "[PERF ][%p] %s\n", source, message);
  else if (verbose && severity == ANARI_SEVERITY_INFO)
    fprintf(stderr, "[INFO ][%p] %s\n", source, message);
  else if (verbose && severity == ANARI_SEVERITY_DEBUG)
    fprintf(stderr, "[DEBUG][%p] %s\n", source, message);
}

static void initializeANARI()
{
  auto library =
      anariLoadLibrary(g_libraryName.c_str(), statusFunc, &g_verbose);
  if (!library)
    throw std::runtime_error("Failed to load ANARI library");

  if (g_enableDebug)
    g_debug = anariLoadLibrary("debug", statusFunc, nullptr);

  anari::Device dev = anariNewDevice(library, "default");
  anari::unloadLibrary(library);

  if (g_enableDebug) {
    anari::setParameter(dev, dev, "glDebug", true);
    anari::Device dbg = anariNewDevice(g_debug, "debug");
    anari::setParameter(dbg, dbg, "wrappedDevice", dev);
    if (g_traceDir) {
      anari::setParameter(dbg, dbg, "traceDir", g_traceDir);
      anari::setParameter(dbg, dbg, "traceMode", "code");
    }
    anari::commitParameters(dbg, dbg);
    anari::release(dev, dev);
    dev = dbg;
  }

  anari::setParameter(dev, dev, "glAPI", "OpenGL");
  anari::commitParameters(dev, dev);

  g_device = dev;
}

// ============================================================================
// Geometry Creation
// ============================================================================

static anari::Array2D makeTextureData(anari::Device d, int dim)
{
  using texel = std::array<uint8_t, 3>;
  texel *data = (texel *)std::malloc(dim * dim * sizeof(texel));

  auto makeTexel = [](uint8_t v) -> texel { return {v, v, v}; };

  for (int h = 0; h < dim; h++) {
    for (int w = 0; w < dim; w++) {
      bool even = h & 1;
      if (even)
        data[h * dim + w] = w & 1 ? makeTexel(255) : makeTexel(0);
      else
        data[h * dim + w] = w & 1 ? makeTexel(0) : makeTexel(255);
    }
  }

  return anariNewArray2D(
      d, data, [](const void *, const void *ptr) {
        std::free(const_cast<void *>(ptr));
      },
      nullptr, ANARI_UFIXED8_VEC3, dim, dim);
}

static anari::Surface makePlane(anari::Device d, box3_t bounds)
{
  anari::math::float3 vertices[4];
  vertices[0] = {bounds[0][0], bounds[0][1], bounds[1][2]};
  vertices[1] = {bounds[1][0], bounds[0][1], bounds[1][2]};
  vertices[2] = {bounds[1][0], bounds[0][1], bounds[0][2]};
  vertices[3] = {bounds[0][0], bounds[0][1], bounds[0][2]};

  anari::math::float2 texcoords[4] = {
      {0.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f}};

  auto geom = anari::newObject<anari::Geometry>(d, "quad");
  anari::setAndReleaseParameter(d, geom, "vertex.position",
                                anari::newArray1D(d, vertices, 4));
  anari::setAndReleaseParameter(d, geom, "vertex.attribute0",
                                anari::newArray1D(d, texcoords, 4));
  anari::commitParameters(d, geom);

  auto surface = anari::newObject<anari::Surface>(d);
  anari::setAndReleaseParameter(d, surface, "geometry", geom);

  auto tex = anari::newObject<anari::Sampler>(d, "image2D");
  anari::setAndReleaseParameter(d, tex, "image", makeTextureData(d, 8));
  anari::setParameter(d, tex, "inAttribute", "attribute0");
  anari::setParameter(d, tex, "wrapMode1", "clampToEdge");
  anari::setParameter(d, tex, "wrapMode2", "clampToEdge");
  anari::setParameter(d, tex, "filter", "nearest");
  anari::commitParameters(d, tex);

  auto mat = anari::newObject<anari::Material>(d, "matte");
  anari::setAndReleaseParameter(d, mat, "color", tex);
  anari::setParameter(d, mat, "alphaMode", "blend");
  anari::setParameter(d, mat, "opacity", g_groundPlaneOpacity);
  anari::commitParameters(d, mat);
  anari::setAndReleaseParameter(d, surface, "material", mat);

  anari::commitParameters(d, surface);

  return surface;
}

static anari::Instance makePlaneInstance(anari::Device d, const box3_t &bounds)
{
  auto surface = makePlane(d, bounds);

  auto group = anari::newObject<anari::Group>(d);
  anari::setAndReleaseParameter(d, group, "surface",
                                anari::newArray1D(d, &surface));
  anari::commitParameters(d, group);

  anari::release(d, surface);

  auto inst = anari::newObject<anari::Instance>(d, "transform");
  anari::setAndReleaseParameter(d, inst, "group", group);
  anari::commitParameters(d, inst);

  return inst;
}

static anari::Instance makeArrowInstance(anari::Device d,
                                         anari::math::float3 v1,
                                         anari::math::float3 v2,
                                         anari::math::float3 color)
{
  // Cylinder geometry
  anari::math::float3 cylPositions[] = {v1, v2};
  auto cylGeom = anari::newObject<anari::Geometry>(d, "cylinder");
  anari::setAndReleaseParameter(d, cylGeom, "vertex.position",
                                anari::newArray1D(d, cylPositions, 2));
  anari::setParameter(d, cylGeom, "radius", 0.02f);
  anari::commitParameters(d, cylGeom);

  // Cone geometry
  anari::math::float3 dir = v1 + v2;
  anari::math::float3 conePositions[] = {v2, v2 + normalize(dir) / 6.f};
  float coneRadii[] = {0.05f, 0.0f};
  auto coneGeom = anari::newObject<anari::Geometry>(d, "cone");
  anari::setAndReleaseParameter(d, coneGeom, "vertex.position",
                                anari::newArray1D(d, conePositions, 2));
  anari::setAndReleaseParameter(d, coneGeom, "vertex.radius",
                                anari::newArray1D(d, coneRadii, 2));
  anari::commitParameters(d, coneGeom);

  // Surfaces and material
  auto mat = anari::newObject<anari::Material>(d, "matte");
  anari::setParameter(d, mat, "color", color);
  anari::commitParameters(d, mat);

  auto cylSurface = anari::newObject<anari::Surface>(d);
  anari::setAndReleaseParameter(d, cylSurface, "geometry", cylGeom);
  anari::setParameter(d, cylSurface, "material", mat);
  anari::commitParameters(d, cylSurface);

  auto coneSurface = anari::newObject<anari::Surface>(d);
  anari::setAndReleaseParameter(d, coneSurface, "geometry", coneGeom);
  anari::setParameter(d, coneSurface, "material", mat);
  anari::commitParameters(d, coneSurface);

  anari::release(d, mat);

  anari::Surface surface[2];
  surface[0] = cylSurface;
  surface[1] = coneSurface;

  auto group = anari::newObject<anari::Group>(d);
  anari::setAndReleaseParameter(d, group, "surface",
                                anari::newArray1D(d, surface, 2));
  anari::commitParameters(d, group);

  anari::release(d, cylSurface);
  anari::release(d, coneSurface);

  auto inst = anari::newObject<anari::Instance>(d, "transform");
  anari::setAndReleaseParameter(d, inst, "group", group);
  anari::commitParameters(d, inst);

  return inst;
}

static anari::Geometry generateSphereMesh(anari::Device device,
                                          const explorer::Material &mat)
{
  float3 viewDir{0.f, 1.f, 0.f};
  float3 lightDir = normalize(g_lightDir);
  float3 lightIntensity{1.f};
  float3 Ng{0.f, 1.f, 0.f}, Ns{0.f, 1.f, 0.f};

  int segments = 400;
  int vertexCount = (segments - 1) * segments;
  int indexCount = ((segments - 2) * segments) * 2;

  auto positionArray =
      anari::newArray1D(device, ANARI_FLOAT32_VEC3, vertexCount);
  auto *position = anari::map<anari::math::float3>(device, positionArray);

  auto indexArray =
      anari::newArray1D(device, ANARI_UINT32_VEC3, indexCount);
  auto *index = anari::map<anari::math::uint3>(device, indexArray);

  int cnt = 0;
  for (int i = 0; i < segments - 1; ++i) {
    for (int j = 0; j < segments; ++j) {
      float phi = M_PI * (i + 1) / float(segments);
      float theta = 2.f * M_PI * j / float(segments);

      anari::math::float3 v(sinf(phi) * cosf(theta), cosf(phi),
                            sinf(phi) * sinf(theta));

      viewDir = normalize(float3(v.x, v.y, v.z));
      float3 value = mat.eval(Ng, Ns, normalize(viewDir), lightDir, lightIntensity);
      float scale = fabsf(value.y);
      position[cnt++] = v * scale;
    }
  }

  cnt = 0;
  for (int j = 0; j < segments - 2; ++j) {
    for (int i = 0; i < segments; ++i) {
      int j0 = j * segments + 1;
      int j1 = (j + 1) * segments + 1;
      unsigned idx0 = j0 + i;
      unsigned idx1 = j0 + (i + 1) % segments;
      unsigned idx2 = j1 + (i + 1) % segments;
      unsigned idx3 = j1 + i;
      index[cnt++] = anari::math::uint3(idx0, idx1, idx2);
      index[cnt++] = anari::math::uint3(idx0, idx2, idx3);
    }
  }

  anari::unmap(device, positionArray);
  anari::unmap(device, indexArray);

  auto geometry = anari::newObject<anari::Geometry>(device, "triangle");
  anari::setAndReleaseParameter(device, geometry, "vertex.position",
                                positionArray);
  anari::setAndReleaseParameter(device, geometry, "primitive.index",
                                indexArray);
  anari::commitParameters(device, geometry);

  return geometry;
}

static anari::Surface makeBRDFSurface(anari::Device device,
                                      const explorer::Material &mat)
{
  auto geometry = generateSphereMesh(device, mat);
  anari::commitParameters(device, geometry);

  auto material = anari::newObject<anari::Material>(device, "matte");
  anari::commitParameters(device, material);

  auto quadSurface = anari::newObject<anari::Surface>(device);
  anari::setAndReleaseParameter(device, quadSurface, "geometry", geometry);
  anari::setAndReleaseParameter(device, quadSurface, "material", material);
  anari::commitParameters(device, quadSurface);
  return quadSurface;
}

static void addPlaneAndArrows(anari::Device device, anari::World world)
{
  std::vector<anari::Instance> instances;

  if (g_showGroundPlane) {
    auto planeInst = makePlaneInstance(device, g_bounds);
    instances.push_back(planeInst);
  }

  if (g_showLightDir && length(g_lightDir) > 0.f) {
    auto ld = normalize(g_lightDir);
    anari::math::float3 origin(0.f, 0.f, 0.f);
    anari::math::float3 lightDir(ld.x, ld.y, ld.z);
    auto lightDirInst = makeArrowInstance(device, origin, (lightDir - origin) * 1.2f,
                                          anari::math::float3(1.f, 1.f, 0.f));
    instances.push_back(lightDirInst);
  }

  if (g_showAxes) {
    auto xInst = makeArrowInstance(device, anari::math::float3(0.f, 0.f, 0.f),
                                   anari::math::float3(1.2f, 0.f, 0.f),
                                   anari::math::float3(1.f, 0.f, 0.f));
    auto yInst = makeArrowInstance(device, anari::math::float3(0.f, 0.f, 0.f),
                                   anari::math::float3(0.f, 1.2f, 0.f),
                                   anari::math::float3(0.f, 1.f, 0.f));
    auto zInst = makeArrowInstance(device, anari::math::float3(0.f, 0.f, 0.f),
                                   anari::math::float3(0.f, 0.f, 1.2f),
                                   anari::math::float3(0.f, 0.f, 1.f));
    instances.push_back(xInst);
    instances.push_back(yInst);
    instances.push_back(zInst);
  }

  if (!instances.empty()) {
    anari::setAndReleaseParameter(
        device, world, "instance",
        anari::newArray1D(device, instances.data(), instances.size()));

    for (auto &i : instances) {
      anari::release(device, i);
    }
  } else {
    anari::unsetParameter(device, world, "instance");
  }

  anari::commitParameters(device, world);
}

static void addBRDFGeom(anari::Device device, anari::World world,
                        const explorer::Material &mat)
{
  std::vector<anari::Surface> surfaces;
  auto brdfSurf = makeBRDFSurface(device, mat);
  surfaces.push_back(brdfSurf);

  anari::setAndReleaseParameter(
      device, world, "surface",
      anari::newArray1D(device, surfaces.data(), surfaces.size()));
  anari::commitParameters(device, world);
}

// ============================================================================
// Application / Main Loop
// ============================================================================

struct AppState {
  anari::Device device{nullptr};
  anari::World world{nullptr};
  explorer::Material *material{nullptr};
  std::vector<std::unique_ptr<Window>> windows;
};

static void parseCommandLine(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-v" || arg == "--verbose")
      g_verbose = true;
    else if (arg == "--help" || arg == "-h") {
      std::cout << "./anariBRDFExplorer [{--help|-h}]\n"
                << "   [{--verbose|-v}] [{--debug|-g}]\n"
                << "   [{--library|-l} <ANARI library>]\n"
                << "   [{--trace|-t} <directory>]\n";
      std::exit(0);
    } else if (arg == "-l" || arg == "--library")
      g_libraryName = argv[++i];
    else if (arg == "--debug" || arg == "-g")
      g_enableDebug = true;
    else if (arg == "--trace")
      g_traceDir = argv[++i];
  }
}

int main(int argc, char *argv[])
{
  parseCommandLine(argc, argv);

  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return 1;
  }

  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window =
      glfwCreateWindow(1440, 900, "ANARI BRDF Explorer", NULL, NULL);
  if (!window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Setup ImGui
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.FontGlobalScale = 1.5f;

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
  ImGui::StyleColorsDark();

  try {
    // Initialize ANARI
    initializeANARI();

    if (!g_device) {
      throw std::runtime_error("Failed to initialize ANARI device");
    }

    // Setup application state
    AppState appState;
    appState.device = g_device;
    appState.world = anari::newObject<anari::World>(g_device);

    // Load material plugin
    explorer::Material::loadPlugin("visionaray_material");
    if (!explorer::Material::pluginLoaded()) {
      std::cerr << "Warning: Material plugin not loaded\n";
    }

    appState.material = explorer::Material::createInstance(g_selectedMaterial);

    // Add geometry to world
    addBRDFGeom(appState.device, appState.world, *appState.material);
    addPlaneAndArrows(appState.device, appState.world);
    anari::commitParameters(appState.device, appState.world);

    // Setup windows
    auto viewport = std::make_unique<Viewport>(g_device, "Viewport");
    viewport->setWorld(appState.world);
    viewport->resetView();

    auto paramEditor = std::make_unique<ParamEditor>(*appState.material,
                                                      g_lightDir,
                                                      g_selectedMaterial);
    paramEditor->setLightUpdateCallback([&]() {
      addPlaneAndArrows(appState.device, appState.world);
      addBRDFGeom(appState.device, appState.world, *appState.material);
    });
    paramEditor->setMaterialUpdateCallback([&]() {
      addBRDFGeom(appState.device, appState.world, *appState.material);
    });

    appState.windows.push_back(std::move(viewport));
    appState.windows.push_back(std::move(paramEditor));

    // Main loop
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      // Start ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Build menu bar
      if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
          for (auto &w : appState.windows) {
            bool visible = w->isVisible();
            if (ImGui::MenuItem(w->name().c_str(), nullptr, &visible)) {
              w->setVisible(visible);
            }
          }
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
          if (ImGui::MenuItem("Enable Denoiser", nullptr, &g_enableDenoiser)) {
            g_denoiser.setEnabled(g_enableDenoiser);
          }
          ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
      }

      // Build window UIs
      for (auto &w : appState.windows) {
        w->buildUI();
      }

      // Rendering
      ImGui::Render();
      int display_w, display_h;
      glfwGetFramebufferSize(window, &display_w, &display_h);
      glViewport(0, 0, display_w, display_h);
      glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);
    }

    // Cleanup
    if (appState.material)
      delete appState.material;
    anari::release(appState.device, appState.world);
    anari::release(appState.device, appState.device);

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }

  // Cleanup ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

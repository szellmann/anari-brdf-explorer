// anari_viewer
#include "anari_viewer/Application.h"
#include "anari_viewer/windows/LightsEditor.h"
#include "anari_viewer/windows/Viewport.h"
// anari
#define ANARI_EXTENSION_UTILITY_IMPL
#include <anari/anari_cpp.hpp>
#include <iostream>
// ours
#include "material.h"
#include "ParamEditor.h"

using box3_t = std::array<anari::math::float3, 2>;
namespace anari {
ANARI_TYPEFOR_SPECIALIZATION(box3_t, ANARI_FLOAT32_BOX3);
ANARI_TYPEFOR_DEFINITION(box3_t);
} // namespace anari

using namespace anari::math;

static const bool g_true = true;
static bool g_verbose = false;
static bool g_useDefaultLayout = true;
static bool g_enableDebug = false;
static std::string g_libraryName = "environment";
static anari::Library g_debug = nullptr;
static anari::Device g_device = nullptr;
static const char *g_traceDir = nullptr;

static  float  g_groundPlaneOpacity = { 0.5f };
static std::string g_selectedMaterial = "Matte";
static  float3 g_lightDir = { 1.f, 1.f, 0.f };
static  float  g_metallic = { 0.f };
static  float  g_roughness = { 0.f };
static  float  g_clearcoat = { 0.f };
static  float  g_clearcoatRoughness = { 0.f };
static  float  g_ior = { 1.f };
static  bool   g_showGroundPlane = { true };
static  bool   g_showLightDir = { true };
static  bool   g_showAxes = { true };
static box3_t  g_bounds = { anari::math::float3{-3.f, 0.f, -3.f},
                            anari::math::float3{3.f, 1.f, 3.f} };

static const char *g_defaultLayout =
    R"layout(
[Window][MainDockSpace]
Pos=0,25
Size=1440,813
Collapsed=0

[Window][Viewport]
Pos=551,25
Size=889,813
Collapsed=0
DockId=0x00000003,0

[Window][Lights Editor]
Pos=0,25
Size=549,813
Collapsed=0
DockId=0x00000002,1

[Window][Param Editor]
Pos=0,25
Size=549,813
Collapsed=0
DockId=0x00000002,0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Docking][Data]
DockSpace   ID=0x782A6D6B Window=0xDEDC5B90 Pos=0,25 Size=1440,813 Split=X
  DockNode  ID=0x00000002 Parent=0x782A6D6B SizeRef=549,1174 Selected=0xE3280322
  DockNode  ID=0x00000003 Parent=0x782A6D6B SizeRef=1369,1174 CentralNode=1 Selected=0x13926F0B
)layout";

void statusFunc(const void *userData,
    ANARIDevice device,
    ANARIObject source,
    ANARIDataType sourceType,
    ANARIStatusSeverity severity,
    ANARIStatusCode code,
    const char *message)
{
  (void)userData;
  if (severity == ANARI_SEVERITY_FATAL_ERROR)
    fprintf(stderr, "[FATAL] %s\n", message);
  else if (severity == ANARI_SEVERITY_ERROR)
    fprintf(stderr, "[ERROR] %s\n", message);
  else if (severity == ANARI_SEVERITY_WARNING)
    fprintf(stderr, "[WARN ] %s\n", message);
  else if (severity == ANARI_SEVERITY_PERFORMANCE_WARNING)
    fprintf(stderr, "[PERF ] %s\n", message);
  else if (severity == ANARI_SEVERITY_INFO)
    fprintf(stderr, "[INFO] %s\n", message);
}

static void anari_free(const void * /*user_data*/, const void *ptr)
{
  std::free(const_cast<void *>(ptr));
}

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
      d, data, &anari_free, nullptr, ANARI_UFIXED8_VEC3, dim, dim);
}

static anari::Surface makePlane(anari::Device d, box3_t bounds)
{
  anari::math::float3 vertices[4];
  vertices[0] = { bounds[0][0], bounds[0][1], bounds[1][2] };
  vertices[1] = { bounds[1][0], bounds[0][1], bounds[1][2] };
  vertices[2] = { bounds[1][0], bounds[0][1], bounds[0][2] };
  vertices[3] = { bounds[0][0], bounds[0][1], bounds[0][2] };

  anari::math::float2 texcoords[4] = {
      {0.f, 0.f},
      {0.f, 1.f},
      {1.f, 1.f},
      {1.f, 0.f},
  };

  auto geom = anari::newObject<anari::Geometry>(d, "quad");
  anari::setAndReleaseParameter(d,
      geom,
      "vertex.position",
      anari::newArray1D(d, vertices, 4));
  anari::setAndReleaseParameter(d,
      geom,
      "vertex.attribute0",
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
  anari::setAndReleaseParameter(
      d, group, "surface", anari::newArray1D(d, &surface));
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
  // Cylinder geometry:
  anari::math::float3 cylPositions[] = { v1, v2 };
  auto cylGeom = anari::newObject<anari::Geometry>(d, "cylinder");
  anari::setAndReleaseParameter(d,
      cylGeom,
      "vertex.position",
      anari::newArray1D(d, cylPositions, 2));
  anari::setParameter(d, cylGeom, "radius", 0.02f);
  anari::commitParameters(d, cylGeom);

  // Cone geometry:
  anari::math::float3 dir = v1 + v2;
  anari::math::float3 conePositions[] = { v2, v2+normalize(dir)/6.f };
  float coneRadii[] = { 0.05f, 0.0f };
  auto coneGeom = anari::newObject<anari::Geometry>(d, "cone");
  anari::setAndReleaseParameter(d,
      coneGeom,
      "vertex.position",
      anari::newArray1D(d, conePositions, 2));
  anari::setAndReleaseParameter(d,
      coneGeom,
      "vertex.radius",
      anari::newArray1D(d, coneRadii, 2));
  anari::commitParameters(d, coneGeom);

  // Surfaces and material:

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
  anari::setAndReleaseParameter(
      d, group, "surface", anari::newArray1D(d, surface, 2));
  anari::commitParameters(d, group);

  anari::release(d, cylSurface);
  anari::release(d, coneSurface);

  auto inst = anari::newObject<anari::Instance>(d, "transform");
  anari::setAndReleaseParameter(d, inst, "group", group);
  anari::commitParameters(d, inst);

  return inst;
}

static anari::Geometry generateSphereMesh(anari::Device device, explorer::Material mat)
{
  float3 viewDir{0.f,1.f,0.f};
  float3 lightDir = normalize(g_lightDir);
  float3 lightIntensity{1.f};
  float3 Ng{0.f,1.f,0.f}, Ns{0.f,1.f,0.f};

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
  for (int i = 0; i < segments-1; ++i) {
    for (int j = 0; j < segments; ++j) {
      float phi = M_PI * (i+1) / float(segments);
      float theta = 2.f * M_PI * j / float(segments);

      anari::math::float3 v(
        sinf(phi) * cosf(theta),
        cosf(phi),
        sinf(phi) * sinf(theta));

      viewDir = normalize(float3(v.x,v.y,v.z));
      float3 value = mat.eval(Ng,Ns,normalize(viewDir),lightDir,lightIntensity);
      float scale = fabsf(value.y);
      position[cnt++] = v * scale;
    }
  }

  cnt = 0;
  for (int j = 0; j < segments-2; ++j) {
    for (int i = 0; i < segments; ++i) {
      int j0 = j * segments + 1;
      int j1 = (j+1) * segments + 1;
      unsigned idx0 = j0 + i;
      unsigned idx1 = j0 + (i+1) % segments;
      unsigned idx2 = j1 + (i+1) % segments;
      unsigned idx3 = j1 + i;
      index[cnt++] = anari::math::uint3(idx0,idx1,idx2);
      index[cnt++] = anari::math::uint3(idx0,idx2,idx3);
    }
  }

  anari::unmap(device, positionArray);
  anari::unmap(device, indexArray);

  //auto geometry = anari::newObject<anari::Geometry>(device, "quad");
  auto geometry = anari::newObject<anari::Geometry>(device, "triangle");
  anari::setAndReleaseParameter(
      device, geometry, "vertex.position", positionArray);
  anari::setAndReleaseParameter(
      device, geometry, "primitive.index", indexArray);
  anari::commitParameters(device, geometry);

  return geometry;
}

#if 0
static anari::Geometry generateSampleMesh(anari::Device device, dco::Material mat)
{
  bool lightDirAsInput{true}; // to test that like doesn't leak underneath the surface

  Random rng(0,0);
  float3 viewDir{0.f,1.f,0.f};
  float3 lightDir = normalize(g_lightDir);
  float3 lightIntensity{1.f};
  float3 Ng{0.f,1.f,0.f}, Ns{0.f,1.f,0.f};
  int primID{0};
  dco::Sampler *samplers{nullptr};
  float4 *attribs{nullptr};

  int segments = 40;
  int vertexCount = (segments - 1) * segments;

  auto positionArray =
      anari::newArray1D(device, ANARI_FLOAT32_VEC3, vertexCount);
  auto *position = anari::map<anari::math::float3>(device, positionArray);

  int cnt = 0;
  for (int i = 0; i < segments-1; ++i) {
    for (int j = 0; j < segments; ++j) {
      float phi = M_PI * (i+1) / float(segments);
      float theta = 2.f * M_PI * j / float(segments);

      anari::math::float3 v(
        sinf(phi) * cosf(theta),
        cosf(phi),
        sinf(phi) * sinf(theta));

      viewDir = normalize(vec3(v.x,v.y,v.z));

      float3 dirIN = lightDirAsInput ? lightDir : viewDir;
      float3 dir;
      float pdf;
      sampleMaterial(mat,samplers,attribs,rng,primID,
                     Ns,Ng,normalize(dirIN),dir,pdf);
      float scale = 1.f;//pdf;
      position[cnt++] = anari::math::float3(dir.x,dir.y,dir.z) * scale;
    }
  }

  anari::unmap(device, positionArray);

  auto geometry = anari::newObject<anari::Geometry>(device, "sphere");
  anari::setAndReleaseParameter(
      device, geometry, "vertex.position", positionArray);

  return geometry;
}
#endif

static anari::Surface makeBRDFSurface(anari::Device device, explorer::Material mat)
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

static anari::Surface makeBRDFSamples(anari::Device device, explorer::Material mat)
{
#if 0
  auto geometry = generateSampleMesh(device, mat);
  anari::commitParameters(device, geometry);

  auto material = anari::newObject<anari::Material>(device, "matte");
  anari::setParameter(device, material, "color", anari::math::float3(0.4f, 0.8f, 0.4f));
  anari::commitParameters(device, material);

  auto quadSurface = anari::newObject<anari::Surface>(device);
  anari::setAndReleaseParameter(device, quadSurface, "geometry", geometry);
  anari::setAndReleaseParameter(device, quadSurface, "material", material);
  anari::commitParameters(device, quadSurface);
  return quadSurface;
#endif
}

static void addPlaneAndArrows(anari::Device device, anari::World world)
{
  std::vector<anari::Instance> instances;

  // ground plane
  if (g_showGroundPlane) {
    auto planeInst = makePlaneInstance(device, g_bounds);
    instances.push_back(planeInst);
  }

  // light dir:
  if (g_showLightDir) {
    if (length(g_lightDir) > 0.f) {
      auto ld = normalize(g_lightDir);
      anari::math::float3 origin(0.f, 0.f, 0.f);
      anari::math::float3 lightDir(ld.x, ld.y, ld.z);
      auto lightDirInst = makeArrowInstance(device,
                                            origin,
                                            (lightDir-origin) * 1.2f,
                                            anari::math::float3(1.f, 1.f, 0.f));
      instances.push_back(lightDirInst);
    }
  }

  // basis vectors:
  if (g_showAxes) {
    auto xInst = makeArrowInstance(device,
                                   anari::math::float3(0.f, 0.f, 0.f),
                                   anari::math::float3(1.2f, 0.f, 0.f),
                                   anari::math::float3(1.f, 0.f, 0.f));
    auto yInst = makeArrowInstance(device,
                                   anari::math::float3(0.f, 0.f, 0.f),
                                   anari::math::float3(0.f, 1.2f, 0.f),
                                   anari::math::float3(0.f, 1.f, 0.f));
    auto zInst = makeArrowInstance(device,
                                   anari::math::float3(0.f, 0.f, 0.f),
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

static void addBRDFGeom(anari::Device device, anari::World world, explorer::Material mat)
{
  std::vector<anari::Surface> surfaces;

  auto brdfSurf = makeBRDFSurface(device, mat);
  surfaces.push_back(brdfSurf);

  //auto brdfSamples = makeBRDFSamples(device, mat);
  //surfaces.push_back(brdfSamples);

  anari::setAndReleaseParameter(
      device, world, "surface",
      anari::newArray1D(device, surfaces.data(), surfaces.size()));
  anari::commitParameters(device, world);
}

#if 0
Renderer::Renderer()
{
  anari.library = anari::loadLibrary("visionaray", statusFunc);
  anari.device = anariNewDevice(anari.library, "default");
  anari.renderer = anari::newObject<anari::Renderer>(anari.device, "default");
  //anari.renderer = anari::newObject<anari::Renderer>(anari.device, "raycast");

  anari.world = anari::newObject<anari::World>(anari.device);

  addBRDFGeom(anari.device, anari.world);
  addPlaneAndArrows(anari.device, anari.world);

  anari.light = anari::newObject<anari::Light>(anari.device, "directional");
  anari::setParameter(anari.device, anari.light, "direction",
      anari::math::float3(1.f, -1.f, -1.f));
  anari::setParameter(anari.device, anari.light, "irradiance", 1.f);
  anari::setParameter(anari.device, anari.light, "color",
      anari::math::float3(1.f, 1.f, 1.f));
  anari::setAndReleaseParameter(anari.device,
      anari.world,
      "light",
      anari::newArray1D(anari.device, &anari.light, 1));

  anari::commitParameters(anari.device, anari.world);

  cam = std::make_shared<AnariCamera>(anari.device);

  float aspect = width() / float(height());
  cam->perspective(60.0f * constants::degrees_to_radians<float>(), aspect, 0.001f, 1000.0f);
  cam->set_viewport(0, 0, width(), height());

  cam->viewAll(g_bounds);
  cam->commit();

  anari::setParameter(anari.device, anari.renderer, "background",
      anari::math::float4(0.6f, 0.6f, 0.6f, 1.0f));

  anari::setParameter(anari.device, anari.renderer, "ambientRadiance", 0.f);
  //anari::setParameter(anari.device, anari.renderer, "mode", "Ng");
  //anari::setParameter(anari.device, anari.renderer, "mode", "Ns");
  //anari::setParameter(anari.device, anari.renderer, "heatMapEnabled", true);
  //anari::setParameter(anari.device, anari.renderer, "taa", true);
  //anari::setParameter(anari.device, anari.renderer, "taaAlpha", 0.1f);
  //anari::math::float4 clipPlane[] = {{ 0.f, 0.f, -1.f, 0.f }};
  //anari::math::float4 clipPlane[] = {{ 0.707f, 0.f, -0.707f, 0.4f }};
  //anari::setAndReleaseParameter(
  //    anari.device, anari.renderer, "clipPlane",
  //    anari::newArray1D(anari.device, clipPlane, 1));

  anari::commitParameters(anari.device, anari.renderer);
}

Renderer::~Renderer()
{
  //cam.reset(nullptr);
  anari::release(anari.device, anari.world);
  anari::release(anari.device, anari.renderer);
  anari::release(anari.device, anari.frame);
  anari::release(anari.device, anari.device);
  anari::unloadLibrary(anari.library);
}

void Renderer::on_display()
{
  anari::render(anari.device, anari.frame);
  anari::wait(anari.device, anari.frame);

  auto channelColor = anari::map<uint32_t>(anari.device, anari.frame, "channel.color");

  glDrawPixels(width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, channelColor.data);

  anari::unmap(anari.device, anari.frame, "channel.color");

  ImGui::Begin("Parameters");
  bool updated = false;
  if (ImGui::DragFloat3("Light dir", (float *)g_lightDir.data())) {
    addPlaneAndArrows(anari.device, anari.world);
    updated = true;
  }

  const char *selected = g_selectedMaterial;
  if (ImGui::BeginCombo("Material Type", g_selectedMaterial)) {
    if (ImGui::Selectable("Matte", std::string(g_selectedMaterial)=="Matte")) {
      selected = "Matte";
    }
    else if (ImGui::Selectable("PBM", std::string(g_selectedMaterial)=="PBM")) {
      selected = "PBM";
    }
    if (std::string(selected) != std::string(g_selectedMaterial)) {
      g_selectedMaterial = selected;
      updated = true;
    }
    ImGui::EndCombo();
  }

  if (std::string(g_selectedMaterial)=="PBM") {
    updated |= ImGui::DragFloat("Metallic", &g_metallic, g_metallic, 0.f, 1.f);
    updated |= ImGui::DragFloat("Roughness", &g_roughness, g_roughness, 0.f, 1.f);
    updated |= ImGui::DragFloat("Clearcoat", &g_clearcoat, g_clearcoat, 0.f, 1.f);
    updated |= ImGui::DragFloat("Clearcoat roughness",
        &g_clearcoatRoughness, g_clearcoatRoughness, 0.f, 1.f);
    updated |= ImGui::DragFloat("IOR", &g_ior, g_ior, 0.f, 10.f);
  }

  if (ImGui::Checkbox("Show axes", &g_showAxes)) {
    addPlaneAndArrows(anari.device, anari.world);
  }

  ImGui::SameLine();
  if (ImGui::Checkbox("Show light dir", &g_showLightDir)) {
    addPlaneAndArrows(anari.device, anari.world);
  }

  ImGui::SameLine();
  if (ImGui::Checkbox("Show ground plane", &g_showGroundPlane)) {
    addPlaneAndArrows(anari.device, anari.world);
  }

  ImGui::End();

  if (updated) {
    addBRDFGeom(anari.device, anari.world);
  }
  viewer_glut::on_display();
}
#endif

namespace viewer {

struct AppState
{
  anari_viewer::manipulators::Orbit manipulator;
  anari::Device device{nullptr};
  anari::World world{nullptr};
};

static void statusFunc(const void *userData,
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
    g_debug = anariLoadLibrary("debug", statusFunc, &g_true);

  anari::Device dev = anariNewDevice(library, "default");

  anari::unloadLibrary(library);

  if (g_enableDebug)
    anari::setParameter(dev, dev, "glDebug", true);

#ifdef USE_GLES2
  anari::setParameter(dev, dev, "glAPI", "OpenGL_ES");
#else
  anari::setParameter(dev, dev, "glAPI", "OpenGL");
#endif

  if (g_enableDebug) {
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

  anari::commitParameters(dev, dev);

  g_device = dev;
}

// Application definition /////////////////////////////////////////////////////

class Application : public anari_viewer::Application
{
 public:
  Application() = default;
  ~Application() override = default;

  anari_viewer::WindowArray setupWindows() override
  {
    anari_viewer::ui::init();

    // ANARI //

    initializeANARI();

    auto device = g_device;

    if (!device)
      std::exit(1);

    m_state.device = device;
    m_state.world = anari::newObject<anari::World>(device);

    m_material = explorer::Material::createInstance(g_selectedMaterial);

    addBRDFGeom(m_state.device, m_state.world, m_material);
    addPlaneAndArrows(m_state.device, m_state.world);

    anari::commitParameters(device, m_state.world);

    // ImGui //

    ImGuiIO &io = ImGui::GetIO();
    io.FontGlobalScale = 1.5f;
    io.IniFilename = nullptr;

    if (g_useDefaultLayout)
      ImGui::LoadIniSettingsFromMemory(g_defaultLayout);

    auto *viewport = new anari_viewer::windows::Viewport(device, "Viewport");
    viewport->setManipulator(&m_state.manipulator);
    viewport->setWorld(m_state.world);
    viewport->resetView();

    auto *leditor = new anari_viewer::windows::LightsEditor({device});
    leditor->setWorlds({m_state.world});

    auto *peditor = new windows::ParamEditor(m_material,
                                             g_lightDir,
                                             g_selectedMaterial);

    peditor->setLightUpdateCallback(
        [=]() {
          addPlaneAndArrows(m_state.device, m_state.world);
          addBRDFGeom(m_state.device, m_state.world, m_material);
        });

    peditor->setMaterialUpdateCallback(
        [=]() {
          addBRDFGeom(m_state.device, m_state.world, m_material);
        });

    // Setup scene //

    anari_viewer::WindowArray windows;
    windows.emplace_back(viewport);
    windows.emplace_back(leditor);
    windows.emplace_back(peditor);
    //  windows.emplace_back(isoeditor);

    return windows;
  }

  void buildMainMenuUI()
  {
  std::cout << "???\n";
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("print ImGui ini")) {
          const char *info = ImGui::SaveIniSettingsToMemory();
          printf("%s\n", info);
        }

        ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
    }
  }

  void teardown() override
  {
    anari::release(m_state.device, m_state.world);
    anari::release(m_state.device, m_state.device);
    anari_viewer::ui::shutdown();
  }

 private:
  AppState m_state;

  explorer::Material m_material;
};

} // namespace viewer

static void printUsage()
{
  std::cout << "./anariBRDFExplorer [{--help|-h}]\n"
            << "   [{--verbose|-v}] [{--debug|-g}]\n"
            << "   [{--library|-l} <ANARI library>]\n"
            << "   [{--trace|-t} <directory>]\n";
}

static void parseCommandLine(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-v" || arg == "--verbose")
      g_verbose = true;
    if (arg == "--help" || arg == "-h") {
      printUsage();
      std::exit(0);
    } else if (arg == "--noDefaultLayout")
      g_useDefaultLayout = false;
    else if (arg == "-l" || arg == "--library")
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
  viewer::Application app;
  app.run(1920, 1200, "ANARI BRDF Explorer");
  return 0;
}

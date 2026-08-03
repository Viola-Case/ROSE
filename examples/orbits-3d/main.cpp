#line 2 "examples/orbits/main.cpp"
/**

  @file       main.cpp
  @brief      Entry point for the orbits example.
  @details    ~
  @author     Viola Case
  @date       03.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>

#include "centralbody.h"
#include "orbitcamera.h"
#include "pointcloud.h"
#include "simcontrol.h"
#include "simhud.h"
#include "tracer.h"

#include <fstream>
#include <sstream>

using namespace ROSE;

int main() {

  /* Registration has to happen before any scene is loaded: the loader resolves the `typeid` strings in the JSON
   * through the factory, and an unregistered one is a null dereference during load rather than a diagnostic. */
  {
    BehaviorFactory &factory = BehaviorFactory::get();
    RoseRegisterCoreModule(factory);
    Pair<FactoryFn, UUID> fns[] {
      { MakeBehavior<OrbitCamera>, OrbitCamera::TypeID() },
      { MakeBehavior<CentralBody>, CentralBody::TypeID() },
      { MakeBehavior<PointCloud>, PointCloud::TypeID() },
      { MakeBehavior<Tracer>, Tracer::TypeID() },
      { MakeBehavior<SimHud>, SimHud::TypeID() },
      { MakeBehavior<SimControl>, SimControl::TypeID() },
    };
    for (const auto &p : fns)
      factory.Register(p.first, p.second, "Orbits");
  }

  auto scenes = List<Scene>();

  String sceneJSON;
  {
    std::ifstream sceneJSONStream("assets/orbits.json");
    if (!sceneJSONStream) {
      ROSE_LOG_ERROR("Could not open assets/orbits.json - run the example from the directory holding assets/.");
      return 1;
    }
    auto sstream = std::stringstream();
    sstream << sceneJSONStream.rdbuf();
    sceneJSON = sstream.str();
  }
  scenes.push_back(Scene::FromJSONString(sceneJSON));

  Application &sim = *new Application("ROSE - orbits", 0, Move(scenes));
  sim.SetFlag(ApplicationFlag::SoftwareRenderer, 1);
  sim.SetWindowSize(1360, 850);

  sim.Init();

  sim.Run();

  delete &sim;

  return 0;
}

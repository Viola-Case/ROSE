/**

    @file      main.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      07.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/



#include <ROSE/ROSE.h>
#include "pointcloud.h"

constexpr int WIDTH = 800, HEIGHT = 800;

using namespace ROSE;
using namespace Orbits;

class Closer : public Behavior {
public:
  static constexpr UUID typeID = "f1f08ac7480fbd15-6c1353928d659a6b"_uuid;
  static constexpr UUID TypeID() { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }
protected:
  void FrameUpdate() override {
    if (InputSystem::GetKey(KeyCode::ESCAPE))
      m_object->GetScene().GetApplication().Quit();
  }
};

int main() {
  if (auto i = Init(); i != InitStatus::Success) {
    return static_cast<int>(i);
  }

  {
    BehaviorFactory &factory = BehaviorFactory::Get();
    RoseRegisterCoreModule(factory);
    Pair<FactoryFn, UUID> fns[] {
      { MakeBehavior<PointCloud>, PointCloud::TypeID() },
      { MakeBehavior<Closer>, Closer::TypeID() },
    };
    for (const auto &p : fns) {
      factory.Register(p.first, p.second, "Orbits");
    }
  }

  ApplicationInitSettings settings { "Orbits" };
  settings.SetFlags(APPLICATION_SOFTWARE_RENDERER).SetWindowSize(WIDTH, HEIGHT).AddSceneFromFile("assets/orbits.json");

  Application app;
  if (const int err = app.Init(Move(settings))) return err;

  app.Run();

  return 0;
}

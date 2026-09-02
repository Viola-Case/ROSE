/**

    @file      main.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      02.09.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include "ROSE/Core/imgui.h"


#include <set>
#include <ROSE/ROSE.h>

using namespace ROSE;

Mesh CUBE_MESH {
                  {
                    { { -1, -1, -1 } },
                    { {  1, -1, -1 } },
                    { { -1,  1, -1 } },
                    { {  1,  1, -1 } },
                    { { -1, -1,  1 } },
                    { { -1,  1,  1 } },
                    { {  1, -1,  1 } },
                    { {  1,  1,  1 } }
                  },
              { // -Z (back)
                   0, 2, 1, 1, 2, 3,

                   // +Z (front)
                   4, 6, 5, 6, 7, 5,

                   // -X (left)
                   0, 4, 2, 2, 4, 5,

                   // +X (right)
                   1, 3, 6, 3, 7, 6,

                   // -Y (bottom)
                   0, 1, 4, 1, 6, 4,

                   // +Y (top)
                   2, 5, 3, 3, 5, 7

                 } };

class RotateFunny : public Behavior {
public:
  static constexpr UUID typeID = "71a70e9852f27ed4-e177f66081659b53"_uuid;
  static constexpr UUID TypeID() { return typeID; }
  UUID GetTypeID() const noexcept override { return TypeID(); }
protected:
  void OnStart() override {
    m_motion = GetObject().FindBehavior<Motion>();
    if (!m_motion) {
      ROSE_LOG_ERROR("Motion behavior not found!!!\n");
      GetObject().CreateBehavior<Motion>();
    }
  }
  void FrameUpdate() override {
    if (!m_motion) {
      m_motion = GetObject().FindBehavior<Motion>();
      if (!m_motion) {
        ROSE_LOG_ERROR("Motion behavior not found!!!\n");
        GetObject().CreateBehavior<Motion>();
      } else {
        m_motion->SetAngularVelocity({0.5, 0.6, 1.4});
      }
      return;
    }
    Motion &m = *m_motion;
    auto angv = m.GetAngularVelocity();
    angv = (Quatd::FromEuler(angv) * Quatd::FromEuler({0.1, 0.8, 0.7})).ToEuler();
    m.SetAngularVelocity(
      angv
      );

  }
private:
  Motion *m_motion{nullptr};
};

int main() {

  if (Init() != InitStatus::Success) {
    ROSE_LOG_FATAL("Failed to initialize ROSE.");
    return 1;
  }

  BehaviorFactory::Get().Register(
    MakeBehavior<RotateFunny>, RotateFunny::TypeID(), "Cube"
    );

  /* The registry takes ownership, so it gets a copy of its own - handing it the address of a
   * global would have it delete a static at exit. */
  MeshRegistry::Get().RegisterMesh(new Mesh(CUBE_MESH), "13a2e5be51dcfa51-70653c9582659933"_uuid, "Cube");

  Application app;

  {
    ApplicationInitSettings settings{"CUBE"};
    settings.SetFlag(ApplicationFlag::OpenGL, true)
      .SetWindowSize(800,800)
      .AddSceneFromFile("assets/cube.json")
      .SetVSync(false)
      .SetTargetFrameRate(60);

    if (const auto err = app.Init(Move(settings))) return err;
  }

  AttachImGui();

  app.Run();



}

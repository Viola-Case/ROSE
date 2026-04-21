#include <ROSE/ROSE.h>
#include <ROSE/Core/ROSE_io.h>

using namespace ROSE;

// ---------------------------------------------------------------------------
// AppCloser
// Quits the application on its first active FrameUpdate.
// Behaviors reach m_behaviors on the tick after Apply(), so this fires on
// the second frame.
// ---------------------------------------------------------------------------
class AppCloser : public Behavior {
public:
  static void SetApplication(Application &app) noexcept { s_app = &app; }

protected:
  void OnStart()     override {}
  void FixedUpdate() override {}
  void FrameUpdate() override { if (s_app) s_app->Quit(); }

private:
  static Application *s_app;
};

Application *AppCloser::s_app = nullptr;

// ---------------------------------------------------------------------------

int main() {
  // Register the behavior type so BehaviorFactory can instantiate it when
  // SceneIO::Apply encounters "AppCloser" in the scene descriptor.
  BehaviorFactory::Register<AppCloser>("AppCloser");

  // Load the premade scene from JSON.
  const SceneDesc desc = SceneDesc::LoadFromFile("game1.scene");

  // Build the application with one empty scene.
  List<Scene> scenes;
  scenes.push_back(Scene());
  Application game("Game 1", 0, Move(scenes));

  // Give AppCloser a way to reach the application before Run().
  AppCloser::SetApplication(game);

  game.Init();

  // Populate the current scene from the descriptor.
  SceneIO::Apply(game.GetCurrentScene(), desc);

  game.Run();
}

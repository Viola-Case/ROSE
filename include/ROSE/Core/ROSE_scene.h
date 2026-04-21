/**

  @file      ROSE_scene.h
  @brief     Scene class — container for all Objects in a level
  @details   A Scene owns an ordered collection of Objects and drives their
             per-frame updates. Scenes are managed by an Application; obtain
             the owning application via GetApplication().
  @author    Viola Case
  @date      23.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once
#include <ROSE/Core/ROSE_object.h>

//class Application;

namespace ROSE {
  /**
    @class   Scene
    @brief   Owner of all game objects within a single level or game state.
    @details Objects are added and destroyed lazily — changes take effect at the
             start of the next frame so that in-flight iterators are never
             invalidated mid-update.
  **/
  class Scene final {
    friend class Application;
    friend class Object;
    friend class SceneIO;
  public:
    /**
      @brief   Returns a reference to the Application that owns this scene.
    **/
    Application &GetApplication() const noexcept;
  private:
    /**
      @brief   Transfers ownership of an Object into this scene.
      @details The object is queued and activated on the next frame.
      @param   Object r-value to be adopted by the scene.
    **/
    void AddObject(Object &&) noexcept;

    /**
      @brief   Schedules an object for destruction at the end of the current frame.
      @param   u  UUID of the object to destroy.
    **/
    void DestroyObject(const UUID &u) noexcept;

    /**
      @brief   Called once on the first frame to start all pending objects.
    **/
    void OnStart() noexcept;

    /**
      @brief   Advances the scene by one frame: starts pending objects,
               updates active objects, and removes destroyed objects.
    **/
    void FrameUpdate() noexcept;

    /**
      @brief   Finds an object by its UUID.
      @param   UUID of the desired object.
      @retval  Pointer to the Object, or nullptr if not found.
    **/
    Object *GetObject(const UUID &) noexcept;


  private:
    TypedHashMap<UUID, UniquePtr<Object>> m_objects{};
    List<UniquePtr<Object>> m_pendingAdd{};
    List<const UUID> m_pendingDestroy{};
    Application *m_application{nullptr};
  };
}

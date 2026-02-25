/**

    @file      ROSE_object.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/ROSE_preamble.h>

namespace ROSE {
  class Behavior;
  template <class T>
  concept BehaviorType = std::derived_from<T, Behavior>;


  class Object {
    friend class Behavior;
    friend class Scene;

  public:
    const Scene *GetParentScene();
    
    template<BehaviorType T>
    T *GetBehaviorOfType() {
      auto it = behaviors.find(TypeIdOf<T>());
      if (it != behaviors.end())
        return static_cast<T *>(it->second.get());
      return nullptr;
    }

  private:
    TypedHashMap<UUID, UniquePtr<Behavior>> behaviors;
    Scene *const Parent;

  };
}
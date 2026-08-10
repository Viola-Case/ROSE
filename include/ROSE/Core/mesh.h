/**

  @file       mesh.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       14.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/macros.h>
#include <ROSE/Core/math.h>

namespace ROSE {
  /*!
   *
   */
  struct Vert {
    Vec3d position;
    Vec3d normal;
    Vec2d texCoord;
  };

  struct Tri {
    Vert verts[3];
  };

  /*!
   *
   */
  struct Mesh {
    List<Vert> vertices;
    List<uint32_t> indices;
  };
  /*!
   *
   */
  struct MeshInstance {
    Mesh *mesh;
  };

  class ROSE_API(Core) MeshRegistry {
  public:
    static MeshRegistry &Get() noexcept;
    void RegisterMesh (Mesh *mesh, const UUID &id, const String &name);
    const Mesh *GetMesh(const UUID &id) noexcept;
    const Mesh *GetMesh(const String &name) noexcept;

  private:
    MeshRegistry() = default;
    TypedHashMap<UUID, UniquePtr<Mesh>> m_meshes;
    TypedHashMap<String, UUID> m_meshNameIDs;

  };
}

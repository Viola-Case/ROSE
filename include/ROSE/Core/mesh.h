/**

  @file       mesh.h
  @brief      
  @details    ~
  @author     Viola Case
  @date       14.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.
  
**/
#pragma once

#include <ROSE/Core/api.h>
#include <ROSE/Core/math.h>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/uuid.h>

namespace ROSE {
  /*!
   * One corner of a triangle.
   *
   * Float, not double: a `Vert` is bulk data measured in millions, and every consumer downstream
   * of here is float already - `DrawVertex` (`gfx.h`) is what actually reaches a backend, and it
   * has never carried anything wider. Doubles here bought a conversion per vertex per rebuild and
   * nothing else.
   *
   * @note `normal` is read nowhere yet. `DrawVertex` carries position, colour and texcoord only,
   *       so no normal crosses the behavior/backend boundary and there is no lighting to consume
   *       one. The field is kept because every importer worth having will produce it.
   */
  struct Vert {
    Vec3f position;
    Vec3f normal;
    Vec2f texCoord;
  };

  struct Tri {
    Vert verts[3];
  };

  /*!
   * Indexed triangles, and nothing else. No material, no name, no transform - a `Mesh` is the
   * geometry, and everything about how it is drawn belongs to the `MeshRenderable` that points
   * at it, so the same mesh can be drawn many times without being copied.
   */
  struct Mesh {
    List<Vert>     vertices;
    List<uint32_t> indices;
  };
  /*!
   *
   */
  struct MeshInstance {
    Mesh *mesh;
  };

  /*!
   * The one place a mesh's geometry lives, keyed by id. It owns the `Mesh`es and hands out
   * borrowed pointers.
   *
   * Why an id and not a pointer: a `MeshRenderable` names its mesh by UUID, which is what lets a
   * scene file reference geometry that does not exist yet at parse time. Resolution happens on
   * the first frame instead, so registration order and scene-load order do not have to agree.
   *
   * Deliberately no more than this - no asset-file integration, no streaming, no reference
   * counting, and no thread safety. `TextureRegistry` is the same design applied to pixels.
   */
  class ROSE_API(CORE) MeshRegistry {
  public:
    static MeshRegistry &Get() noexcept;

    /*!
     * Takes ownership of @p mesh, on every path. A null mesh is ignored; an invalid or
     * already-registered id is refused and the mesh is destroyed rather than leaked, so a caller
     * never has to know whether registration succeeded to know who frees it.
     *
     * @param name optional. When given, the mesh can also be found by it. A name already in use
     *             is reassigned to the new mesh, and the old one keeps only its id.
     */
    void RegisterMesh(Mesh *mesh, const UUID &id, const String &name);

    [[nodiscard]] const Mesh *GetMesh(const UUID &id) noexcept;
    [[nodiscard]] const Mesh *GetMesh(const String &name) noexcept;

    //! @retval `UUID::Invalid()` when no mesh is registered under that name.
    [[nodiscard]] UUID GetMeshID(const String &name) noexcept;

    /*!
     * Drops everything.
     *
     * `MeshRenderable` caches the borrowed pointer it got from `GetMesh` across frames, so this
     * dangles every live renderable. Tear the scene down first. (Registering is safe by
     * contrast: the map holds `UniquePtr<Mesh>`, so a rehash moves the pointer, not the mesh.)
     */
    void Clear() noexcept;

  private:
    MeshRegistry() = default;

    TypedHashMap<UUID, UniquePtr<Mesh>> m_meshes;
    TypedHashMap<String, UUID>          m_meshNameIDs;
  };
} // namespace ROSE

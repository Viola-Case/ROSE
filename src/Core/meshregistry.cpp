/**

    @file      meshregistry.cpp
    @brief     
    @details   ~
    @author    Viola Case
    @date      30.07.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>

namespace ROSE {
  MeshRegistry &MeshRegistry::Get() noexcept {
    static MeshRegistry instance;
    return instance;
  }

  void MeshRegistry::RegisterMesh(Mesh *mesh, const UUID &id, const String &name) {
    if (!mesh) return;

    if (id == UUID::Invalid()) {
      ROSE_LOG_ERROR("Refusing to register mesh '{}' under an invalid id.", name);
      delete mesh;
      return;
    }

    /* The lookup before the insert is load-bearing: TypedHashMap::insert overwrites an existing
     * key, which would destroy the incumbent mesh and dangle every renderable holding it. */
    if (m_meshes.find(id) != m_meshes.end()) {
      ROSE_LOG_WARN("A mesh is already registered under that id; '{}' was dropped.", name);
      delete mesh;
      return;
    }

    m_meshes.insert(id, UniquePtr<Mesh>(mesh));
    if (!name.empty()) m_meshNameIDs.insert(name, id);
  }

  const Mesh *MeshRegistry::GetMesh(const UUID &id) noexcept {
    auto &m = m_meshes;
    if (const auto it = m.find(id); it != m.end()) {
      return it->second.get();
    }
    return nullptr;
  }

  const Mesh *MeshRegistry::GetMesh(const String &name) noexcept {
    if (auto it = m_meshNameIDs.find(name); it != m_meshNameIDs.end()) {
      if (auto it2 = m_meshes.find(it->second); it2 != m_meshes.end()) {
        return it2->second.get();
      }
    }
    return nullptr;
  }

  UUID MeshRegistry::GetMeshID(const String &name) noexcept {
    if (const auto it = m_meshNameIDs.find(name); it != m_meshNameIDs.end()) return it->second;
    return UUID::Invalid();
  }

  void MeshRegistry::Clear() noexcept {
    m_meshes.clear();
    m_meshNameIDs.clear();
  }
} // namespace ROSE

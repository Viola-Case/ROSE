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
} // namespace ROSE
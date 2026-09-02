/**

  @file       texture.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       29.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>
#include <ROSE/Core/texture.h>

namespace ROSE {

  TextureRegistry &TextureRegistry::Get() noexcept {
    static TextureRegistry instance;
    return instance;
  }

  void TextureRegistry::RegisterTexture(Surface *texture, const UUID &id, const String &name) {
    if (!texture) return;

    if (id == UUID::Invalid()) {
      ROSE_LOG_ERROR("Refusing to register texture '{}' under an invalid id.\n", name);
      delete texture;
      return;
    }

    if (m_textures.find(id) != m_textures.end()) {
      ROSE_LOG_WARN("A texture is already registered under that id; '{}' was dropped.\n", name);
      delete texture;
      return;
    }

    /* One format past this point, so no sampler and no uploader has to branch. The conversion is
     * a no-op when the decoder already produced ARGB. */
    if (!texture->ConvertTo(PixelFormat::ARGB32))
      ROSE_LOG_WARN("Texture '{}' could not be converted to ARGB32; it stays in its source format.\n", name);

    m_textures.insert(id, UniquePtr<Surface>(texture));
    if (!name.empty()) m_textureNameIDs.insert(name, id);
  }

  const Surface *TextureRegistry::GetTexture(const UUID &id) noexcept {
    if (const auto it = m_textures.find(id); it != m_textures.end()) return it->second.get();
    return nullptr;
  }

  const Surface *TextureRegistry::GetTexture(const String &name) noexcept {
    if (const auto it = m_textureNameIDs.find(name); it != m_textureNameIDs.end())
      return GetTexture(it->second);
    return nullptr;
  }

  UUID TextureRegistry::GetTextureID(const String &name) noexcept {
    if (const auto it = m_textureNameIDs.find(name); it != m_textureNameIDs.end()) return it->second;
    return UUID::Invalid();
  }

  void TextureRegistry::Clear() noexcept {
    m_textures.clear();
    m_textureNameIDs.clear();
  }

  UUID LoadTexture(const String &path, const String &name) {
    Surface surface = Surface::LoadImage(path.c_str());
    if (!surface.IsValid()) {
      ROSE_LOG_ERROR("Texture '{}' could not be loaded from '{}'.\n", name, path);
      return UUID::Invalid();
    }

    const UUID id = UUID::Generate();
    TextureRegistry::Get().RegisterTexture(new Surface(Move(surface)), id, name);
    return id;
  }

} // namespace ROSE

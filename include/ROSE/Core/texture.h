/**

  @file       texture.h
  @brief      Where textures live between loading them and drawing them.
  @details    ~
  @author     Viola Case
  @date       29.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/api.h>
#include <ROSE/Core/rtl.h>
#include <ROSE/Core/surface.h>
#include <ROSE/Core/uuid.h>

namespace ROSE {

  /*!
   * The one place a texture's pixels live, keyed by id. Shaped after `MeshRegistry`, and
   * deliberately no more than that - no asset-file integration, no streaming, no reference
   * counting. It owns CPU-side `Surface`s and hands out borrowed pointers.
   *
   * Why the pixels and not a backend handle: a `Renderable` refers to a texture by id and never
   * holds anything a backend owns, so the same scene draws on every backend. Each backend keeps
   * its own cache against that id - an `SDL_Texture`, a GL name - and fills it from here on
   * first use. The software rasterizer needs the pixels themselves, so this is also the only
   * representation all three can share.
   *
   * Everything registered here is normalised to `PixelFormat::ARGB32`, so a sampler never has to
   * branch on format.
   */
  class ROSE_API(CORE) TextureRegistry {
  public:
    static TextureRegistry &Get() noexcept;

    /*! Takes ownership of @p texture. A duplicate id is rejected and the texture is destroyed. */
    void RegisterTexture(Surface *texture, const UUID &id, const String &name);

    [[nodiscard]] const Surface *GetTexture(const UUID &id) noexcept;
    [[nodiscard]] const Surface *GetTexture(const String &name) noexcept;

    [[nodiscard]] UUID GetTextureID(const String &name) noexcept;

    /*! Drops everything. Backends cache against these ids, so shut them down first. */
    void Clear() noexcept;

  private:
    TextureRegistry() = default;

    TypedHashMap<UUID, UniquePtr<Surface>> m_textures;
    TypedHashMap<String, UUID> m_textureNameIDs;
  };

  /*!
   * Decode an image off disk and register it under a fresh id.
   *
   * @retval `UUID::Invalid()` if the file could not be read or decoded.
   */
  ROSE_API(CORE) UUID LoadTexture(const String &path, const String &name);

} // namespace ROSE

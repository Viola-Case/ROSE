/**

  @file       sdlrenderer.cpp
  @brief      The backend that draws through SDL's own 2D renderer.
  @details    Was softwarerenderer.cpp, which is what it was never doing: SDL is asked for
              "vulkan,direct3d11,opengl,gpu,software" in priority order, so this runs on the GPU
              and "software" is a last-resort fallback that in practice never fires. The real
              software rasterizer is its own backend now.
  @author     Viola Case
  @date       20.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "sdlpresenter.h"

#include <ROSE/Core/surface.h>
#include <ROSE/Core/texture.h>

namespace ROSE {

  /* SDL wants tightly packed arrays of its own structs. Rather than copy into them, the scratch
   * buffers are declared as the math types with the same layout, and asserted to match here. */
  static_assert(sizeof(Vec4f) == sizeof(SDL_FColor), "DrawVertex::color must alias SDL_FColor");
  static_assert(sizeof(Vec2f) == sizeof(SDL_FPoint), "scratch XY must alias SDL_FPoint");
  static_assert(sizeof(Vec4f) == sizeof(SDL_FRect), "scratch rects must alias SDL_FRect");

  namespace {
    SDLPresenter *AsPresenter(void *_p) noexcept { return static_cast<SDLPresenter *>(_p); }
  } // namespace

  SDLRenderer::SDLRenderer() = default;

  SDLRenderer::~SDLRenderer() {
    Shutdown();
    delete AsPresenter(m_presenter);
    m_presenter = nullptr;
  }

  BackendStatus SDLRenderer::Init(const RenderBackendContext &_ctx) {
    if (!m_presenter) m_presenter = new SDLPresenter();
    return AsPresenter(m_presenter)->Init(_ctx);
  }

  void SDLRenderer::Shutdown() {
    for (auto &entry : m_textures)
      if (entry.second) SDL_DestroyTexture(static_cast<SDL_Texture *>(entry.second));
    m_textures.clear();

    if (m_presenter) AsPresenter(m_presenter)->Shutdown();
    DetachAllRenderables();
  }

  void SDLRenderer::BeginFrame() {
    const SDLPresenter *p = AsPresenter(m_presenter);
    p->Clear({ 0.f, 0.f, 0.f, 1.f });
    p->NewFrame();
  }

  void SDLRenderer::EndFrame() { AsPresenter(m_presenter)->PresentWithImGui(); }

  void SDLRenderer::OnResize(int _width, int _height) {
    /* SDL's renderer tracks its window's size on its own, and the logical presentation is left
     * at its default (disabled), so there is no target to rebuild here. Kept as the hook the
     * base class requires, and as the place a logical-size policy would go. */
    (void)_width;
    (void)_height;
  }

  void *SDLRenderer::GetNativeHandle() const {
    return m_presenter ? AsPresenter(m_presenter)->Renderer() : nullptr;
  }

  const char *SDLRenderer::GetName() const { return "SDL Renderer"; }

  void *SDLRenderer::ResolveTexture(const TextureID &_id) noexcept {
    if (_id == UUID::Invalid()) return nullptr;

    if (const auto it = m_textures.find(_id); it != m_textures.end()) return it->second;

    // First miss: build one from the registry's CPU pixels and keep it for the backend's life.
    SDL_Texture *texture = nullptr;
    if (const Surface *surface = TextureRegistry::Get().GetTexture(_id); surface && surface->IsValid()) {
      texture = SDL_CreateTextureFromSurface(AsPresenter(m_presenter)->Renderer(),
                                             static_cast<SDL_Surface *>(surface->GetNativeHandle()));
      if (!texture) ROSE_LOG_WARN("Could not upload texture. SDL Error: {}\n", SDL_GetError());
    }

    // Cached even when null, so a missing texture costs one lookup rather than one attempt a frame.
    m_textures.insert(_id, texture);
    return texture;
  }

  void SDLRenderer::Draw(const DrawCommand &_cmd) {
    SDL_Renderer *renderer = AsPresenter(m_presenter)->Renderer();
    if (!renderer || _cmd.vertexCount == 0) return;

    const bool screenSpace = (_cmd.flags & RENDERABLE_SCREEN_SPACE) != 0;
    const bool blend = (_cmd.flags & RENDERABLE_TRANSPARENT) != 0;

    SDL_SetRenderDrawBlendMode(renderer, blend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);

    /* Vertices reach SDL in window pixels either way. Screen-space commands are already there;
     * world-space ones go through the camera on the CPU, because SDL's 2D renderer has no vertex
     * stage to hand a matrix to. */
    m_scratch.clear();
    m_scratch.reserve(_cmd.vertexCount);

    if (screenSpace) {
      for (size_t i = 0; i < _cmd.vertexCount; ++i)
        m_scratch.push_back(_cmd.vertices[i]);
    } else {
      const Mat4f mvp = m_viewProjection * _cmd.transform;
      const math::Vec2<int> output = AsPresenter(m_presenter)->OutputSize();
      const float halfW = static_cast<float>(output.x) * 0.5f;
      const float halfH = static_cast<float>(output.y) * 0.5f;

      for (size_t i = 0; i < _cmd.vertexCount; ++i) {
        const DrawVertex &src = _cmd.vertices[i];
        const Vec4f clip = mvp * Vec4f { src.position.x, src.position.y, src.position.z, 1.f };

        /* No near-plane clipping yet, so a vertex at or behind the eye has no meaningful screen
         * position. Collapsing it to the origin keeps the triangle degenerate and off-screen
         * instead of scattering it across the viewport. */
        DrawVertex out = src;
        if (clip.w > 0.f) {
          const float inv = 1.f / clip.w;
          out.position = { (clip.x * inv + 1.f) * halfW, (1.f - clip.y * inv) * halfH, clip.z * inv };
        } else {
          out.position = { -1.f, -1.f, 0.f };
        }
        m_scratch.push_back(out);
      }
    }

    const auto *first = m_scratch.data();
    const auto count = static_cast<int>(m_scratch.size());

    switch (_cmd.topology) {
    case Topology::Triangles: {
      auto *texture = static_cast<SDL_Texture *>((_cmd.flags & RENDERABLE_TEXTURED) ? ResolveTexture(_cmd.texture)
                                                                                    : nullptr);
      const int stride = static_cast<int>(sizeof(DrawVertex));

      if (!SDL_RenderGeometryRaw(renderer, texture, &first->position.x, stride,
                                 reinterpret_cast<const SDL_FColor *>(&first->color), stride,
                                 &first->texCoord.x, stride, count,
                                 _cmd.indices, static_cast<int>(_cmd.indexCount),
                                 static_cast<int>(sizeof(uint32_t))))
        ROSE_LOG_WARN("SDL_RenderGeometryRaw failed: {}\n", SDL_GetError());
      break;
    }

    case Topology::Points: {
      /* SDL's point and line calls carry no per-vertex colour, so the whole command draws in the
       * first vertex's. Full per-vertex colour is a Triangles-only guarantee on this backend. */
      SDL_SetRenderDrawColorFloat(renderer, first->color.x, first->color.y, first->color.z, first->color.w);

      if (_cmd.pointSize > 1.f) {
        const float size = _cmd.pointSize;
        const float half = size * 0.5f;
        m_scratchRects.clear();
        m_scratchRects.reserve(m_scratch.size());
        for (const DrawVertex &v : m_scratch)
          m_scratchRects.push_back(Vec4f { v.position.x - half, v.position.y - half, size, size });

        SDL_RenderFillRects(renderer, reinterpret_cast<const SDL_FRect *>(m_scratchRects.data()), count);
      } else {
        m_scratchXY.clear();
        m_scratchXY.reserve(m_scratch.size());
        for (const DrawVertex &v : m_scratch)
          m_scratchXY.push_back(Vec2f { v.position.x, v.position.y });

        SDL_RenderPoints(renderer, reinterpret_cast<const SDL_FPoint *>(m_scratchXY.data()), count);
      }
      break;
    }

    case Topology::Lines: {
      SDL_SetRenderDrawColorFloat(renderer, first->color.x, first->color.y, first->color.z, first->color.w);

      /* Disjoint pairs, per Topology::Lines - SDL_RenderLines would draw a connected strip, so
       * the segments go one at a time. */
      const size_t total = _cmd.indices ? _cmd.indexCount : m_scratch.size();
      for (size_t i = 0; i + 1 < total; i += 2) {
        const DrawVertex &a = m_scratch[_cmd.indices ? _cmd.indices[i] : i];
        const DrawVertex &b = m_scratch[_cmd.indices ? _cmd.indices[i + 1] : i + 1];
        SDL_RenderLine(renderer, a.position.x, a.position.y, b.position.x, b.position.y);
      }
      break;
    }
    }
  }

} // namespace ROSE

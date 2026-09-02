/**

  @file       softwarerenderer.cpp
  @brief      A software rasterizer that owns its framebuffer.
  @details    Draws into plain memory this class owns and blits once per frame through a
              streaming SDL_Texture. Owning the memory rather than locking an SDL surface per
              primitive is what keeps the inner loop free of SDL, and decoupling the internal
              resolution from the window is the single largest performance lever here.

              This pass is 2D parity only: no depth buffer, no perspective-correct
              interpolation, no near-plane clipping, no texture sampling. A vertex at or behind
              the eye has no meaningful screen position and is collapsed rather than scattered.
  @author     Viola Case
  @date       29.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include "sdlpresenter.h"

namespace ROSE {

  namespace {
    SDLPresenter *AsPresenter(void *_p) noexcept { return static_cast<SDLPresenter *>(_p); }

    constexpr uint32_t PackARGB(const Vec4f &_c) noexcept {
      const auto to8 = [](const float _v) noexcept -> uint32_t {
        const float clamped = _v < 0.f ? 0.f : (_v > 1.f ? 1.f : _v);
        return static_cast<uint32_t>(clamped * 255.f + 0.5f);
      };
      return (to8(_c.w) << 24) | (to8(_c.x) << 16) | (to8(_c.y) << 8) | to8(_c.z);
    }

    /*! Edge function. Positive on the inside of a positively-wound triangle. */
    constexpr float Edge(const float _ax, const float _ay, const float _bx, const float _by, const float _px,
                         const float _py) noexcept {
      return (_bx - _ax) * (_py - _ay) - (_by - _ay) * (_px - _ax);
    }

    /*! Fill rule. Screen space is y-down and the caller has already made the winding positive,
     *  so a shared edge is rasterized by exactly one of the two triangles that meet on it. */
    constexpr bool IsTopLeft(const Vec3f &_from, const Vec3f &_to) noexcept {
      const bool isTop = _from.y == _to.y && _to.x > _from.x;
      const bool isLeft = _to.y < _from.y;
      return isTop || isLeft;
    }
  } // namespace

  SoftwareRenderer::SoftwareRenderer(const int _internalWidth, const int _internalHeight)
    : m_requestedWidth(_internalWidth), m_requestedHeight(_internalHeight) {}

  SoftwareRenderer::~SoftwareRenderer() {
    Shutdown();
    delete AsPresenter(m_presenter);
    m_presenter = nullptr;
  }

  BackendStatus SoftwareRenderer::Init(const RenderBackendContext &_ctx) {
    if (!m_presenter) m_presenter = new SDLPresenter();

    if (const BackendStatus status = AsPresenter(m_presenter)->Init(_ctx); status != BackendStatus::Success)
      return status;

    const int width = m_requestedWidth > 0 ? m_requestedWidth : _ctx.width;
    const int height = m_requestedHeight > 0 ? m_requestedHeight : _ctx.height;
    Resize(width, height);

    return m_color.empty() ? BackendStatus::Failure : BackendStatus::Success;
  }

  void SoftwareRenderer::Shutdown() {
    if (m_target) {
      SDL_DestroyTexture(static_cast<SDL_Texture *>(m_target));
      m_target = nullptr;
    }
    if (m_presenter) AsPresenter(m_presenter)->Shutdown();
    DetachAllRenderables();
  }

  void SoftwareRenderer::Resize(const int _width, const int _height) {
    const int width = _width > 0 ? _width : 1;
    const int height = _height > 0 ? _height : 1;
    if (width == m_width && height == m_height && m_target) return;

    m_width = width;
    m_height = height;
    m_color.resize(static_cast<size_t>(m_width) * static_cast<size_t>(m_height));

    if (m_target) {
      SDL_DestroyTexture(static_cast<SDL_Texture *>(m_target));
      m_target = nullptr;
    }

    SDL_Renderer *renderer = AsPresenter(m_presenter)->Renderer();
    if (!renderer) return;

    SDL_Texture *target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                            m_width, m_height);
    if (!target) {
      ROSE_LOG_ERROR("Could not create the present target. SDL Error: {}\n", SDL_GetError());
      return;
    }

    /* Nearest, not linear: the point of a fixed low internal resolution is chunky pixels, and a
     * bilinear upscale would smear them into mush. */
    SDL_SetTextureScaleMode(target, SDL_SCALEMODE_NEAREST);
    m_target = target;

    /* Resize events arrive after BeginFrame has already cleared, so a freshly allocated buffer
     * would otherwise present one frame of whatever the allocator handed back. */
    ClearTo(m_clear);
  }

  void SoftwareRenderer::SetInternalResolution(const int _width, const int _height) {
    m_requestedWidth = _width;
    m_requestedHeight = _height;

    if (_width > 0 && _height > 0) {
      Resize(_width, _height);
    } else if (m_presenter) {
      const math::Vec2<int> output = AsPresenter(m_presenter)->OutputSize();
      Resize(output.x, output.y);
    }
  }

  void SoftwareRenderer::OnResize(const int _width, const int _height) {
    // A fixed internal resolution ignores the window: only the destination rect changes, and
    // that is handled by presenting with a null rect.
    if (m_requestedWidth > 0 && m_requestedHeight > 0) return;
    Resize(_width, _height);
  }

  void SoftwareRenderer::ClearTo(const uint32_t _argb) noexcept {
    for (uint32_t &pixel : m_color) pixel = _argb;
  }

  void SoftwareRenderer::BeginFrame() {
    ClearTo(m_clear);
    AsPresenter(m_presenter)->NewFrame();
  }

  void SoftwareRenderer::EndFrame() {
    SDLPresenter *presenter = AsPresenter(m_presenter);
    SDL_Renderer *renderer = presenter->Renderer();

    if (m_target && renderer) {
      void *pixels = nullptr;
      int pitch = 0;
      if (SDL_LockTexture(static_cast<SDL_Texture *>(m_target), nullptr, &pixels, &pitch)) {
        const auto *src = reinterpret_cast<const uint8_t *>(m_color.data());
        auto *dst = static_cast<uint8_t *>(pixels);
        const size_t rowBytes = static_cast<size_t>(m_width) * sizeof(uint32_t);

        // The pitch SDL hands back is not necessarily m_width * 4.
        if (pitch == static_cast<int>(rowBytes)) {
          std::memcpy(dst, src, rowBytes * static_cast<size_t>(m_height));
        } else {
          for (int y = 0; y < m_height; ++y)
            std::memcpy(dst + static_cast<size_t>(y) * static_cast<size_t>(pitch),
                        src + static_cast<size_t>(y) * rowBytes, rowBytes);
        }
        SDL_UnlockTexture(static_cast<SDL_Texture *>(m_target));
      }

      // Null destination rect stretches to the output: the upscale, free.
      SDL_RenderTexture(renderer, static_cast<SDL_Texture *>(m_target), nullptr, nullptr);
    }

    // ImGui after the blit, so the HUD stays crisp at native size over a chunky world.
    presenter->PresentWithImGui();
  }

  void *SoftwareRenderer::GetNativeHandle() const { return m_color.empty() ? nullptr : (void *)m_color.data(); }
  const char *SoftwareRenderer::GetName() const { return "Software rasterizer"; }

#pragma region rasterizer

  void SoftwareRenderer::BlendPixel(const int _x, const int _y, const Vec4f &_rgba, const bool _blend) noexcept {
    if (_x < 0 || _y < 0 || _x >= m_width || _y >= m_height) return;

    uint32_t &dst = m_color[static_cast<size_t>(_y) * static_cast<size_t>(m_width) + static_cast<size_t>(_x)];

    if (!_blend || _rgba.w >= 1.f) {
      dst = PackARGB(_rgba);
      return;
    }
    if (_rgba.w <= 0.f) return;

    // Straight (non-premultiplied) alpha, matching SDL_BLENDMODE_BLEND.
    const float a = _rgba.w;
    const float inv = 1.f - a;
    const float dr = static_cast<float>((dst >> 16) & 0xFF) / 255.f;
    const float dg = static_cast<float>((dst >> 8) & 0xFF) / 255.f;
    const float db = static_cast<float>(dst & 0xFF) / 255.f;
    const float da = static_cast<float>((dst >> 24) & 0xFF) / 255.f;

    dst = PackARGB({ _rgba.x * a + dr * inv, _rgba.y * a + dg * inv, _rgba.z * a + db * inv, a + da * inv });
  }

  void SoftwareRenderer::FillTriangle(const DrawVertex &_a, const DrawVertex &_b, const DrawVertex &_c,
                                      const bool _blend) noexcept {
    const DrawVertex *a = &_a;
    const DrawVertex *b = &_b;
    const DrawVertex *c = &_c;

    float area = Edge(a->position.x, a->position.y, b->position.x, b->position.y, c->position.x, c->position.y);
    if (area == 0.f) return; // degenerate

    // Normalise the winding so every edge function is positive on the inside. No backface
    // culling in this pass - that arrives with the depth buffer.
    if (area < 0.f) {
      const DrawVertex *swap = b;
      b = c;
      c = swap;
      area = -area;
    }

    /* The fill is confined to the triangle's bounding box, which is what makes tiled binning a
     * later intersection rather than a rewrite. */
    const float minXf = math::Min(a->position.x, math::Min(b->position.x, c->position.x));
    const float maxXf = math::Max(a->position.x, math::Max(b->position.x, c->position.x));
    const float minYf = math::Min(a->position.y, math::Min(b->position.y, c->position.y));
    const float maxYf = math::Max(a->position.y, math::Max(b->position.y, c->position.y));

    int minX = static_cast<int>(minXf);
    int maxX = static_cast<int>(maxXf) + 1;
    int minY = static_cast<int>(minYf);
    int maxY = static_cast<int>(maxYf) + 1;

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX > m_width) maxX = m_width;
    if (maxY > m_height) maxY = m_height;
    if (minX >= maxX || minY >= maxY) return;

    const bool topLeft0 = IsTopLeft(b->position, c->position);
    const bool topLeft1 = IsTopLeft(c->position, a->position);
    const bool topLeft2 = IsTopLeft(a->position, b->position);

    const float invArea = 1.f / area;

    for (int y = minY; y < maxY; ++y) {
      const float py = static_cast<float>(y) + 0.5f;
      for (int x = minX; x < maxX; ++x) {
        const float px = static_cast<float>(x) + 0.5f;

        const float e0 = Edge(b->position.x, b->position.y, c->position.x, c->position.y, px, py);
        const float e1 = Edge(c->position.x, c->position.y, a->position.x, a->position.y, px, py);
        const float e2 = Edge(a->position.x, a->position.y, b->position.x, b->position.y, px, py);

        if (!(topLeft0 ? e0 >= 0.f : e0 > 0.f)) continue;
        if (!(topLeft1 ? e1 >= 0.f : e1 > 0.f)) continue;
        if (!(topLeft2 ? e2 >= 0.f : e2 > 0.f)) continue;

        const float w0 = e0 * invArea;
        const float w1 = e1 * invArea;
        const float w2 = e2 * invArea;

        /* Screen-space linear, not perspective-correct: with no 1/w term this is only right for
         * geometry that is flat to the camera, which is all this pass claims to draw. */
        const Vec4f color { a->color.x * w0 + b->color.x * w1 + c->color.x * w2,
                            a->color.y * w0 + b->color.y * w1 + c->color.y * w2,
                            a->color.z * w0 + b->color.z * w1 + c->color.z * w2,
                            a->color.w * w0 + b->color.w * w1 + c->color.w * w2 };
        BlendPixel(x, y, color, _blend);
      }
    }
  }

  void SoftwareRenderer::RasterLine(const DrawVertex &_a, const DrawVertex &_b, const bool _blend) noexcept {
    const float dx = _b.position.x - _a.position.x;
    const float dy = _b.position.y - _a.position.y;
    const float span = math::Max(dx < 0.f ? -dx : dx, dy < 0.f ? -dy : dy);
    if (!(span > 0.f)) {
      BlendPixel(static_cast<int>(_a.position.x), static_cast<int>(_a.position.y), _a.color, _blend);
      return;
    }

    const int steps = static_cast<int>(span);
    const float invSteps = 1.f / span;
    const float stepX = dx * invSteps;
    const float stepY = dy * invSteps;

    float x = _a.position.x;
    float y = _a.position.y;
    for (int i = 0; i <= steps; ++i) {
      const float t = static_cast<float>(i) * invSteps;
      const Vec4f color { _a.color.x + (_b.color.x - _a.color.x) * t, _a.color.y + (_b.color.y - _a.color.y) * t,
                          _a.color.z + (_b.color.z - _a.color.z) * t, _a.color.w + (_b.color.w - _a.color.w) * t };
      BlendPixel(static_cast<int>(x), static_cast<int>(y), color, _blend);
      x += stepX;
      y += stepY;
    }
  }

  void SoftwareRenderer::RasterPoint(const DrawVertex &_v, const float _size, const bool _blend) noexcept {
    if (_size <= 1.f) {
      BlendPixel(static_cast<int>(_v.position.x), static_cast<int>(_v.position.y), _v.color, _blend);
      return;
    }

    const int half = static_cast<int>(_size * 0.5f);
    const int cx = static_cast<int>(_v.position.x);
    const int cy = static_cast<int>(_v.position.y);
    for (int y = cy - half; y <= cy + half; ++y)
      for (int x = cx - half; x <= cx + half; ++x)
        BlendPixel(x, y, _v.color, _blend);
  }

#pragma endregion

  void SoftwareRenderer::Draw(const DrawCommand &_cmd) {
    if (m_color.empty() || _cmd.vertexCount == 0) return;

    const bool screenSpace = (_cmd.flags & RENDERABLE_SCREEN_SPACE) != 0;
    const bool blend = (_cmd.flags & RENDERABLE_TRANSPARENT) != 0;

    m_scratch.clear();
    m_scratch.reserve(_cmd.vertexCount);

    const float halfW = static_cast<float>(m_width) * 0.5f;
    const float halfH = static_cast<float>(m_height) * 0.5f;

    if (screenSpace) {
      /* Screen-space positions are window pixels, but the framebuffer need not be window-sized.
       * Scaling by the ratio keeps a UI laid out against the window in the right relative place
       * once the internal resolution is decoupled from it. */
      const math::Vec2<int> output = AsPresenter(m_presenter)->OutputSize();
      const float sx = output.x > 0 ? static_cast<float>(m_width) / static_cast<float>(output.x) : 1.f;
      const float sy = output.y > 0 ? static_cast<float>(m_height) / static_cast<float>(output.y) : 1.f;

      for (size_t i = 0; i < _cmd.vertexCount; ++i) {
        DrawVertex v = _cmd.vertices[i];
        v.position = { v.position.x * sx, v.position.y * sy, 0.f };
        m_scratch.push_back(v);
      }
    } else {
      const Mat4f mvp = m_viewProjection * _cmd.transform;
      for (size_t i = 0; i < _cmd.vertexCount; ++i) {
        const DrawVertex &src = _cmd.vertices[i];
        const Vec4f clip = mvp * Vec4f { src.position.x, src.position.y, src.position.z, 1.f };

        DrawVertex out = src;
        if (clip.w > 0.f) {
          const float inv = 1.f / clip.w;
          out.position = { (clip.x * inv + 1.f) * halfW, (1.f - clip.y * inv) * halfH, clip.z * inv };
        } else {
          // No near-plane clipping yet; park it off-screen rather than let it wrap around.
          out.position = { -1.f, -1.f, 0.f };
        }
        m_scratch.push_back(out);
      }
    }

    const size_t indexCount = _cmd.indices ? _cmd.indexCount : m_scratch.size();
    const auto at = [&](const size_t _i) noexcept -> const DrawVertex & {
      return m_scratch[_cmd.indices ? _cmd.indices[_i] : _i];
    };

    switch (_cmd.topology) {
    case Topology::Triangles:
      for (size_t i = 0; i + 2 < indexCount; i += 3) FillTriangle(at(i), at(i + 1), at(i + 2), blend);
      break;

    case Topology::Lines:
      for (size_t i = 0; i + 1 < indexCount; i += 2) RasterLine(at(i), at(i + 1), blend);
      break;

    case Topology::Points:
      for (size_t i = 0; i < indexCount; ++i) RasterPoint(at(i), _cmd.pointSize, blend);
      break;
    }
  }

} // namespace ROSE

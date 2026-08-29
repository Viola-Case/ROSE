/**

  @file       gfx.cpp
  @brief      Backend-independent half of the render path: enrollment and the frame pass.
  @details    ~
  @author     Viola Case
  @date       19.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/Core/gfx.h>
#include <ROSE/Core/renderable.h>

#include <algorithm>

namespace ROSE {

#pragma region RenderList

  void RenderList::Add(const DrawCommand &_command) noexcept {
    /* An empty command is not an error - a renderable with nothing to show this frame says so by
     * emitting one, or by emitting nothing - but it must not reach a backend, which would have
     * to guard every Draw against a null vertex pointer. */
    if (_command.vertices == nullptr || _command.vertexCount == 0) return;

    m_sink->push_back(_command);
    ++m_added;
  }

  size_t RenderList::Size() const noexcept { return m_added; }
  bool RenderList::Empty() const noexcept { return m_added == 0; }

#pragma endregion

#pragma region RenderBackend

  RenderBackend::~RenderBackend() { DetachAllRenderables(); }

  void RenderBackend::DetachAllRenderables() noexcept {
    for (Renderable *r : m_renderables) {
      r->m_renderer = nullptr;
      r->m_slot = 0;
    }
    m_renderables.clear();
  }

  void RenderBackend::Enroll(Renderable &_renderable) noexcept {
    if (_renderable.m_renderer == this) return; // already ours; a replayed OnCreate lands here

    // Enrolled with someone else: leave cleanly before joining.
    if (_renderable.m_renderer) _renderable.m_renderer->Withdraw(_renderable);

    _renderable.m_renderer = this;
    _renderable.m_slot = m_renderables.size();
    m_renderables.push_back(&_renderable);
  }

  void RenderBackend::Withdraw(Renderable &_renderable) noexcept {
    if (_renderable.m_renderer != this) return;

    const size_t slot = _renderable.m_slot;
    if (slot < m_renderables.size() && m_renderables[slot] == &_renderable) {
      const size_t last = m_renderables.size() - 1;
      if (slot != last) {
        m_renderables[slot] = m_renderables[last];
        m_renderables[slot]->m_slot = slot;
      }
      m_renderables.pop_back();
    }

    _renderable.m_renderer = nullptr;
    _renderable.m_slot = 0;
  }

  void RenderBackend::SetViewProjection(const Mat4f &_viewProjection) noexcept {
    m_viewProjection = _viewProjection;
  }

  namespace {
    /*! Opaque first, then transparent, then overlay. See `RenderBackend::RenderFrame`. */
    constexpr uint32_t BandOf(const RenderableFlags _flags) noexcept {
      if (_flags & RENDERABLE_OVERLAY) return 2;
      if (_flags & RENDERABLE_TRANSPARENT) return 1;
      return 0;
    }
  } // namespace

  void RenderBackend::RenderFrame() {
    m_frameCommands.clear();
    m_frameOrder.clear();
    if (m_renderables.empty()) return;

    /* Collect everything before ordering anything. m_frameCommands reallocates as it grows, so
     * nothing may hold a pointer into it until the last Collect has run - hence indices in
     * SortEntry rather than pointers. */
    uint32_t sequence = 0;
    for (Renderable *r : m_renderables) {
      const uint32_t mine = sequence++;
      if (!r->IsEnabled()) continue;

      const size_t first = m_frameCommands.size();
      RenderList list { m_frameCommands };
      r->Collect(list);

      for (size_t i = first; i < m_frameCommands.size(); ++i)
        m_frameOrder.push_back(SortEntry { BandOf(m_frameCommands[i].flags), r->GetLayer(), mine,
                                           static_cast<uint32_t>(i) });
    }

    if (m_frameOrder.empty()) return;

    std::stable_sort(m_frameOrder.begin(), m_frameOrder.end(),
                     [](const SortEntry &_a, const SortEntry &_b) noexcept {
                       if (_a.band != _b.band) return _a.band < _b.band;
                       if (_a.layer != _b.layer) return _a.layer < _b.layer;
                       return _a.sequence < _b.sequence;
                     });

    /* m_frameCommands is not touched again, so every pointer a command carries stays valid for
     * the whole loop - a backend is free to defer past its own Draw call. */
    for (const SortEntry &entry : m_frameOrder)
      Draw(m_frameCommands[entry.index]);
  }

#pragma endregion

} // namespace ROSE

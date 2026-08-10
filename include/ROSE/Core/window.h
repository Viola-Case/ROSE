/**

  @file       window.h
  @brief
  @details    ~
  @author     Viola Case
  @date       04.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#pragma once

#include <cstdint>
#include <ROSE/Core/macros.h>
#include <ROSE/Core/math.h>

namespace ROSE {

  /*!
   * Owning, move-only wrapper around a platform window. The handle is opaque so that no
   * windowing headers leak into the public API; cast it at the use site.
   *
   * A `Window` is only ever produced by `Create`, and a failed creation yields an invalid
   * `Window` rather than throwing - always check `IsValid()`.
   *
   * @todo The flags passed to `Create` are the raw backend bitmask. Define ROSE-side window
   *       flags and translate them in the implementation.
   */
  class ROSE_API(Core) Window final {
  private:
    Window() noexcept = default;

  public:
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window &&) noexcept;
    Window &operator=(Window &&) noexcept;

    ~Window() noexcept;

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] void *GetHandle() const noexcept; //!< `SDL_Window *`, cast at the use site

    [[nodiscard]] math::Vec2<int> GetSize() const noexcept;     //!< cached; no backend call
    [[nodiscard]] math::Vec2<int> GetPosition() const noexcept; //!< cached; no backend call

    void SetSize(math::Vec2<int> _size) noexcept;         //!< pushes to the backend, updates the cache
    void SetPosition(math::Vec2<int> _position) noexcept; //!< pushes to the backend, updates the cache
    void SetTitle(const char *_title) noexcept;

    void Show() noexcept;
    void Hide() noexcept;

    /* Fed by the event loop once the backend reports the change; these update the cache only
     * and never push back, otherwise handling a resize event would provoke another one. */
    void OnResized(math::Vec2<int> _size) noexcept;
    void OnMoved(math::Vec2<int> _position) noexcept;

    static Window Create(const char *_title, math::Vec2<int> _size, uint64_t _flags) noexcept;

  private:
    void *m_handle { nullptr };
    math::Vec2<int> m_size { 800, 600 };
    math::Vec2<int> m_position {};
  };
} // namespace ROSE

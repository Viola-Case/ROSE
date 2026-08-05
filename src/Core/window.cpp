/**

  @file       window.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       04.08.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/Core/window.h>
#include <ROSE/Core/log.h>
#include <ROSE/Core/utility.h>
#include <SDL3/SDL.h>

namespace ROSE {

  Window::Window(Window &&rval) noexcept : m_handle(rval.m_handle),
                                           m_size(rval.m_size),
                                           m_position(rval.m_position) {
    rval.m_handle = nullptr;
  }

  Window &Window::operator=(Window &&rval) noexcept {
    if (this == &rval) return *this;

    if (m_handle) SDL_DestroyWindow(static_cast<SDL_Window *>(m_handle));

    m_handle = rval.m_handle;
    m_size = rval.m_size;
    m_position = rval.m_position;
    rval.m_handle = nullptr;

    return *this;
  }

  Window::~Window() noexcept {
    if (m_handle) SDL_DestroyWindow(static_cast<SDL_Window *>(m_handle));
    m_handle = nullptr;
  }

  bool Window::IsValid() const noexcept { return m_handle != nullptr; }

  void *Window::GetHandle() const noexcept { return m_handle; }

  math::Vec2<int> Window::GetSize() const noexcept { return m_size; }

  math::Vec2<int> Window::GetPosition() const noexcept { return m_position; }

  void Window::SetSize(const math::Vec2<int> _size) noexcept {
    if (!m_handle) return;
    if (!SDL_SetWindowSize(static_cast<SDL_Window *>(m_handle), _size.x, _size.y)) {
      ROSE_LOG_WARN("Window resize failed: {}\n", SDL_GetError());
      return;
    }
    m_size = _size;
  }

  void Window::SetPosition(const math::Vec2<int> _position) noexcept {
    if (!m_handle) return;
    if (!SDL_SetWindowPosition(static_cast<SDL_Window *>(m_handle), _position.x, _position.y)) {
      ROSE_LOG_WARN("Window move failed: {}\n", SDL_GetError());
      return;
    }
    m_position = _position;
  }

  void Window::SetTitle(const char *_title) noexcept {
    if (!m_handle || !_title) return;
    SDL_SetWindowTitle(static_cast<SDL_Window *>(m_handle), _title);
  }

  void Window::Show() noexcept {
    if (m_handle) SDL_ShowWindow(static_cast<SDL_Window *>(m_handle));
  }

  void Window::Hide() noexcept {
    if (m_handle) SDL_HideWindow(static_cast<SDL_Window *>(m_handle));
  }

  void Window::OnResized(const math::Vec2<int> _size) noexcept { m_size = _size; }

  void Window::OnMoved(const math::Vec2<int> _position) noexcept { m_position = _position; }

  Window Window::Create(const char *_title, const math::Vec2<int> _size, const uint64_t _flags) noexcept {
    Window window;

    SDL_Window *handle = SDL_CreateWindow(_title, _size.x, _size.y, static_cast<SDL_WindowFlags>(_flags));
    if (!handle) {
      ROSE_LOG_ERROR("Window creation failed: {}\n", SDL_GetError());
      return Move(window);
    }

    window.m_handle = handle;

    /* Ask the backend rather than trusting the request: the window manager is free to hand back
     * something other than what we asked for, and the position was never ours to pick. */
    SDL_GetWindowSize(handle, &window.m_size.x, &window.m_size.y);
    SDL_GetWindowPosition(handle, &window.m_position.x, &window.m_position.y);

    return Move(window);
  }
} // namespace ROSE

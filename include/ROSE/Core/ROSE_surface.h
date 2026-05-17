/**

  @file       ROSE_surface.h
  @brief
  @details    ~
  @author     Viola Case
  @date       07.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#pragma once

#include <cstdint>

namespace ROSE {

  struct PixelFormat {
    enum Value : uint16_t {
      Unknown = 0,
      RGBA32,
      RGB24,
      BGRA32,
      ARGB32,
      RGBA64,
      Unsupported,
    } value;
    constexpr PixelFormat(Value v) noexcept : value(v) {}
    constexpr PixelFormat(uint16_t v) noexcept : value(static_cast<Value>(v)) {}
    constexpr operator uint16_t() const noexcept { return value; }
  };

  class Surface {
  public:
    Surface(const Surface &) = delete;
    Surface &operator=(const Surface &) = delete;
    Surface(Surface &&) noexcept;
    Surface &operator=(Surface &&) noexcept;

    ~Surface() noexcept;

    uint16_t GetWidth() const noexcept;
    uint16_t GetHeight() const noexcept;
    uint16_t GetPitch() const noexcept;
    PixelFormat GetFormat() const noexcept;

    bool IsValid() const noexcept;

    static Surface LoadImage(const char *path) noexcept;
    static Surface LoadAsset(const char *assetId) noexcept;

  private:
    void *m_ptr;
    uint16_t m_width, m_height, m_pitch;
    PixelFormat m_format;
  };
} // namespace ROSE
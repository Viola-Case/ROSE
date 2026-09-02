/**

  @file       surface.h
  @brief
  @details    ~
  @author     Viola Case
  @date       07.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#pragma once

#include <ROSE/Core/api.h>
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
    constexpr PixelFormat() = default;
    constexpr PixelFormat(Value v) noexcept : value(v) {}
    constexpr PixelFormat(uint16_t v) noexcept : value(static_cast<Value>(v)) {}
    constexpr operator uint16_t() const noexcept { return value; }
  };

  class ROSE_API(CORE) Surface {
  private:
    Surface() noexcept = default;
  public:
    Surface(const Surface &) = delete;
    Surface &operator=(const Surface &) = delete;
    Surface(Surface &&) noexcept;
    Surface &operator=(Surface &&) noexcept;

    ~Surface() noexcept;

    uint16_t GetWidth() const noexcept;
    uint16_t GetHeight() const noexcept;
    uint16_t GetPitch() const noexcept; //!< bytes per row, which need not be width * bytesPerPixel
    PixelFormat GetFormat() const noexcept;

    bool IsValid() const noexcept;

    /*!
     * The pixels, row-major, `GetPitch()` bytes apart. Null when the surface is invalid.
     *
     * What a software rasterizer samples and what an uploader reads. Interpreting them needs
     * `GetFormat()`; `ConvertTo` exists so a caller can stop caring.
     */
    const void *GetPixels() const noexcept;

    /*!
     * The underlying `SDL_Surface *`, as an opaque pointer so this header stays SDL-free.
     * For the SDL backends, which have something better to do with it than copy it out.
     */
    void *GetNativeHandle() const noexcept;

    /*!
     * Re-encode in place. A no-op when the format already matches.
     *
     * @retval false if the conversion failed, in which case the surface is left as it was.
     */
    bool ConvertTo(PixelFormat) noexcept;

    static Surface LoadImage(const char *path) noexcept;
    static Surface LoadAsset(const char *assetId) noexcept;

  private:
    void *m_ptr { nullptr }; //!< `SDL_Surface *`; null is a valid, invalid surface
    // uint16_t m_width, m_height, m_pitch;
    PixelFormat m_format { PixelFormat::Unknown };
  };
} // namespace ROSE
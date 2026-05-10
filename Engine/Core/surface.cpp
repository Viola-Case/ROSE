/**

  @file       surface.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       07.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/Core/ROSE_buffer.h>

#include <ROSE/Core/ROSE_surface.h>
#include <SDL3_image/SDL_image.h>

namespace ROSE {

  PixelFormat FormatFromSDLPixelFormat(const SDL_PixelFormat format) noexcept {
    switch (format) {
    case SDL_PIXELFORMAT_RGBA32:
      return PixelFormat::RGBA32;
    case SDL_PIXELFORMAT_RGB24:
      return PixelFormat::RGB24;
    case SDL_PIXELFORMAT_BGRA32:
      return PixelFormat::BGRA32;
    case SDL_PIXELFORMAT_ARGB32:
      return PixelFormat::ARGB32;
    case SDL_PIXELFORMAT_RGBA64:
      return PixelFormat::RGBA64;
    default:
      return PixelFormat::Unsupported;
    }
  }

  struct SurfaceSourceType {
    enum Value : uint8_t {
      SDLImage,
      PackagedAsset,
      Procedural,
      Buffer
    } value;
    constexpr SurfaceSourceType(Value v) : value(v) {}
    constexpr SurfaceSourceType(uint8_t v) noexcept : value(static_cast<Value>(v)) {}
    constexpr operator uint8_t() const noexcept { return static_cast<uint8_t>(value); }
  };

  struct SurfaceContainer {
    void *data;
    SurfaceSourceType sourceType;

    SurfaceContainer(void *data, SurfaceSourceType sourceType) noexcept : data(data),
                                                                          sourceType(sourceType) {}
    SurfaceContainer(SDL_Surface *surface) noexcept : data(surface),
                                                      sourceType(SurfaceSourceType::SDLImage) {}

    ~SurfaceContainer() {
      switch (sourceType) {
      case SurfaceSourceType::SDLImage:
        SDL_DestroySurface(static_cast<SDL_Surface *>(data));
        break;
      case SurfaceSourceType::PackagedAsset:
        // idk
        break;
      case SurfaceSourceType::Procedural:
        // idk
        break;
      case SurfaceSourceType::Buffer:
        delete static_cast<RawBuffer *>(data);
        break;
      default:
        break;
      }
    }
  };


  Surface::Surface(Surface &&rval) noexcept : m_ptr(rval.m_ptr),
                                              m_width(rval.m_width),
                                              m_height(rval.m_height),
                                              m_format(rval.m_format),
                                              m_pitch(rval.m_pitch) {
    rval.m_ptr = nullptr;
  }
  Surface &Surface::operator=(Surface &&rval) noexcept {
    if (m_ptr == rval.m_ptr) {
      return *this;
    }

    this->~Surface();

    m_ptr = rval.m_ptr;
    m_width = rval.m_width;
    m_height = rval.m_height;
    m_pitch = rval.m_pitch;
    m_format = rval.m_format;
    rval.m_ptr = nullptr;

    return *this;
  }

  Surface::~Surface() noexcept {
    delete static_cast<SurfaceContainer *>(m_ptr);
    m_ptr = nullptr;
  }

  uint16_t Surface::GetWidth() const noexcept {
    return m_width;
  }
  uint16_t Surface::GetHeight() const noexcept {
    return m_height;
  }
  uint16_t Surface::GetPitch() const noexcept {
    return m_pitch;
  }
  PixelFormat Surface::GetFormat() const noexcept {
    return m_format;
  }

  bool Surface::IsValid() const noexcept {
    return m_ptr != nullptr;
  }

  Surface Surface::LoadImage(const char *path) noexcept {
    Surface surface;
    SDL_Surface *surf = IMG_Load(path);
    if (surf == nullptr) {
      /// TODO log error
      surface.m_ptr = nullptr;
      surface.m_pitch = surface.m_height = surface.m_width = 0;
      surface.m_format = PixelFormat::Unknown;
      return surface;
    }

    surface.m_ptr = new SurfaceContainer(surf, SurfaceSourceType::SDLImage);
    surface.m_pitch = surf->pitch;
    surface.m_width = surf->w;
    surface.m_height = surf->h;

    surface.m_format = FormatFromSDLPixelFormat(surf->format);
    if (surface.m_format == PixelFormat::Unsupported) {
      /// TODO log error ("Unsupported pixel format: {}", SDL_GetPixelFormatName(surf->format));
    }
    return surface;
  }

  Surface Surface::LoadAsset(const char *assetId) noexcept {
    Surface surface{};
    // Asset asset(assetId);
    // if (!asset.data) {
    //   /// TODO log error ("Asset failed to load: {}", assetId);
    //   surface.m_ptr = nullptr;
    //   surface.m_pitch = surface.m_height = surface.m_width = 0;
    //   surface.m_format = PixelFormat::Unknown;
    //   return surface;
    //
    // }
    return surface;
  }


} // namespace ROSE
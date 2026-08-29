/**

  @file       surface.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       07.05.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/Core/buffer.h>
#include <ROSE/Core/surface.h>
#include <ROSE/Core/utility.h>
#include <SDL3_image/SDL_image.h>


namespace ROSE {

  inline SDL_PixelFormat SDLPixelFormatFromPixelFormat(const PixelFormat format) noexcept {
    switch (format.value) {
    case PixelFormat::RGBA32:
      return SDL_PIXELFORMAT_RGBA32;
    case PixelFormat::RGB24:
      return SDL_PIXELFORMAT_RGB24;
    case PixelFormat::BGRA32:
      return SDL_PIXELFORMAT_BGRA32;
    case PixelFormat::ARGB32:
      return SDL_PIXELFORMAT_ARGB32;
    case PixelFormat::RGBA64:
      return SDL_PIXELFORMAT_RGBA64;
    default:
      return SDL_PIXELFORMAT_UNKNOWN;
    }
  }

  inline PixelFormat PixelFormatFromSDLPixelFormat(const SDL_PixelFormat format) noexcept {
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

  /**
   * @todo ditch this
   */
  struct SurfaceSourceType {
    enum Value : uint8_t { SDLImage, PackagedAsset, Procedural, Buffer } value;
    constexpr SurfaceSourceType(Value v) : value(v) {}
    constexpr SurfaceSourceType(uint8_t v) noexcept : value(static_cast<Value>(v)) {}
    constexpr operator uint8_t() const noexcept { return static_cast<uint8_t>(value); }
  };


  /**
   * @todo ditch this
   */
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
                                              m_format(rval.m_format){
    rval.m_ptr = nullptr;
  }
  Surface &Surface::operator=(Surface &&rval) noexcept {
    if (m_ptr == rval.m_ptr) {
      return *this;
    }

    Swap(m_ptr, rval.m_ptr);
    m_format = rval.m_format;
    rval.m_ptr = nullptr;

    return *this;
  }

  Surface::~Surface() noexcept {
    //delete static_cast<SurfaceContainer *>(m_ptr);
    SDL_DestroySurface(static_cast<SDL_Surface *>(m_ptr));
    m_ptr = nullptr;
  }

  /* All four guard on m_ptr: an invalid Surface is an ordinary outcome - a missing file, an
   * unimplemented LoadAsset - and asking it for its size should answer zero, not crash. */
  uint16_t Surface::GetWidth() const noexcept {
    return m_ptr ? static_cast<uint16_t>(static_cast<SDL_Surface *>(m_ptr)->w) : 0;
  }
  uint16_t Surface::GetHeight() const noexcept {
    return m_ptr ? static_cast<uint16_t>(static_cast<SDL_Surface *>(m_ptr)->h) : 0;
  }
  uint16_t Surface::GetPitch() const noexcept {
    return m_ptr ? static_cast<uint16_t>(static_cast<SDL_Surface *>(m_ptr)->pitch) : 0;
  }
  PixelFormat Surface::GetFormat() const noexcept {
    return m_format;
  }

  const void *Surface::GetPixels() const noexcept {
    return m_ptr ? static_cast<SDL_Surface *>(m_ptr)->pixels : nullptr;
  }

  void *Surface::GetNativeHandle() const noexcept { return m_ptr; }

  bool Surface::ConvertTo(const PixelFormat format) noexcept {
    if (!m_ptr) return false;
    if (m_format.value == format.value) return true;

    const SDL_PixelFormat target = SDLPixelFormatFromPixelFormat(format);
    if (target == SDL_PIXELFORMAT_UNKNOWN) return false;

    SDL_Surface *converted = SDL_ConvertSurface(static_cast<SDL_Surface *>(m_ptr), target);
    if (!converted) return false;

    SDL_DestroySurface(static_cast<SDL_Surface *>(m_ptr));
    m_ptr = converted;
    m_format = format;
    return true;
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
      surface.m_format = PixelFormat::Unknown;
      return Move(surface);
    }

    surface.m_ptr = surf;

    surface.m_format = PixelFormatFromSDLPixelFormat(surf->format);
    if (surface.m_format == PixelFormat::Unsupported) {
      /// TODO log error ("Unsupported pixel format: {}", SDL_GetPixelFormatName(surf->format));
    }
    return Move(surface);
  }

  Surface Surface::LoadAsset(const char *assetId) noexcept {
    Surface surface;
    // Asset asset(assetId);
    // if (!asset.data) {
    //   /// TODO log error ("Asset failed to load: {}", assetId);
    //   surface.m_ptr = nullptr;
    //   surface.m_pitch = surface.m_height = surface.m_width = 0;
    //   surface.m_format = PixelFormat::Unknown;
    //   return surface;
    //
    // }
    return Move(surface);
  }


} // namespace ROSE
/**

  @file      camera.h
  @brief
  @details   ~
  @author    Viola Case
  @date      08.04.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#pragma once
#include <ROSE/Core/api.h>
#include <ROSE/Core/behavior.h>
#include <ROSE/Core/math.h>


namespace ROSE {
  class ROSE_API(CORE) Camera : public Behavior {
  public:
    static constexpr UUID typeID = "98b16c050e659798-2ba97b3cd1a9dd7c"_uuid;
    static constexpr UUID TypeID() { return typeID; }
    UUID GetTypeID() const noexcept override { return TypeID(); }

    /*!
     * The world-to-clip matrix, which is what a render backend wants and the only thing it asks
     * a camera for.
     *
     * @param _aspect viewport width / height. Passed in rather than stored because the camera
     *                does not own the window and the window can be resized between frames.
     */
    [[nodiscard]] Mat4f GetViewProjection(float _aspect) const noexcept;

    //! World-to-view: the inverse of the owning object's transform. Identity when unattached.
    [[nodiscard]] Mat4f GetView() const noexcept;
    [[nodiscard]] Mat4f GetProjection(float _aspect) const noexcept;

    void SetFocalLength(const float _millimeters) noexcept { m_focalLength = _millimeters; }
    [[nodiscard]] float GetFocalLength() const noexcept { return m_focalLength; }

    void SetOrthographic(const bool _orthographic) noexcept { m_orthographic = _orthographic; }
    [[nodiscard]] bool IsOrthographic() const noexcept { return m_orthographic; }

    //! Half the visible height, in world units. Orthographic cameras only.
    void SetOrthographicSize(const float _size) noexcept { m_orthographicSize = _size; }
    [[nodiscard]] float GetOrthographicSize() const noexcept { return m_orthographicSize; }

    void SetClipPlanes(const float _near, const float _far) noexcept {
      m_near = _near;
      m_far = _far;
    }

  protected:
    math::Vec2<int16_t> m_aspectRatio; //!< I should probably make this just a single float
    // [[bounds({ 0, inf })]] <--- I really need to figure out what the hell im gonna do with these attributes because
    // there ain't no clear answer
    float m_focalLength { 30 }; //!< millimeters, against a 36x24mm frame
    bool m_orthographic { false };
    float m_near { 0.1f };
    float m_far { 1000.f };
    float m_orthographicSize { 10.f };

    void Unpack(const ParamView &view) override;
  };
} // namespace ROSE

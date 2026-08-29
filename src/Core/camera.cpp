/**

  @file       camera.cpp
  @brief
  @details    ~
  @author     Viola Case
  @date       15.07.2026
  @copyright  © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>

#include <cmath>

namespace ROSE {

  namespace {
    /*! The vertical half-frame of 35mm film, in millimetres. Focal length only means a field of
     *  view relative to a frame size, and 36x24 is the one everybody quotes lenses against. */
    constexpr float kSensorHalfHeight = 12.f;
  } // namespace

  void Camera::Unpack(const ParamView &view) {
    m_focalLength = static_cast<float>(view.GetDouble("focalLength", 30.0));
    m_orthographic = view.GetBool("orthographic", false);
    m_orthographicSize = static_cast<float>(view.GetDouble("orthographicSize", 10.0));
    m_near = static_cast<float>(view.GetDouble("near", 0.1));
    m_far = static_cast<float>(view.GetDouble("far", 1000.0));
    // The aspect ratio is not read: it belongs to the viewport, and GetProjection takes it.
  }

  Mat4f Camera::GetView() const noexcept {
    if (!m_object) return Mat4f::Identity();

    const Transform &t = m_object->transform;

    /* The inverse of the camera's world transform. For a rigid transform that is the conjugate
     * rotation composed with the negated translation, which avoids a general 4x4 inversion.
     * Scale is deliberately ignored - a scaled camera is a mistake, not a feature. */
    const Quatf conjugate { static_cast<float>(t.rotation.w), -static_cast<float>(t.rotation.x),
                            -static_cast<float>(t.rotation.y), -static_cast<float>(t.rotation.z) };
    const Vec3f negated { -static_cast<float>(t.position.x), -static_cast<float>(t.position.y),
                          -static_cast<float>(t.position.z) };

    return conjugate.ToMat4() * Mat4f::Translation(negated);
  }

  Mat4f Camera::GetProjection(const float _aspect) const noexcept {
    const float aspect = _aspect > 0.f ? _aspect : 1.f;

    if (m_orthographic) {
      const float halfHeight = m_orthographicSize;
      const float halfWidth = halfHeight * aspect;
      return math::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, m_near, m_far);
    }

    /* No Atan in the math library yet, and this runs once a frame at most, so std:: is the
     * honest choice over a Taylor series nobody asked for. */
    const float focal = m_focalLength > 0.f ? m_focalLength : 1.f;
    const float fovY = 2.f * std::atan(kSensorHalfHeight / focal);

    return math::Perspective(fovY, aspect, m_near, m_far);
  }

  Mat4f Camera::GetViewProjection(const float _aspect) const noexcept {
    return GetProjection(_aspect) * GetView();
  }

} // namespace ROSE

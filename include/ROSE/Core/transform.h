/**

    @file      transform.h
    @brief
    @details   ~
    @author    Viola Case
    @date      24.02.2026
    @copyright © Viola Case, 2026. All rights reserved.

**/
#pragma once

#include <ROSE/Core/math.h>

namespace ROSE {
  /*!
   * Transform struct for 6dof position/rotation in Euclidean 3D space
   */
  struct Transform {
    Vec3d position;
    Quatd rotation;
    Vec3d scale;

  /*!
   * Translates the transform in 3D Euclidian space
   */
    void Translate(const Vec3d &translation);
  /*!
   * Rotates the transform centered at a specific origin
   */
    void RotateAroundPoint(const Quatd &rot, const Vec3d &origin);
  /*!
   * Rotates the transform centered at scene origin
   */
    void Rotate(const Quatd &rot) {
      RotateAroundPoint(rot, { 0, 0, 0 });
    }
  /*!
   * Scales a transform centered at a specific origin
   */
    void ScaleAroundPoint(const Vec3d &scl, const Vec3d &origin);
  /*!
   * Scales a transform centered at the scene origin
   */
    void Scale(const Vec3d &scl) {
      ScaleAroundPoint(scl, { 0, 0, 0 });
    }
  };

  void TranslateTransform(Transform &transform, Vec3d translation) noexcept;

  void RotateTransformAroundPoint(Transform &transform, Vec3d origin, Quatd rotation) noexcept;

  inline void RotateTransform(Transform &transform, Quatd rotation) noexcept {
    RotateTransformAroundPoint(transform, { 0, 0, 0 }, rotation);
  }

  void ScaleTransformAroundPoint(Transform &transform, Vec3d origin, Vec3d scale) noexcept;

  inline void ScaleTransform(Transform &transform, Vec3d scale) noexcept {
    ScaleTransformAroundPoint(transform, { 0, 0, 0 }, scale);
  }

  
} // namespace ROSE
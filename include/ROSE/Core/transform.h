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
#include <ROSE/Core/macros.h>

namespace ROSE {
  /*!
   * Transform struct for 6dof position/rotation in Euclidean 3D space
   */
  /* Defined inline rather than exported from ROSE_Core. The DLL boundary is there to keep global state single -
   * `BehaviorFactory::get()`, `Time`'s storage and friends - and Transform holds none of it, so exporting it wrapped a
   * single vector add in an import thunk and bought nothing. Its layout is part of the ABI either way, since Object
   * embeds one by value, so consumers already recompile when it changes. Inline also lets these be constexpr, which an
   * out-of-line definition cannot be. */
  struct Transform {

    alignas(16) Vec3d position;
    alignas(16) Quatd rotation;
    alignas(16) Vec3d scale;

    constexpr Transform(const Vec3d &p = { 0, 0, 0 }, const Quatd &q = { 1, 0, 0, 0 },
                        const Vec3d &s = { 1, 1, 1 }) noexcept
      : position(p), rotation(q), scale(s) {}
  /*!
   * Translates the transform in 3D Euclidian space
   */
    constexpr void Translate(const Vec3d &translation) noexcept { position += translation; }
  /*!
   * Rotates the transform centered at a specific origin
   */
    constexpr void RotateAroundPoint(const Quatd &rot, const Vec3d &origin) noexcept {
      /* Rotates the offset from origin by the standard unit-quaternion sandwich, expanded so it needs no conjugate and
       * no matrix: v' = v + 2w(qv X v) + 2(qv X (qv X v)), with qv the vector part. `rot` is a world-space rotation, so
       * it composes on the left of the existing orientation. Both quaternions are normalized - the formula only holds
       * for unit quaternions, and accumulated products drift off the unit sphere. */
      const Quatd q = rot.Normalized();
      const Vec3d qv { q.x, q.y, q.z };

      const Vec3d offset = position - origin;
      const Vec3d t = qv.cross(offset) * 2.;

      position = origin + offset + t * q.w + qv.cross(t);
      rotation = (q * rotation).Normalized();
    }
  /*!
   * Rotates the transform centered at scene origin
   */
    constexpr void Rotate(const Quatd &rot) noexcept {
      RotateAroundPoint(rot, { 0, 0, 0 });
    }
  /*!
   * Scales a transform centered at a specific origin
   */
    constexpr void ScaleAroundPoint(const Vec3d &scl, const Vec3d &origin) noexcept {
      /* Indexed rather than named reads so this folds: Vec's constructors activate the `data` member of its union, and
       * constant evaluation will not read the inactive one. */
      for (size_t i = 0; i < 3; ++i) {
        scale[i] *= scl[i];
        position[i] += (position[i] - origin[i]) * (scl[i] - 1.);
      }
    }
  /*!
   * Scales a transform centered at the scene origin
   * @todo decide if this needs to be deleted since scene origin is arbitrary AF
   */
    constexpr void Scale(const Vec3d &scl) noexcept {
      ScaleAroundPoint(scl, { 0, 0, 0 });
    }
  };

  
} // namespace ROSE
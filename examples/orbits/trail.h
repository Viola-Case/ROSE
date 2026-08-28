/**

    @file      trail.h
    @brief     Fixed-capacity position history for the point cloud.
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#pragma once

#include <ROSE/Core/list.h>
#include <cstdint>

namespace Orbits {
  using ROSE::List;

  //! Layout-compatible with `SDL_FPoint`; asserted where the cast is made.
  struct Point {
    float x, y;
  };

  /*!
   * The last N positions of every body, held in one allocation.
   *
   * Every trail advances on the same frame, so the whole cloud shares one write cursor
   * instead of carrying a cursor - and a heap block - per trail: a frame is a "column"
   * across the buffer. Storage is trail-major (`trail * Length() + slot`) because the
   * access that runs every frame is the geometry build, which walks one trail end to
   * end; writing a column strides, but that is `TrailCount()` stores against
   * `TrailCount() * Length()` reads.
   *
   * Nothing allocates after `Reset`, and recording a sample costs one store and one
   * wrap rather than shifting the whole history down by one.
   */
  class TrailBuffer {
  public:
    void Reset(uint32_t trails, uint32_t length);
    void Clear() noexcept;

    /*!
     * Opens a new column. Every trail should be written before the next `Advance`;
     * one that is skipped keeps whatever the sample it evicted held.
     */
    void Advance() noexcept;
    void Write(uint32_t trail, Point sample) noexcept;

    [[nodiscard]] uint32_t TrailCount() const noexcept { return m_trails; }
    [[nodiscard]] uint32_t Length() const noexcept { return m_length; }

    //! Columns recorded so far, capped at `Length()`. Below it only while the ring fills.
    [[nodiscard]] uint32_t Filled() const noexcept { return m_filled; }

    //! Sample `age` of `trail`, oldest first, so `Filled() - 1` is the current position.
    [[nodiscard]] Point At(uint32_t trail, uint32_t age) const noexcept;

  private:
    List<Point> m_samples;
    uint32_t m_trails { 0 };
    uint32_t m_length { 0 };
    uint32_t m_filled { 0 };
    uint32_t m_head { 0 }; //!< slot the current column occupies
  };
} // namespace Orbits

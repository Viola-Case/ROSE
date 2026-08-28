/**

    @file      trail.cpp
    @brief
    @details   ~
    @author    Viola Case
    @date      27.08.2026
    @copyright © Viola Case, 2026. All right reserved.

**/

#include "trail.h"

namespace Orbits {
  void TrailBuffer::Reset(uint32_t trails, uint32_t length) {
    m_trails = trails;
    m_length = length;
    m_samples.clear();
    m_samples.resize(static_cast<size_t>(trails) * length);
    Clear();
  }

  void TrailBuffer::Clear() noexcept {
    m_filled = 0;
    // The next Advance wraps this round to zero, so the first column lands at the front of the ring.
    m_head = m_length ? m_length - 1 : 0;
  }

  void TrailBuffer::Advance() noexcept {
    if (!m_length) return;
    m_head = m_head + 1 == m_length ? 0 : m_head + 1;
    if (m_filled < m_length) ++m_filled;
  }

  void TrailBuffer::Write(uint32_t trail, Point sample) noexcept {
    if (trail >= m_trails || !m_length) return;
    m_samples[static_cast<size_t>(trail) * m_length + m_head] = sample;
  }

  Point TrailBuffer::At(uint32_t trail, uint32_t age) const noexcept {
    /* `age` counts up from the oldest live sample while the cursor sits on the newest, so walk
     * back from the cursor by however many samples separate the two. */
    const uint32_t back = m_filled - 1 - age;
    const uint32_t slot = m_head >= back ? m_head - back : m_head + m_length - back;
    return m_samples[static_cast<size_t>(trail) * m_length + slot];
  }
} // namespace Orbits

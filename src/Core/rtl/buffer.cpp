/**

  @file      buffer.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      16.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/

#include <ROSE/ROSE.h>
#include <cstring>

namespace ROSE {
  RawBuffer::RawBuffer() noexcept : m_data(nullptr), m_size(0) {}
  RawBuffer::RawBuffer(size_t bytes) : m_data(nullptr), m_size(0) {
    if (!bytes) return;
    m_data = ::operator new(bytes);
    m_size = bytes;
  }
  RawBuffer::RawBuffer(RawBuffer &&other) noexcept : m_data(other.m_data), m_size(other.m_size) {
    other.m_data = nullptr;
    other.m_size = 0;
  }

  RawBuffer &RawBuffer::operator=(RawBuffer &&other) noexcept {
    if (this == &other)
      return *this;
    ::operator delete(m_data);
    m_data = other.m_data;
    m_size = other.m_size;
    other.m_data = nullptr;
    other.m_size = 0;
    return *this;
  }

  RawBuffer::~RawBuffer() {
    ::operator delete(m_data);
  }

  void *RawBuffer::data() noexcept { return m_data; }
  const void *RawBuffer::data() const noexcept { return m_data; }
  size_t RawBuffer::size_bytes() const noexcept { return m_size; }

  void RawBuffer::allocate(size_t bytes) {
    if (bytes == 0) {
      free();
      return;
    }

    ::operator delete(m_data);

    m_data = ::operator new(bytes);
    m_size = bytes;
  }
  void RawBuffer::reallocate(size_t bytes) {
    if (bytes == m_size)
      return;

    if (bytes == 0) {
      free();
      return;
    }

    void *new_data = ::operator new(bytes);

    if (m_data) {
      const size_t copy_size = (bytes < m_size) ? bytes : m_size;
      std::memcpy(new_data, m_data, copy_size);
      ::operator delete(m_data);
    }

    m_data = new_data;
    m_size = bytes;
  }
  void RawBuffer::free() {
    ::operator delete(m_data);
    m_data = nullptr;
    m_size = 0;
  }

}
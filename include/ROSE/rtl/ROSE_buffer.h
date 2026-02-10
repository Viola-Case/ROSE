/**

    @file      ROSE_buffer.h
    @brief     
    @details   ~
    @author    Cool Guy
    @date      9.02.2026
    @copyright © Cool Guy, 2026. All right reserved.

**/
#pragma once

namespace ROSE {
  class RawBuffer {
  public:
    RawBuffer() noexcept;
    explicit RawBuffer(size_t bytes);
    RawBuffer(const RawBuffer &) = delete;
    RawBuffer &operator=(const RawBuffer &) = delete;

    RawBuffer(RawBuffer &&other) noexcept;
    RawBuffer &operator=(RawBuffer &&other) noexcept;

    ~RawBuffer();

    void allocate(size_t bytes);
    void reallocate(size_t bytes);
    void free();

    void *data() noexcept;
    const void *data() const noexcept;

    size_t size() const noexcept;
  private:
    void *m_data;
    size_t m_size;
  };

}
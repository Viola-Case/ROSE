/**

    @file      ROSE_buffer.h
    @brief     RawBuffer — RAII owner of an untyped heap allocation
    @details   RawBuffer wraps a single malloc/free-style allocation. It is
               non-copyable and move-only; used as the backing storage for
               List and HashMap. Prefer List or a typed container over direct
               use of RawBuffer.
    @author    Viola Case
    @date      9.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once



namespace ROSE {
  /**
    @class   RawBuffer
    @brief   RAII owner of a single untyped heap block.
    @details Move-only. The destructor always frees the managed allocation.
             Provides allocate(), reallocate(), and free() for lifecycle
             management and data()/size_bytes() for access.
  **/
  class RawBuffer {
  public:
    /**
      @brief   Constructs an empty buffer with no allocation.
    **/
    RawBuffer() noexcept;

    /**
      @brief   Allocates a buffer of the given size immediately.
      @param   bytes  Number of bytes to allocate.
    **/
    explicit RawBuffer(size_t bytes);
    RawBuffer(const RawBuffer &) = delete;
    RawBuffer &operator=(const RawBuffer &) = delete;

    /**
      @brief   Move-constructs from another RawBuffer, taking its allocation.
               The source is left empty.
    **/
    RawBuffer(RawBuffer &&other) noexcept;

    /**
      @brief   Move-assigns from another RawBuffer.
               The current allocation is freed before taking ownership of the source.
    **/
    RawBuffer &operator=(RawBuffer &&other) noexcept;

    /**
      @brief   Destructor; frees the managed allocation if one exists.
    **/
    ~RawBuffer();

    /**
      @brief   Frees any existing allocation and allocates a new block of the given size.
      @param   bytes  Number of bytes for the new allocation.
    **/
    void allocate(size_t bytes);

    /**
      @brief   Resizes the allocation, copying existing bytes up to the new size.
               Equivalent to realloc: existing data is preserved up to min(old, new) bytes.
      @param   bytes  New allocation size in bytes.
    **/
    void reallocate(size_t bytes);

    /**
      @brief   Releases the managed allocation and resets the buffer to empty.
    **/
    void free();

    /**
      @brief   Returns a mutable pointer to the allocated memory.
      @retval  Pointer to the first byte, or nullptr if empty.
    **/
    void *data() noexcept;

    /**
      @brief   Returns a read-only pointer to the allocated memory.
      @retval  Pointer to the first byte, or nullptr if empty.
    **/
    const void *data() const noexcept;

    /**
      @brief   Returns the current allocation size in bytes.
      @retval  0 if the buffer is empty.
    **/
    size_t size_bytes() const noexcept;
  private:
    void *m_data;
    size_t m_size;
  };

}

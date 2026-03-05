/**

  @file      ROSE_list.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      4.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/preamble/ROSE_stdlib.h>
#include <ROSE/rtl/ROSE_array.h>
#include <ROSE/rtl/ROSE_utility.h>
#include <ROSE/rtl/ROSE_buffer.h>

namespace ROSE {

  /**

      @class   List
      @brief   
      @details ~
      @tparam  T - 

  **/
  template<typename T>
  class List {
  public:

    using value_type = T;
    using size_type = size_t;

    // -------------------------
    // Constructors
    // -------------------------

    List() noexcept = default;

    explicit List(size_t initial_capacity) {
      reserve(initial_capacity);
    }

    List(const List &other) {
      reserve(other.m_count);

      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(data(), other.data(), other.m_count * sizeof(T));
        m_count = other.m_count;
      } else {
        for (size_t i = 0; i < other.m_count; ++i)
          new (data() + i) T(other.data()[i]);

        m_count = other.m_count;
      }
    }

    List(size_type count, const T &value) {
      reserve(count);

      for (size_type i = 0; i < count; ++i)
        new (data() + i) T(value);

      m_count = count;
    }


    List(std::initializer_list<T> init) {
      reserve(init.size());

      size_type i = 0;
      for (const T &value : init)
        new (data() + i++) T(value);

      m_count = init.size();
    }


    List(List &&other) noexcept
      : m_buffer(Move(other.m_buffer)),
      m_count(other.m_count) {
      other.m_count = 0;
    }


    template<size_t N>
    List(const FixedArray<T, N> &arr) {
      reserve(N);

      for (size_t i = 0; i < N; ++i)
        new (data() + i) T(arr[i]);

      m_count = N;
    }


    List &operator=(const List &other) {
      if (this == &other)
        return *this;

      clear();
      reserve(other.m_count);

      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(data(), other.data(), other.m_count * sizeof(T));
        m_count = other.m_count;
      } else {
        for (size_t i = 0; i < other.m_count; ++i)
          new (data() + i) T(other.data()[i]);

        m_count = other.m_count;
      }

      return *this;
    }

    List &operator=(List &&other) noexcept {
      if (this == &other)
        return *this;

      clear();
      m_buffer = std::move(other.m_buffer);
      m_count = other.m_count;

      other.m_count = 0;
      return *this;
    }

    ~List() {
      clear();
    }

    // -------------------------
    // Capacity
    // -------------------------

    constexpr size_t size() const noexcept {
      return m_count;
    }

    constexpr size_t capacity() const noexcept {
      return m_buffer.size_bytes() / sizeof(T);
    }

    constexpr bool empty() const noexcept {
      return m_count == 0;
    }

    void reserve(size_t new_capacity) {
      if (new_capacity <= capacity())
        return;

      reallocate(new_capacity);
    }

    void resize(size_t new_size) {
      if (new_size < m_count) {
        destroy_range(new_size, m_count);
        m_count = new_size;
      } else if (new_size > m_count) {
        reserve(new_size);
        for (size_t i = m_count; i < new_size; ++i)
          new (data() + i) T();
        m_count = new_size;
      }
    }

    void clear() noexcept {
      destroy_range(0, m_count);
      m_count = 0;
    }

    // -------------------------
    // Element Access
    // -------------------------

    constexpr T &operator[](size_t index) noexcept {
      return data()[index];
    }

    constexpr const T &operator[](size_t index) const noexcept {
      return data()[index];
    }

    T &back() noexcept {
      return data()[m_count - 1];
    }

    const T &back() const noexcept {
      return data()[m_count - 1];
    }

    constexpr T *begin() noexcept { return data(); }
    constexpr T *end()   noexcept { return data() + m_count; }

    constexpr const T *begin() const noexcept { return data(); }
    constexpr const T *end()   const noexcept { return data() + m_count; }

    // -------------------------
    // Modifiers
    // -------------------------

    void push_back(const T &value) {
      ensure_capacity_for_one();
      new (data() + m_count) T(value);
      ++m_count;
    }

    void push_back(T &&value) {
      ensure_capacity_for_one();
      new (data() + m_count) T(std::move(value));
      ++m_count;
    }

    template<typename... Args>
    T &emplace_back(Args&&... args) {
      ensure_capacity_for_one();
      new (data() + m_count) T(std::forward<Args>(args)...);
      return data()[m_count++];
    }

    void pop_back() {
      if (m_count == 0)
        return;

      --m_count;
      data()[m_count].~T();
    }


    T *data() noexcept {
      return reinterpret_cast<T *>(m_buffer.data());
    }

    const T *data() const noexcept {
      return reinterpret_cast<const T *>(m_buffer.data());
    }

  private:

    RawBuffer m_buffer;
    size_t m_count = 0;

    void ensure_capacity_for_one() {
      if (m_count < capacity())
        return;

      size_t new_cap = capacity() ? capacity() * 2 : 4;
      reallocate(new_cap);
    }

    void reallocate(size_t new_capacity) {
      RawBuffer new_buffer(new_capacity * sizeof(T));
      T *new_data = reinterpret_cast<T *>(new_buffer.data());
      T *old_data = data();

      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(new_data, old_data, m_count * sizeof(T));
      } else {
        for (size_t i = 0; i < m_count; ++i) {
          new (new_data + i) T(std::move_if_noexcept(old_data[i]));
          old_data[i].~T();
        }
      }

      m_buffer = std::move(new_buffer);
    }

    void destroy_range(size_t from, size_t to) noexcept {
      if constexpr (!std::is_trivially_destructible_v<T>) {
        T *d = data();
        for (size_t i = from; i < to; ++i)
          d[i].~T();
      }
    }
  };
}
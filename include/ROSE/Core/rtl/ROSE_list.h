/**

  @file      ROSE_list.h
  @brief     List<T> — contiguous dynamic array (ROSE equivalent of std::vector)
  @details   List uses a RawBuffer as its backing store and grows by doubling.
             Trivially-copyable elements are copied with memcpy; non-trivial
             elements are move-constructed during reallocation and properly
             destroyed on clear/resize/destruction.
  @author    Viola Case
  @date      4.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/ROSE_stdlib.h>
#include <ROSE/Core/rtl/ROSE_array.h>
#include <ROSE/Core/rtl/ROSE_utility.h>
#include <ROSE/Core/rtl/ROSE_buffer.h>

namespace ROSE {

  /**

      @class   List
      @brief   Contiguous dynamic array, analogous to std::vector.
      @details Backed by a RawBuffer; capacity doubles on overflow.
               Elements are stored in-place; non-trivially-destructible
               elements are properly destroyed on clear/resize/destruction.
      @tparam  T - element type (must be move-constructible)

  **/
  template<typename T>
  class List {
  public:

    using value_type = T;
    using size_type = size_t;

    // -------------------------
    // Constructors
    // -------------------------

    /// @brief Default constructor; creates an empty list with no allocation.
    List() noexcept = default;

    /**
      @brief   Constructs an empty list with space pre-reserved for at least initial_capacity elements.
      @param   initial_capacity  Number of elements to reserve space for upfront.
    **/
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

    /**
      @brief   Constructs a list with count copies of value.
      @param   count  Number of elements to create.
      @param   value  Value to copy into each element.
    **/
    List(size_type count, const T &value) {
      reserve(count);

      for (size_type i = 0; i < count; ++i)
        new (data() + i) T(value);

      m_count = count;
    }

    /**
      @brief   Constructs a list from a brace-initializer list.
    **/
    List(std::initializer_list<T> init) {
      reserve(init.size());

      size_type i = 0;
      for (const T &value : init)
        new (data() + i++) T(value);

      m_count = init.size();
    }

    /**
      @brief   Move-constructs from another list, taking its backing storage.
               The source is left empty.
    **/
    List(List &&other) noexcept
      : m_buffer(Move(other.m_buffer)),
      m_count(other.m_count) {
      other.m_count = 0;
    }

    /**
      @brief   Constructs a list from a fixed-size array, copying all N elements.
    **/
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

    /// @brief Number of elements currently stored.
    constexpr size_t size() const noexcept {
      return m_count;
    }

    /// @brief Number of elements that can be stored without reallocation.
    constexpr size_t capacity() const noexcept {
      return m_buffer.size_bytes() / sizeof(T);
    }

    /// @brief Returns true if the list contains no elements.
    constexpr bool empty() const noexcept {
      return m_count == 0;
    }

    /**
      @brief   Ensures the list can hold at least new_capacity elements without reallocation.
               Does nothing if current capacity is already sufficient.
      @param   new_capacity  Minimum desired capacity.
    **/
    void reserve(size_t new_capacity) {
      if (new_capacity <= capacity())
        return;

      reallocate(new_capacity);
    }

    /**
      @brief   Resizes the list to new_size elements.
               New elements are default-constructed; excess elements are destroyed.
      @param   new_size  Target element count.
    **/
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

    /// @brief Destroys all elements and sets the size to zero. Capacity is retained.
    void clear() noexcept {
      destroy_range(0, m_count);
      m_count = 0;
    }

    // -------------------------
    // Element Access
    // -------------------------

    /// @brief Returns a reference to the element at index (no bounds check).
    constexpr T &operator[](size_t index) noexcept {
      return data()[index];
    }

    /// @brief Returns a const reference to the element at index (no bounds check).
    constexpr const T &operator[](size_t index) const noexcept {
      return data()[index];
    }

    /// @brief Returns a reference to the last element. Undefined if empty.
    T &back() noexcept {
      return data()[m_count - 1];
    }

    /// @brief Returns a const reference to the last element. Undefined if empty.
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

    /**
      @brief   Appends a copy of value to the end of the list.
               Reallocates if capacity is exhausted.
    **/
    void push_back(const T &value) {
      ensure_capacity_for_one();
      new (data() + m_count) T(value);
      ++m_count;
    }

    /**
      @brief   Moves value to the end of the list.
               Reallocates if capacity is exhausted.
    **/
    void push_back(T &&value) {
      ensure_capacity_for_one();
      new (data() + m_count) T(std::move(value));
      ++m_count;
    }

    /**
      @brief   Constructs a new element in-place at the end of the list.
      @tparam  Args  Constructor argument types (deduced).
      @param   args  Arguments forwarded to T's constructor.
      @retval  Reference to the newly constructed element.
    **/
    template<typename... Args>
    T &emplace_back(Args&&... args) {
      ensure_capacity_for_one();
      new (data() + m_count) T(std::forward<Args>(args)...);
      return data()[m_count++];
    }

    /**
      @brief   Destroys the last element. No-op if the list is empty.
    **/
    void pop_back() {
      if (m_count == 0)
        return;

      --m_count;
      data()[m_count].~T();
    }

    /**
      @brief   Returns a mutable pointer to the first element (or nullptr if empty).
    **/
    T *data() noexcept {
      return reinterpret_cast<T *>(m_buffer.data());
    }

    /**
      @brief   Returns a read-only pointer to the first element (or nullptr if empty).
    **/
    const T *data() const noexcept {
      return reinterpret_cast<const T *>(m_buffer.data());
    }

    const bool operator ==(const List &rhs) {
      return m_count == rhs.m_count && memcmp(m_buffer.data(), rhs.m_buffer.data(), m_count) == 0;
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

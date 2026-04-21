/**

    @file      ROSE_memory.h
    @brief     Smart pointer primitives: UniquePtr, SharedPtr, WeakPtr.
    @details   ~
    @author    Viola Case
    @date      9.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <cstddef>
#include <type_traits>
#include <ROSE/Core/rtl/ROSE_utility.h>

namespace ROSE {

  /**

      @class   UniquePtr
      @brief   Owning smart pointer with exclusive ownership semantics.
      @tparam  T - pointed-to type

  **/
  template<typename T>
  class UniquePtr {
  public:

    // -------------------------
    // Constructors
    // -------------------------

    constexpr UniquePtr() noexcept = default;
    constexpr UniquePtr(std::nullptr_t) noexcept {}

    explicit UniquePtr(T *_p) noexcept : m_ptr(_p) {}

    UniquePtr(const UniquePtr &) = delete;
    UniquePtr &operator=(const UniquePtr &) = delete;

    UniquePtr(UniquePtr &&_other) noexcept : m_ptr(_other.m_ptr) {
      _other.m_ptr = nullptr;
    }

    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    UniquePtr(UniquePtr<U> &&_other) noexcept : m_ptr(_other.release()) {}

    ~UniquePtr() { delete m_ptr; }

    // -------------------------
    // Assignment
    // -------------------------

    UniquePtr &operator=(UniquePtr &&_other) noexcept {
      if (this != &_other) {
        T *old = m_ptr;
        m_ptr = _other.m_ptr;
        _other.m_ptr = nullptr;
        delete old;
      }
      return *this;
    }

    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    UniquePtr &operator=(UniquePtr<U> &&_other) noexcept {
      T *old = m_ptr;
      m_ptr = _other.release();
      delete old;
      return *this;
    }

    UniquePtr &operator=(std::nullptr_t) noexcept {
      reset();
      return *this;
    }

    // -------------------------
    // Access
    // -------------------------

    /**
      @warning dont call `delete` on this
      @retval  raw pointer to the managed object (may be nullptr)
    **/
    [[nodiscard]] T *get() noexcept { return m_ptr; }
    /**
      @warning dont call `delete` on this
      @retval  raw pointer to the managed object (may be nullptr)
    **/
    [[nodiscard]] const T *get() const noexcept { return m_ptr; }

    T &operator*() const noexcept { return *m_ptr; }
    T *operator->() const noexcept { return m_ptr; }

    explicit operator bool() const noexcept { return m_ptr != nullptr; }

    // -------------------------
    // Modifiers
    // -------------------------

    /**
      @brief   Destroys the currently managed object and optionally adopts a new one.
      @param   _p  New raw pointer to manage (defaults to nullptr, leaving the pointer empty).
    **/
    void reset(T *_p = nullptr) noexcept {
      T *old = m_ptr;
      m_ptr = _p;
      delete old;
    }

    /**
      @brief   Releases ownership of the managed object without destroying it.
      @retval  Raw pointer to the previously managed object; caller is now responsible for deletion.
    **/
    [[nodiscard]] T *release() noexcept {
      T *tmp = m_ptr;
      m_ptr = nullptr;
      return tmp;
    }

    /**
      @brief   Swaps ownership with another UniquePtr in O(1).
      @param   _other  UniquePtr to exchange ownership with.
    **/
    void swap(UniquePtr &_other) noexcept {
      T *tmp = m_ptr;
      m_ptr = _other.m_ptr;
      _other.m_ptr = tmp;
    }

  private:
    T *m_ptr{ nullptr };
  };

  // -------------------------
  // UniquePtr free functions / comparisons
  // -------------------------

  template<typename T, typename U>
  constexpr bool operator==(const UniquePtr<T> &_a, const UniquePtr<U> &_b) noexcept {
    return _a.get() == _b.get();
  }
  template<typename T, typename U>
  constexpr bool operator!=(const UniquePtr<T> &_a, const UniquePtr<U> &_b) noexcept {
    return _a.get() != _b.get();
  }
  template<typename T>
  constexpr bool operator==(const UniquePtr<T> &_a, std::nullptr_t) noexcept {
    return _a.get() == nullptr;
  }
  template<typename T>
  constexpr bool operator==(std::nullptr_t, const UniquePtr<T> &_a) noexcept {
    return _a.get() == nullptr;
  }
  template<typename T>
  constexpr bool operator!=(const UniquePtr<T> &_a, std::nullptr_t) noexcept {
    return _a.get() != nullptr;
  }
  template<typename T>
  constexpr bool operator!=(std::nullptr_t, const UniquePtr<T> &_a) noexcept {
    return _a.get() != nullptr;
  }

  template<typename T>
  void Swap(UniquePtr<T> &_a, UniquePtr<T> &_b) noexcept {
    _a.swap(_b);
  }

  /**
    @brief   Allocates a new T constructed from _args and wraps it in a UniquePtr.
    @tparam  T     Type to allocate.
    @tparam  Args  Constructor argument types (deduced).
    @param   _args Arguments forwarded to T's constructor.
    @retval  UniquePtr<T> owning the newly allocated object.
  **/
  template<typename T, typename ...Args>
  [[nodiscard]] UniquePtr<T> MakeUnique(Args&&... _args) {
    return UniquePtr<T>(new T(Forward<Args>(_args)...));
  }

  template<typename T>
  class WeakPtr;

  /**

      @class   SharedPtr
      @brief   Reference-counted owning smart pointer.
      @tparam  T - pointed-to type

  **/
  template<typename T>
  class SharedPtr {
  private:
    template<typename U> friend class WeakPtr;

    struct ControlBlock {
      size_t strong_count{ 1 };
      size_t weak_count{ 0 };
      T *m_ptr;

      explicit ControlBlock(T *_p) noexcept : m_ptr(_p) {}
    };

    ControlBlock *m_ctrl{ nullptr };

    void decrement() noexcept {
      if (!m_ctrl) return;

      if (--m_ctrl->strong_count == 0) {
        delete m_ctrl->m_ptr;
        m_ctrl->m_ptr = nullptr;
        if (m_ctrl->weak_count == 0) {
          delete m_ctrl;
        }
      }
      m_ctrl = nullptr;
    }

  public:

    // -------------------------
    // Constructors
    // -------------------------

    constexpr SharedPtr() noexcept = default;
    constexpr SharedPtr(std::nullptr_t) noexcept {}

    explicit SharedPtr(T *_p) {
      if (_p) m_ctrl = new ControlBlock(_p);
    }

    SharedPtr(const SharedPtr &_other) noexcept : m_ctrl(_other.m_ctrl) {
      if (m_ctrl) ++m_ctrl->strong_count;
    }

    SharedPtr(SharedPtr &&_other) noexcept : m_ctrl(_other.m_ctrl) {
      _other.m_ctrl = nullptr;
    }

    ~SharedPtr() { decrement(); }

    // -------------------------
    // Assignment
    // -------------------------

    SharedPtr &operator=(const SharedPtr &_other) noexcept {
      if (this != &_other) {
        decrement();
        m_ctrl = _other.m_ctrl;
        if (m_ctrl) ++m_ctrl->strong_count;
      }
      return *this;
    }

    SharedPtr &operator=(SharedPtr &&_other) noexcept {
      if (this != &_other) {
        decrement();
        m_ctrl = _other.m_ctrl;
        _other.m_ctrl = nullptr;
      }
      return *this;
    }

    SharedPtr &operator=(std::nullptr_t) noexcept {
      decrement();
      return *this;
    }

    // -------------------------
    // Access
    // -------------------------

    /**
        @warning dont call `delete` on this
        @retval  pointer to managed object (may be nullptr)
    **/
    [[nodiscard]] T *get() const noexcept { return m_ctrl ? m_ctrl->m_ptr : nullptr; }
    T &operator*() const noexcept { return *m_ctrl->m_ptr; }
    T *operator->() const noexcept { return m_ctrl->m_ptr; }

    /**
      @brief   Returns the number of SharedPtr instances sharing this object.
      @retval  0 if this SharedPtr is empty.
    **/
    [[nodiscard]] size_t use_count() const noexcept {
      return m_ctrl ? m_ctrl->strong_count : 0;
    }

    /**
      @brief   Returns true if this is the sole SharedPtr owning the managed object.
    **/
    [[nodiscard]] bool unique() const noexcept {
      return use_count() == 1;
    }

    explicit operator bool() const noexcept { return m_ctrl && m_ctrl->m_ptr; }

    // -------------------------
    // Modifiers
    // -------------------------

    /**
      @brief   Releases ownership of the managed object (decrements ref-count).
               Leaves this SharedPtr empty.
    **/
    void reset() noexcept { decrement(); }

    /**
      @brief   Releases the current object and adopts a new raw pointer.
      @param   _p  New raw pointer to manage.
    **/
    void reset(T *_p) {
      decrement();
      if (_p) m_ctrl = new ControlBlock(_p);
    }

    /**
      @brief   Swaps ownership with another SharedPtr in O(1).
      @param   _other  SharedPtr to exchange with.
    **/
    void swap(SharedPtr &_other) noexcept {
      ControlBlock *tmp = m_ctrl;
      m_ctrl = _other.m_ctrl;
      _other.m_ctrl = tmp;
    }

  };

  // -------------------------
  // SharedPtr free functions / comparisons
  // -------------------------

  template<typename T, typename U>
  constexpr bool operator==(const SharedPtr<T> &_a, const SharedPtr<U> &_b) noexcept {
    return _a.get() == _b.get();
  }
  template<typename T, typename U>
  constexpr bool operator!=(const SharedPtr<T> &_a, const SharedPtr<U> &_b) noexcept {
    return _a.get() != _b.get();
  }
  template<typename T>
  constexpr bool operator==(const SharedPtr<T> &_a, std::nullptr_t) noexcept {
    return _a.get() == nullptr;
  }
  template<typename T>
  constexpr bool operator==(std::nullptr_t, const SharedPtr<T> &_a) noexcept {
    return _a.get() == nullptr;
  }
  template<typename T>
  constexpr bool operator!=(const SharedPtr<T> &_a, std::nullptr_t) noexcept {
    return _a.get() != nullptr;
  }
  template<typename T>
  constexpr bool operator!=(std::nullptr_t, const SharedPtr<T> &_a) noexcept {
    return _a.get() != nullptr;
  }

  template<typename T>
  void Swap(SharedPtr<T> &_a, SharedPtr<T> &_b) noexcept {
    _a.swap(_b);
  }

  /**
    @brief   Allocates a new T constructed from _args and wraps it in a SharedPtr.
    @tparam  T     Type to allocate.
    @tparam  Args  Constructor argument types (deduced).
    @param   _args Arguments forwarded to T's constructor.
    @retval  SharedPtr<T> with a ref-count of 1.
  **/
  template<typename T, typename... Args>
  [[nodiscard]] SharedPtr<T> MakeShared(Args&&... _args) {
    return SharedPtr<T>(new T(Forward<Args>(_args)...));
  }

  /**

      @class   WeakPtr
      @brief   Non-owning observer of a SharedPtr-managed object.
      @tparam  T - pointed-to type

  **/
  template<typename T>
  class WeakPtr {
  private:
    using ControlBlock = typename SharedPtr<T>::ControlBlock;

    ControlBlock *m_ctrl{ nullptr };

    void decrement() noexcept {
      if (!m_ctrl) return;

      --m_ctrl->weak_count;
      if (m_ctrl->weak_count == 0 && m_ctrl->strong_count == 0) {
        delete m_ctrl;
      }
      m_ctrl = nullptr;
    }

  public:

    // -------------------------
    // Constructors
    // -------------------------

    constexpr WeakPtr() noexcept = default;

    WeakPtr(const SharedPtr<T> &_sp) noexcept : m_ctrl(_sp.m_ctrl) {
      if (m_ctrl) ++m_ctrl->weak_count;
    }

    WeakPtr(const WeakPtr &_other) noexcept : m_ctrl(_other.m_ctrl) {
      if (m_ctrl) ++m_ctrl->weak_count;
    }

    WeakPtr(WeakPtr &&_other) noexcept : m_ctrl(_other.m_ctrl) {
      _other.m_ctrl = nullptr;
    }

    ~WeakPtr() { decrement(); }

    // -------------------------
    // Assignment
    // -------------------------

    WeakPtr &operator=(const WeakPtr &_other) noexcept {
      if (this != &_other) {
        decrement();
        m_ctrl = _other.m_ctrl;
        if (m_ctrl) ++m_ctrl->weak_count;
      }
      return *this;
    }

    WeakPtr &operator=(WeakPtr &&_other) noexcept {
      if (this != &_other) {
        decrement();
        m_ctrl = _other.m_ctrl;
        _other.m_ctrl = nullptr;
      }
      return *this;
    }

    WeakPtr &operator=(const SharedPtr<T> &_sp) noexcept {
      decrement();
      m_ctrl = _sp.m_ctrl;
      if (m_ctrl) ++m_ctrl->weak_count;
      return *this;
    }

    // -------------------------
    // Observers
    // -------------------------

    /**
      @brief   Returns true if the SharedPtr-managed object has been destroyed.
    **/
    [[nodiscard]] bool expired() const noexcept {
      return !m_ctrl || m_ctrl->strong_count == 0;
    }

    /**
      @brief   Returns the number of active SharedPtr instances owning the object.
      @retval  0 if expired or empty.
    **/
    [[nodiscard]] size_t use_count() const noexcept {
      return m_ctrl ? m_ctrl->strong_count : 0;
    }

    /**
      @brief   Attempts to acquire a SharedPtr from this WeakPtr.
      @retval  A valid SharedPtr if the object is still alive; an empty SharedPtr if expired.
    **/
    [[nodiscard]] SharedPtr<T> lock() const noexcept {
      if (expired()) return SharedPtr<T>();
      SharedPtr<T> sp;
      sp.m_ctrl = m_ctrl;
      ++m_ctrl->strong_count;
      return sp;
    }

    // -------------------------
    // Modifiers
    // -------------------------

    /**
      @brief   Releases this weak reference.
    **/
    void reset() noexcept { decrement(); }

    /**
      @brief   Swaps this weak reference with another in O(1).
    **/
    void swap(WeakPtr &_other) noexcept {
      ControlBlock *tmp = m_ctrl;
      m_ctrl = _other.m_ctrl;
      _other.m_ctrl = tmp;
    }

  };

  template<typename T>
  void Swap(WeakPtr<T> &_a, WeakPtr<T> &_b) noexcept {
    _a.swap(_b);
  }

}

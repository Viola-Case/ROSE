/**

    @file      ROSE_memory.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      9.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <utility>
#include <ROSE/Core/rtl/ROSE_utility.h>

namespace ROSE {

  template<typename T>
  class UniquePtr {
  public:
    explicit UniquePtr(T *p_ = nullptr) : m_ptr(p_) {}

    ~UniquePtr() { delete m_ptr; }

    UniquePtr(const UniquePtr &) = delete;
    UniquePtr &operator=(const UniquePtr &) = delete;

    UniquePtr(UniquePtr &&other) noexcept : m_ptr(other.m_ptr) {
      other.m_ptr = nullptr;
    }

    template<typename U, typename = std::enable_if_t<std::is_convertible<U *, T *>::value>>
    UniquePtr(UniquePtr<U> &&other) noexcept : m_ptr(other.release()) {}

    /**
      @warning dont call `delete` on this
      @retval
    **/
    T *get() noexcept { return m_ptr; }
    /**
      @warning dont call `delete` on this
      @retval
    **/
    const T *get() const noexcept { return m_ptr; }

    T &operator*() const { return *m_ptr; }
    T *operator->() const { return m_ptr; }

    explicit operator bool() const { return m_ptr != nullptr; }

    void reset(T *p = nullptr) {
      delete m_ptr;
      m_ptr = p;
    }

    T *release() {
      T *tmp = m_ptr;
      m_ptr = nullptr;
      return tmp;
    }

  private:
    T *m_ptr;
  };

  template<typename T, typename ...Args>
  UniquePtr<T> MakeUnique(Args&&... args) {
    return UniquePtr<T>(new T(Forward<Args>(args)...));
  }

  template<typename T>
  class WeakPtr;

  template<typename T>
  class SharedPtr {
  private:
    struct ControlBlock {
      size_t strong_count{ 1 };
      size_t weak_count{ 0 };

      explicit ControlBlock(T *p) : m_ptr(p) {}

      ~ControlBlock() { delete m_ptr; }

      T *m_ptr;
    };

    ControlBlock *ctrl{ nullptr };

    void release() {
      if (ctrl) {
        if (--ctrl->strong_count == 0) {
          delete ctrl->m_ptr;
          ctrl->m_ptr = nullptr;
          if (ctrl->weak_count == 0) {
            delete ctrl;
          }
        }
        ctrl = nullptr;
      }
    }

    template<typename U> friend class WeakPtr;

  public:

    SharedPtr() = default;

    explicit SharedPtr(T *p_) {
      if (p_) ctrl = new ControlBlock(p_);
    }

    SharedPtr(const SharedPtr &other) noexcept : ctrl(other.ctrl) {
      if (ctrl) ++ctrl->strong_count;
    }

    SharedPtr(SharedPtr &&other) noexcept : ctrl(other.ctrl) {
      other.ctrl = nullptr;
    }

    ~SharedPtr() { release(); }

    SharedPtr &operator=(const SharedPtr &other) noexcept {
      if (this != &other) {
        release();
        ctrl = other.ctrl;
        if (ctrl) ++ctrl->strong_count;
      }
      return *this;
    }

    SharedPtr &operator=(SharedPtr &&other) noexcept {
      if (this != &other) {
        release();
        ctrl = other.ctrl;
        other.ctrl = nullptr;
      }
      return *this;
    }
    /**
        @warning dont call `delete` on this
        @retval  pointer to managed object
    **/
    T *get() const noexcept { return ctrl ? ctrl->m_ptr : nullptr; }
    T &operator*() const noexcept { return *ctrl->m_ptr; }
    T *operator->() const noexcept { return ctrl->m_ptr; }

    size_t use_count() const noexcept {
      return ctrl ? ctrl->strong_count : 0;
    }

    explicit operator bool() const noexcept { return ctrl && ctrl->m_ptr; }

  };

  template<typename T, typename... Args>
  SharedPtr<T> make_shared(Args&&... args) {
    T *obj = new T(std::forward<Args>(args)...);
    return SharedPtr<T>(obj);
  }

  template<typename T>
  class WeakPtr {
  private:
    typename SharedPtr<T>::ControlBlock *ctrl{ nullptr };

    void release() {
      if (ctrl) {
        if (--ctrl->weak_count == 0 && ctrl->strong_count == 0) {
          delete ctrl;
        }
        ctrl = nullptr;
      }
    }

  public:
    WeakPtr() = default;
    WeakPtr(const SharedPtr<T> &sp) noexcept : ctrl(sp.ctrl) {
      if (ctrl) ++ctrl->weak_count;
    }
    WeakPtr(const WeakPtr &other) noexcept : ctrl(other.ctrl) {
      if (ctrl) ++ctrl->weak_count;
    }
    WeakPtr(WeakPtr &&other) noexcept : ctrl(other.ctrl) {
      other.ctrl = nullptr;
    }
    ~WeakPtr() { release(); }

    WeakPtr &operator=(const WeakPtr &other) noexcept {
      if (this != &other) {
        release();
        ctrl = other.ctrl;
        if (ctrl) ++ctrl->weak_count;
      }
      return *this;
    }

    WeakPtr &operator=(WeakPtr &&other) noexcept {
      if (this != &other) {
        release();
        ctrl = other.ctrl;
        other.ctrl = nullptr;
      }
      return *this;
    }

    bool expired() const noexcept { return !ctrl || ctrl->strong_count == 0; }

    SharedPtr<T> lock() const noexcept {
      if (expired()) return SharedPtr<T>();
      SharedPtr<T> sp;
      sp.ctrl = ctrl;
      ++ctrl->strong_count;
      return sp;
    }

  };
}
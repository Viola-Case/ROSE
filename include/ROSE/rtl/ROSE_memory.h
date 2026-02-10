/**

    @file      ROSE_memory.h
    @brief     
    @details   ~
    @author    Cool Guy
    @date      9.02.2026
    @copyright © Cool Guy, 2026. All right reserved.

**/
#pragma once

#include <utility>

namespace ROSE {
  template<typename T>
  class UniquePtr {
  public:
    explicit UniquePtr(T *p_ = nullptr) : ptr(p_) {}

    ~UniquePtr() { delete ptr; }

    UniquePtr(const UniquePtr &) = delete;
    UniquePtr &operator=(const UniquePtr &) = delete;

    UniquePtr(UniquePtr &&other) noexcept {
      if (this != &other) {
        delete ptr;
        ptr = other.ptr;
        other.ptr = nullptr;
      }
      return *this;
    }

    template<typename U, typename = std::enable_if_t<std::is_convertible<U *, T *>::value>>
    UniquePtr(UniquePtr<U> &&other) noexcept : ptr(other.release()) {}

    /**
      @warning dont call `delete` on this
      @retval
    **/
    T *get() noexcept { return ptr };
    /**
      @warning dont call `delete` on this
      @retval
    **/
    const T *get() const noexcept { return ptr };

    T &operator*() const { return *ptr; }
    T *operator->() const { return ptr; }

    explicit operator bool() const { return ptr != nullptr; }

    void reset(T *p = nullptr) {
      delete ptr;
      ptr = p;
    }

    T *release() {
      T *tmp = ptr;
      ptr = nullptr;
      return tmp;
    }

  private:
    T *ptr;
  };

  template<typename T, typename ...Args>
  UniquePtr<T> MakeUnique(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
  }

  template<typename T>
  class WeakPtr;

  template<typename T>
  class SharedPtr {
  private:
    struct ControlBlock {
      size_t strong_count{ 1 };
      size_t weak_count{ 0 };

      explicit ControlBlock(T *p) : ptr(p) {}

      ~ControlBlock() { delete ptr; }

      T *ptr;
    };

    ControlBlock *ctrl{ nullptr };

    void release() {
      if (ctrl) {
        if (--ctrl->strong_count == 0) {
          delete ctrl->ptr;
          ctrl->ptr = nullptr;
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
      if (p_) ctrl = new ControlBlock(p);
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
    T *get() const noexcept { return ctrl ? ctrl->ptr : nullptr; }
    T &operator*() const noexcept { return *ctrl->ptr; }
    T *operator*() const noexcept { return ctrl->ptr; }

    size_t use_count() const noexcept {
      return ctrl ? ctrl->strong_count : 0;
    }

    explicit operator bool() const noexcept { return ctrl && ctrl->ptr; }

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


  };
}
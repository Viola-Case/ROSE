/**

  @file      string.h
  @brief
  @details   ~
  @author    Viola Case
  @date      6.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/typetraits.h>
#include <ROSE/Core/utility.h>
#include <ROSE/Core/list.h>

namespace ROSE {

  template <Character CharT>
  class BasicStringView;

  // TODO: SBO in BasicString — do before plugin ABI freezes.
  template <Character CharT>
  class BasicString {
  private:
    template <Character U>
    friend class BasicStringView;

    CharT *m_data;
    size_t m_size;
    size_t m_capacity;

    static CharT *allocate(const size_t count) { return static_cast<CharT *>(::operator new(count * sizeof(CharT))); }
    static void deallocate(CharT *ptr) noexcept { ::operator delete(ptr); }
    // Copy `count` characters from `src` into `dst` (no null terminator added)
    static void copy(CharT *dst, const CharT *src, const size_t count) noexcept {
      for (size_t i = 0; i < count; ++i)
        dst[i] = src[i];
    }

  public:
    using value_type = CharT;

    BasicString() noexcept : m_data(nullptr), m_size(0), m_capacity(0) {}

    BasicString(const CharT *str) {
      m_size = StrLen(str);
      m_capacity = m_size + 1;
      m_data = allocate(m_capacity);
      if (str) copy(m_data, str, m_size);
      m_data[m_size] = CharT(0);
    }
    BasicString(const BasicString &other) : m_size(other.m_size), m_capacity(other.m_size + 1) {
      m_data = allocate(m_capacity);
      copy(m_data, other.m_data, m_size);
      m_data[m_size] = CharT { 0 };
    }
    BasicString(BasicString &&other) noexcept
        : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
      other.m_data = nullptr;
      other.m_size = 0;
      other.m_capacity = 0;
    }
    ~BasicString() { deallocate(m_data); }

    BasicString &operator=(const BasicString &lvalue) {
      if (this != &lvalue) {
        BasicString tmp(lvalue);
        swap(tmp);
      }
      return *this;
    }

    BasicString &operator=(const CharT *str) {
      BasicString tmp(str);
      swap(tmp);
      return *this;
    }

    BasicString(const std::basic_string<CharT> &other) : BasicString(other.c_str()) {}
    BasicString &operator=(const std::basic_string<CharT> &other) { return operator=(other.c_str()); }

    /*!
     * Copies the viewed characters. A view is not required to be null-terminated - it may name a
     * slice of a larger buffer - so this copies by size rather than scanning for a terminator, and
     * adds the terminator itself.
     *
     * Defined out of line because BasicStringView is not complete until below.
     */
    BasicString(BasicStringView<CharT> view);
    BasicString &operator=(BasicStringView<CharT> view);


    BasicString &operator=(BasicString &&rvalue) noexcept {
      if (this != &rvalue) {
        deallocate(m_data);
        m_data = rvalue.m_data;
        m_size = rvalue.m_size;
        m_capacity = rvalue.m_capacity;
        rvalue.m_data = nullptr;
        rvalue.m_size = 0;
        rvalue.m_capacity = 0;
      }
      return *this;
    }

    void swap(BasicString &other) noexcept {
      CharT *d = m_data;
      m_data = other.m_data;
      other.m_data = d;
      size_t s = m_size;
      m_size = other.m_size;
      other.m_size = s;
      size_t c = m_capacity;
      m_capacity = other.m_capacity;
      other.m_capacity = c;
    }

    const CharT *data() const noexcept { return m_data; }
    CharT *data() noexcept { return m_data; }

    const CharT *c_str() const noexcept {
      static constexpr CharT null = CharT { 0 };
      return m_data ? m_data : &null;
    }

    size_t size() const noexcept { return m_size; }
    size_t capacity() const noexcept { return m_capacity; }
    bool empty() const noexcept { return m_size == 0; }

    void reserve(size_t newCapacity) {
      if (newCapacity <= m_capacity) return;

      CharT *newData = allocate(newCapacity);
      if (m_data) {
        copy(newData, m_data, m_size);
        newData[m_size] = CharT(0);
        deallocate(m_data);
      } else {
        newData[0] = CharT(0);
      }
      m_data = newData;
      m_capacity = newCapacity;
    }

    void resize(size_t newSize) {
      if (newSize > m_capacity) reserve(newSize + 1);
      if (newSize > m_size)
        for (size_t i = m_size; i < newSize; ++i)
          m_data[i] = CharT(0);

      m_size = newSize;
      m_data[m_size] = CharT(0);
    }

    void append(const CharT *str) {
      if (!str) return;

      size_t addLen = StrLen(str);
      size_t newSize = m_size + addLen;

      if (newSize + 1 > m_capacity) {
        size_t newCap = m_capacity == 0 ? newSize + 1 : m_capacity;
        while (newCap < newSize + 1)
          newCap *= 2;
        reserve(newCap);
      }

      copy(m_data + m_size, str, addLen);
      m_size = newSize;
      m_data[m_size] = CharT(0);
    }

    void append(const BasicString &other) { append(other.c_str()); }

    void push_back(CharT ch) {
      CharT buf[2] = { ch, CharT(0) };
      append(buf);
    }

    constexpr const CharT *begin() const noexcept { return data(); }
    constexpr const CharT *end() const noexcept { return data() + m_size; }

    CharT &operator[](size_t i) noexcept { return m_data[i]; }
    CharT operator[](size_t i) const noexcept { return m_data[i]; }

    CharT &at(size_t i) {
      // if (i >= m_size) throw std::out_of_range("BasicString::at");
      return m_data[i];
    }

    BasicString operator+(const BasicString &rhs) const {
      BasicString result(*this);
      result.append(rhs);
      return result;
    }

    BasicString &operator+=(const BasicString &rhs) {
      append(rhs);
      return *this;
    }

    bool operator==(const BasicString &rhs) const noexcept {
      return m_size == rhs.m_size && MemCmp(m_data, rhs.m_data, m_size) == 0;
    }

    bool operator!=(const BasicString &rhs) const noexcept { return !(*this == rhs); }

    /*!
     * Comparing against a view is an exact match, which is the point of it existing: without this
     * overload `string == view` is ambiguous, since the view converts to a string and the string
     * converts to a view. This one needs neither, so it wins outright - and never allocates.
     */
    bool operator==(BasicStringView<CharT> rhs) const noexcept;
  };

  template <Character CharT>
  class BasicStringView {
    friend bool operator==(BasicStringView<CharT> a, BasicStringView<CharT> b) {
      return a.size() == b.size() && MemCmp(a.data(), b.data(), a.size()) == 0;
    }
  public:
    constexpr BasicStringView() = default;
    constexpr BasicStringView(const BasicString<CharT> &string) noexcept : m_data(string.m_data), m_size(string.m_size) {}
    constexpr BasicStringView(const CharT *str) noexcept : m_data(str), m_size(StrLen(str)) {}
    constexpr BasicStringView(const CharT *str, const size_t size) noexcept : m_data(str), m_size(size) {}

    constexpr const CharT *begin() const noexcept { return m_data; }
    constexpr const CharT *end() const noexcept { return m_data + m_size; }

    constexpr size_t size() const noexcept { return m_size; }
    constexpr const CharT *data() const noexcept { return m_data; }
    constexpr const CharT *c_str() const noexcept { return m_data; }


  private:
    const CharT *m_data;
    size_t m_size;
  };

  //template <Character CharT>
  //bool operator==(BasicStringView<CharT> a, BasicStringView<CharT> b) {
  //  return a.size() == b.size() && memcmp(a.data(), b.data(), a.size()) == 0;
  //}

  template <Character CharT>
  BasicString<CharT>::BasicString(const BasicStringView<CharT> view)
      : m_data(allocate(view.size() + 1)), m_size(view.size()), m_capacity(view.size() + 1) {
    if (view.data()) copy(m_data, view.data(), m_size);
    m_data[m_size] = CharT { 0 };
  }

  template <Character CharT>
  bool BasicString<CharT>::operator==(const BasicStringView<CharT> rhs) const noexcept {
    return m_size == rhs.size() && MemCmp(m_data, rhs.data(), m_size) == 0;
  }

  template <Character CharT>
  BasicString<CharT> &BasicString<CharT>::operator=(const BasicStringView<CharT> view) {
    /* Copy-and-swap rather than assigning in place: the view may point into this string's own
     * buffer, which a resize would free out from under it. */
    BasicString tmp(view);
    swap(tmp);
    return *this;
  }

  using String = BasicString<char>;
  // using WString = BasicString<char16_t>;
  // using UString = BasicString<char32_t>;

  using StringView = BasicStringView<char>;
  // using WStringView = BasicStringView<char16_t>;
  // using UStringView = BasicStringView<char32_t>;


  //uint64_t FNV1A64(const StringView &str);
} // namespace ROSE

template <ROSE::Character CharT>
struct std::formatter<ROSE::BasicString<CharT>, CharT> : std::formatter<std::basic_string<CharT>, CharT> {
  template <typename FormatContext>
  auto format(const ROSE::BasicString<CharT> &s, FormatContext &ctx) const {
    return std::formatter<std::basic_string<CharT>, CharT>::format(std::basic_string<CharT>(s.data(), s.size()), ctx);
  }
};

template <ROSE::Character CharT>
struct std::formatter<ROSE::BasicStringView<CharT>, CharT> : std::formatter<std::basic_string<CharT>, CharT> {
  template <typename FormatContext>
  auto format(const ROSE::BasicStringView<CharT> &s, FormatContext &ctx) const {
    return std::formatter<std::basic_string<CharT>, CharT>::format(std::basic_string<CharT>(s.data(), s.size()), ctx);
  }
};

/**

  @file      ROSE_string.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      6.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/preamble/ROSE_typetraits.h>
#include <ROSE/Core/rtl/ROSE_list.h>

namespace ROSE {

  /**
      @brief  General StrLen template for any Character type
      @tparam CharT - character type
      @param  str   - input string
      @retval       - length of string
  **/
  template<Character CharT>
  constexpr size_t StrLen(const CharT *str) noexcept {
    if (!str) return 0;
    size_t len = 0;
    while (str[len] != CharT(0))
      ++len;
    return len;
  }


  template <Character CharT>
  class BasicString {
  private:
    template <Character U>
    friend class BasicStringView;

    CharT *m_data;
    size_t m_size;
    size_t m_capacity;

    static CharT *allocate(const size_t count) {
      return static_cast<CharT *>(::operator new(count * sizeof(CharT)));
    }
    static void deallocate(CharT *ptr) noexcept {
      ::operator delete(ptr);
    }
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
    BasicString(const BasicString &other) : m_size(other.m_size), m_capacity(other.m_size+1) {
      m_data = allocate(m_capacity);
      copy(m_data, other.m_data, m_size);
      m_data[m_size] = CharT{ 0 };
    }
    BasicString(BasicString &&other) noexcept : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
      other.m_data = nullptr;
      other.m_size = 0;
      other.m_capacity = 0;
    }
    ~BasicString() {
      deallocate(m_data);
    }
    
    BasicString &operator=(const BasicString &lvalue) {
      if (this != &lvalue) {
        BasicString tmp(lvalue);
        swap(tmp);
      }
      return *this;
    }


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
      size_t  s = m_size;
      m_size = other.m_size;
      other.m_size = s;
      size_t c = m_capacity;
      m_capacity = other.m_capacity;
      other.m_capacity = c;
    }

    const CharT *data() const noexcept { return m_data; }
    CharT *data() noexcept { return m_data; }

    const CharT *c_str() const noexcept {
      static constexpr CharT null = CharT{ 0 };
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
        size_t newCap = m_capacity == 0 ? newSize + 1
          : m_capacity;
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
    constexpr const CharT *end()   const noexcept { return data() + m_size; }

    CharT &operator[](size_t i)       noexcept { return m_data[i]; }
    CharT  operator[](size_t i) const noexcept { return m_data[i]; }

    CharT &at(size_t i) {
      //if (i >= m_size) throw std::out_of_range("BasicString::at");
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
      return m_size == rhs.m_size && memcmp(m_data, rhs.m_data, m_size) == 0;
    }

    bool operator!=(const BasicString &rhs) const noexcept { return !(*this == rhs); }

  };

  template <Character CharT>
  class BasicStringView {
  public:
    constexpr BasicStringView(const BasicString<CharT> &string) : m_data(string.m_data), m_size(string.m_size) {}

    constexpr const CharT *begin() const noexcept { return m_data; }
    constexpr const CharT *end()   const noexcept { return m_data + m_size; }

    constexpr size_t size() { return m_size; }
    constexpr CharT *data() { return m_data; }
    constexpr const CharT *c_str() const { return m_data; }


  private:
    CharT *m_data;
    size_t m_size;
  };

  template<Character CharT>
  bool operator==(BasicStringView<CharT> a, BasicStringView<CharT> b) {
    return a.size() == b.size() && memcmp(a.data(), b.data(), a.size()) == 0;
  }

  using String = BasicString<char>;
  //using WString = BasicString<char16_t>;
  //using UString = BasicString<char32_t>;

  using StringView = BasicStringView<char>;
  //using WStringView = BasicStringView<char16_t>;
  //using UStringView = BasicStringView<char32_t>;


  uint64_t FNV1A(const StringView &str);
}
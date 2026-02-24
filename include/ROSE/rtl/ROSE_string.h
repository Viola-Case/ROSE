/**

  @file      ROSE_string.h
  @brief     
  @details   ~
  @author    Viola Case
  @date      6.02.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/preamble/ROSE_typetraits.h>
#include <ROSE/rtl/ROSE_list.h>

namespace ROSE {

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
    CharT *m_data;
    size_t m_size;
    size_t m_capacity;

  public:
    using value_type = CharT;

    BasicString() noexcept : m_data(nullptr), m_size(0), m_capacity(0) {}
    //BasicString(const CharT *str) {
    //  m_size = StrLen(str);
    //  m_capacity = m_size + 1;
    //  m_data = allocate(m_capacity);
    //  copy(m_data, str, m_size);
    //  m_data[m_size] = 0;
    //}
    //BasicString(const BasicString &other) {
    //  
    //}
    //BasicString(BasicString &&other) noexcept : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity) {
    //  other.m_data = nullptr;
    //  other.m_size = 0;
    //  other.m_capacity = 0;
    //}
    //~BasicString();
    //
    BasicString &operator=(const BasicString &lvalue);


    BasicString &operator=(BasicString &&rvalue) noexcept {
      m_data = rvalue.m_data;
      m_size = rvalue.m_size;
      m_capacity = rvalue.m_capacity;
      rvalue.m_data = nullptr;
      rvalue.m_size = 0;
      rvalue.m_capacity = 0;

    }

    const CharT *data() const noexcept;
    CharT *data() noexcept;

    size_t size() const noexcept;
    size_t capacity() const noexcept;

    void reserve(size_t newCapacity);
    void resize(size_t newSize);

    void append(const CharT *str);
  };

  using String = BasicString<char>;
  using WString = BasicString<wchar_t>;

}
/**

    @file      ROSE_hashmap.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/rtl/ROSE_buffer.h>
#include <ROSE/preamble/ROSE_stdlib.h>

namespace ROSE {
  class HashMap {

    using HashFn = uint64_t(*)(const void *);
    using EqualFn = bool(*)(const void *, const void *);

  private:
    RawBuffer _buffer;

    size_t _keySize;
    size_t _valueSize;
    size_t _bucketSize;

    size_t _capacity;
    size_t _size;

  };

  template<typename K, typename V>
  class TypedHashMap {
    HashMap _map;
  };
}
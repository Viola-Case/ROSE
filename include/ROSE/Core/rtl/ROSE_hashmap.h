/**

    @file      ROSE_hashmap.h
    @brief     
    @details   ~
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/rtl/ROSE_buffer.h>
#include <ROSE/Core/preamble/ROSE_stdlib.h>
#include <ROSE/Core/rtl/ROSE_utility.h>

namespace ROSE {


  class HashMap {

    using HashFn = uint64_t(*)(const void *);
    using EqualFn = bool(*)(const void *, const void *);

  private:

    static constexpr size_t NPOS = static_cast<size_t>(-1);

    struct BucketState {
      enum Value : uint8_t {
        Empty = 0,
        Occupied = 1,
        Tombstone = 2
      } value;
      constexpr BucketState(Value v) : value(v) {}
      constexpr BucketState(uint8_t v) : value(static_cast<Value>(v)) {}
      constexpr operator uint8_t() const noexcept { return static_cast<uint8_t>(value); }
    };

    uint8_t *bucketAt(size_t idx);

    BucketState getState(size_t index);

    void *keyAt(size_t idx);
    void *valueAt(size_t idx);

    size_t findSlot(const void *key);

    void insert(const void *key, const void *value);

    void remove(const void *key);

    uint8_t *bucketAt(RawBuffer &buf, size_t idx);

    void reserve(size_t newCapacity);

    void resize(size_t newSize);

    HashFn m_hashFn;
    EqualFn m_equalFn;

    RawBuffer m_buffer;

    size_t m_keySize;
    size_t m_valueSize;
    size_t m_bucketSize;

    size_t m_capacity;
    size_t m_size;

  };

  

  template<typename _Key, typename _Val, HashFunction Func = HashFunction::MurmurHash>
  class LinkedHashMap {
    HashMap _map;
    
  };

  template<typename _Key, typename _Val, HashFunction Func = HashFunction::MurmurHash>
  class TypedHashMap {
    HashMap _map;
  };
}
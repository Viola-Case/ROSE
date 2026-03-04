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
#include <ROSE/rtl/ROSE_utility.h>

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

    uint8_t *bucketAt(size_t idx) {
      return static_cast<uint8_t *>(m_buffer.data()) + idx * m_bucketSize;
    }

    BucketState getState(size_t index) {
      return static_cast<BucketState>(*bucketAt(index));
    }

    void *keyAt(size_t idx) { return bucketAt(idx) + 1; }
    void *valueAt(size_t idx) { return bucketAt(idx) + 1 + m_keySize; }

    size_t findSlot(const void *key) {
      size_t idx = m_hashFn(key) % m_capacity;
      while (getState(idx) != BucketState::Empty) {
        if (getState(idx) == BucketState::Occupied && m_equalFn(keyAt(idx), key))
          return idx; // found
        idx = (idx + 1) % m_capacity;
      }
      return NPOS; // not found
    }

    void insert(const void *key, const void *value) {
      if (m_size >= m_capacity * 3 / 4) resize(m_capacity * 2);

      size_t idx = m_hashFn(key) % m_capacity;
      while (getState(idx) == BucketState::Occupied) {
        if (m_equalFn(keyAt(idx), key)) {
          memcpy(valueAt(idx), value, m_valueSize); 
          return;
        }
        idx = (idx + 1) % m_capacity;
      }
      *bucketAt(idx) = static_cast<uint8_t>(BucketState::Occupied);
      memcpy(keyAt(idx), key, m_keySize);
      memcpy(valueAt(idx), value, m_valueSize);
      m_size++;
    }

    void remove(const void *key) {
      size_t idx = findSlot(key);
      if (idx == NPOS) return;
      *bucketAt(idx) = static_cast<uint8_t>(BucketState::Tombstone);
      m_size--;
    }

    uint8_t *bucketAt(RawBuffer &buf, size_t idx) {
      return static_cast<uint8_t *>(buf.data()) + idx * m_bucketSize;
    }

    void reserve(size_t newCapacity) {
      RawBuffer newBuffer(newCapacity * m_bucketSize);
      memset(newBuffer.data(), 0, newCapacity * m_bucketSize);
      for (size_t i = 0; i < m_capacity; i++) {
        if (getState(i) != BucketState::Occupied) continue;

        size_t idx = m_hashFn(keyAt(i)) % newCapacity;
        while (*bucketAt(newBuffer, idx) != BucketState::Empty)
          idx = (idx + 1) % newCapacity;

        MemCpy(reinterpret_cast<unsigned char *>(newBuffer.data()) + idx * m_bucketSize, bucketAt(i), m_bucketSize);
      }
      m_buffer.free();
      m_buffer = Move(newBuffer);
    }

    void resize(size_t newSize) {
      size_t needed = newSize * 4 / 3;
      if (needed > m_capacity)
        reserve(nextPow2(needed));

    }

    HashFn m_hashFn;
    EqualFn m_equalFn;

    RawBuffer m_buffer;

    size_t m_keySize;
    size_t m_valueSize;
    size_t m_bucketSize;

    size_t m_capacity;
    size_t m_size;

  };

  enum class HashFunction {
    MurmurHash,
    FNV_1a,
    CityHash,
    XXHash
  };

  template<typename _Key, typename _Val, HashFunction Func>
  class TypedHashMap {
    HashMap _map;
  };
}
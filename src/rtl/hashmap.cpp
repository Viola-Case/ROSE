/**

  @file      hashmap.cpp
  @brief     
  @details   ~
  @author    Viola Case
  @date      9.03.2026
  @copyright © Viola Case, 2026. All rights reserved.

**/

#include <ROSE/ROSE.h>

namespace ROSE {





  uint8_t *HashMap::bucketAt(size_t idx) {
    return static_cast<uint8_t *>(m_buffer.data()) + idx * m_bucketSize;
  }

  HashMap::BucketState HashMap::getState(size_t index) {
    return static_cast<BucketState>(*bucketAt(index));
  }

  void *HashMap::keyAt(size_t idx) { return bucketAt(idx) + 1; }
  void *HashMap::valueAt(size_t idx) { return bucketAt(idx) + 1 + m_keySize; }

  size_t HashMap::findSlot(const void *key) {
    size_t idx = m_hashFn(key) % m_capacity;
    while (getState(idx) != BucketState::Empty) {
      if (getState(idx) == BucketState::Occupied && m_equalFn(keyAt(idx), key))
        return idx; // found
      idx = (idx + 1) % m_capacity;
    }
    return NPOS; // not found
  }

  void HashMap::insert(const void *key, const void *value) {
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


  void HashMap::remove(const void *key) {
    size_t idx = findSlot(key);
    if (idx == NPOS) return;
    *bucketAt(idx) = static_cast<uint8_t>(BucketState::Tombstone);
    m_size--;
  }

  uint8_t *HashMap::bucketAt(RawBuffer &buf, size_t idx) {
    return static_cast<uint8_t *>(buf.data()) + idx * m_bucketSize;
  }


  void HashMap::reserve(size_t newCapacity) {
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

  void HashMap::resize(size_t newSize) {
    size_t capacity_needed = newSize * 4 / 3;
    if (capacity_needed > m_capacity)
      reserve(NextPow2(capacity_needed));

  }
}
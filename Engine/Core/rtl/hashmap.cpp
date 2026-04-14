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

  // ---------------------------------------------------------------------------
  // Construction / destruction
  // ---------------------------------------------------------------------------

  HashMap::HashMap(const TypeOps *_ops) noexcept
    : m_ops(_ops),
    m_stateBuffer(),
    m_entryBuffer(),
    m_capacity(0),
    m_size(0) {
  }

  HashMap::~HashMap() {
    destroyAllEntries();
  }

  HashMap::HashMap(HashMap &&_other) noexcept
    : m_ops(_other.m_ops),
    m_stateBuffer(Move(_other.m_stateBuffer)),
    m_entryBuffer(Move(_other.m_entryBuffer)),
    m_capacity(_other.m_capacity),
    m_size(_other.m_size) {
    _other.m_capacity = 0;
    _other.m_size = 0;
  }

  HashMap &HashMap::operator=(HashMap &&_other) noexcept {
    if (this == &_other)
      return *this;

    destroyAllEntries();

    m_ops = _other.m_ops;
    m_stateBuffer = Move(_other.m_stateBuffer);
    m_entryBuffer = Move(_other.m_entryBuffer);
    m_capacity = _other.m_capacity;
    m_size = _other.m_size;

    _other.m_capacity = 0;
    _other.m_size = 0;
    return *this;
  }

  // ---------------------------------------------------------------------------
  // Slot accessors
  // ---------------------------------------------------------------------------

  uint8_t *HashMap::stateAt(size_t _idx) noexcept {
    return static_cast<uint8_t *>(m_stateBuffer.data()) + _idx;
  }

  const uint8_t *HashMap::stateAt(size_t _idx) const noexcept {
    return static_cast<const uint8_t *>(m_stateBuffer.data()) + _idx;
  }

  void *HashMap::entryAt(size_t _idx) noexcept {
    return static_cast<uint8_t *>(m_entryBuffer.data()) + _idx * m_ops->entrySize;
  }

  const void *HashMap::entryAt(size_t _idx) const noexcept {
    return static_cast<const uint8_t *>(m_entryBuffer.data()) + _idx * m_ops->entrySize;
  }

  const void *HashMap::keyInEntry(const void *_entry) const noexcept {
    return static_cast<const uint8_t *>(_entry) + m_ops->keyOffset;
  }

  // ---------------------------------------------------------------------------
  // Probing
  // ---------------------------------------------------------------------------

  size_t HashMap::findSlot(const void *_key) const noexcept {
    if (m_capacity == 0)
      return NPOS;

    const size_t mask = m_capacity - 1;
    size_t idx = m_ops->hash(_key) & mask;

    for (size_t probes = 0; probes < m_capacity; ++probes) {
      const uint8_t state = *stateAt(idx);
      if (state == BucketState::Empty)
        return NPOS;
      if (state == BucketState::Occupied &&
          m_ops->equal(keyInEntry(entryAt(idx)), _key))
        return idx;
      idx = (idx + 1) & mask;
    }
    return NPOS;
  }

  size_t HashMap::probeForInsert(const void *_key) noexcept {
    const size_t mask = m_capacity - 1;
    size_t idx = m_ops->hash(_key) & mask;
    size_t firstTombstone = NPOS;

    for (size_t probes = 0; probes < m_capacity; ++probes) {
      const uint8_t state = *stateAt(idx);

      if (state == BucketState::Empty)
        return firstTombstone != NPOS ? firstTombstone : idx;

      if (state == BucketState::Tombstone) {
        if (firstTombstone == NPOS)
          firstTombstone = idx;
      } else if (m_ops->equal(keyInEntry(entryAt(idx)), _key)) {
        return idx;
      }

      idx = (idx + 1) & mask;
    }

    // Completely full (should not happen with ensureCapacity) -- fall back.
    return firstTombstone;
  }

  // ---------------------------------------------------------------------------
  // Growth
  // ---------------------------------------------------------------------------

  void HashMap::ensureCapacity() {
    if (m_capacity == 0) {
      rehash(INITIAL_CAPACITY);
      return;
    }
    // 75% load factor: grow when m_size * 4 >= m_capacity * 3
    if ((m_size + 1) * 4 >= m_capacity * 3)
      rehash(m_capacity * 2);
  }

  void HashMap::rehash(size_t _newCapacity) {
    RawBuffer newStates(_newCapacity);
    RawBuffer newEntries(_newCapacity * m_ops->entrySize);

    // Initialise all new slots to Empty.
    std::memset(newStates.data(), BucketState::Empty, _newCapacity);

    const size_t newMask = _newCapacity - 1;
    uint8_t *newStateData = static_cast<uint8_t *>(newStates.data());
    uint8_t *newEntryData = static_cast<uint8_t *>(newEntries.data());

    for (size_t i = 0; i < m_capacity; ++i) {
      if (*stateAt(i) != BucketState::Occupied)
        continue;

      void *oldEntry = entryAt(i);
      size_t idx = m_ops->hash(keyInEntry(oldEntry)) & newMask;
      while (newStateData[idx] == BucketState::Occupied)
        idx = (idx + 1) & newMask;

      void *dst = newEntryData + idx * m_ops->entrySize;
      m_ops->moveConstructEntry(dst, oldEntry);
      m_ops->destroyEntry(oldEntry);
      newStateData[idx] = BucketState::Occupied;
    }

    m_stateBuffer = Move(newStates);
    m_entryBuffer = Move(newEntries);
    m_capacity = _newCapacity;
  }

  // ---------------------------------------------------------------------------
  // Lookup
  // ---------------------------------------------------------------------------

  HashMap::Iterator HashMap::find(const void *_key) noexcept {
    const size_t idx = findSlot(_key);
    if (idx == NPOS)
      return end();
    return Iterator{this, idx};
  }

  bool HashMap::contains(const void *_key) const noexcept {
    return findSlot(_key) != NPOS;
  }

  // ---------------------------------------------------------------------------
  // Insert / erase / clear
  // ---------------------------------------------------------------------------

  void *HashMap::insert(void *_entrySrc) {
    ensureCapacity();

    const void *key = static_cast<const uint8_t *>(_entrySrc) + m_ops->keyOffset;
    const size_t idx = probeForInsert(key);

    uint8_t *state = stateAt(idx);
    void *dst = entryAt(idx);

    if (*state == BucketState::Occupied) {
      // Overwrite: destroy the old entry first.
      m_ops->destroyEntry(dst);
      m_ops->moveConstructEntry(dst, _entrySrc);
      return dst;
    }

    m_ops->moveConstructEntry(dst, _entrySrc);
    *state = BucketState::Occupied;
    ++m_size;
    return dst;
  }

  bool HashMap::erase(const void *_key) noexcept {
    const size_t idx = findSlot(_key);
    if (idx == NPOS)
      return false;

    m_ops->destroyEntry(entryAt(idx));
    *stateAt(idx) = BucketState::Tombstone;
    --m_size;
    return true;
  }

  void HashMap::clear() noexcept {
    if (m_capacity == 0)
      return;

    for (size_t i = 0; i < m_capacity; ++i) {
      uint8_t *state = stateAt(i);
      if (*state == BucketState::Occupied)
        m_ops->destroyEntry(entryAt(i));
      *state = BucketState::Empty;
    }
    m_size = 0;
  }

  void HashMap::destroyAllEntries() noexcept {
    if (m_capacity == 0 || m_stateBuffer.data() == nullptr)
      return;

    for (size_t i = 0; i < m_capacity; ++i) {
      if (*stateAt(i) == BucketState::Occupied)
        m_ops->destroyEntry(entryAt(i));
    }
    m_size = 0;
  }

  void HashMap::reset() noexcept {
    m_stateBuffer.free();
    m_entryBuffer.free();
    m_capacity = 0;
    m_size = 0;
  }

  // ---------------------------------------------------------------------------
  // Iteration
  // ---------------------------------------------------------------------------

  HashMap::Iterator HashMap::begin() noexcept {
    Iterator it{this, 0};
    it.skipToOccupied();
    return it;
  }

  HashMap::Iterator HashMap::end() noexcept {
    return Iterator{this, m_capacity};
  }

  void HashMap::Iterator::skipToOccupied() noexcept {
    while (m_index < m_owner->m_capacity &&
           *m_owner->stateAt(m_index) != BucketState::Occupied)
      ++m_index;
  }

  void *HashMap::Iterator::get() const noexcept {
    return m_owner->entryAt(m_index);
  }

  HashMap::Iterator &HashMap::Iterator::operator++() noexcept {
    if (m_index < m_owner->m_capacity) {
      ++m_index;
      skipToOccupied();
    }
    return *this;
  }

  bool HashMap::Iterator::operator==(const Iterator &_other) const noexcept {
    return m_owner == _other.m_owner && m_index == _other.m_index;
  }

  bool HashMap::Iterator::operator!=(const Iterator &_other) const noexcept {
    return !(*this == _other);
  }
}

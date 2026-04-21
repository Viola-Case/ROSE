/**

    @file      ROSE_hashmap.h
    @brief     HashMap and TypedHashMap — open-addressed hash map containers
    @details   HashMap is a type-erased implementation whose heavy lifting is
               compiled once in hashmap.cpp. TypedHashMap<K,V> is a thin
               type-safe wrapper that supplies the TypeOps table at construction.
    @author    Viola Case
    @date      18.02.2026
    @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <initializer_list>
#include <type_traits>

#include <ROSE/Core/ROSE_stdlib.h>
#include <ROSE/Core/rtl/ROSE_buffer.h>
#include <ROSE/Core/rtl/ROSE_pair.h>
#include <ROSE/Core/rtl/ROSE_string.h>
#include <ROSE/Core/rtl/ROSE_utility.h>

namespace ROSE {

  /**

      @class   HashMap
      @brief   Type-erased open-addressed hash map.
      @details The heavy lifting lives here and in hashmap.cpp so that the
               per-instantiation code emitted by TypedHashMap stays tiny.
               Layout, probing, tombstoning, and rehashing are all driven by
               the per-instance TypeOps table supplied by the templated
               wrapper. All method names stay lower-case to match the
               existing RTL (List, BasicString) and to satisfy C++
               range-for's begin()/end() lookup.

  **/
  class HashMap {
    friend class Iterator;

  public:

    struct TypeOps {
      size_t entrySize;
      size_t entryAlign;
      size_t keyOffset;
      size_t valueOffset;
      size_t keySize;
      uint64_t(*hash)(const void *_key);
      bool(*equal)(const void *_a, const void *_b);
      void(*moveConstructEntry)(void *_dst, void *_src);
      void(*destroyEntry)(void *_entry);
    };

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

    class Iterator {
      friend class HashMap;
    public:
      [[nodiscard]] void *get() const noexcept;
      Iterator &operator++() noexcept;
      [[nodiscard]] bool operator==(const Iterator &_other) const noexcept;
      [[nodiscard]] bool operator!=(const Iterator &_other) const noexcept;
    private:
      Iterator(HashMap *_owner, size_t _index) noexcept : m_owner(_owner), m_index(_index) {}
      void skipToOccupied() noexcept;

      HashMap *m_owner{nullptr};
      size_t m_index{0};
    };

    explicit HashMap(const TypeOps *_ops) noexcept;
    ~HashMap();

    HashMap(const HashMap &) = delete;
    HashMap &operator=(const HashMap &) = delete;

    HashMap(HashMap &&_other) noexcept;
    HashMap &operator=(HashMap &&_other) noexcept;

    /// @brief Returns an iterator to the first occupied bucket.
    [[nodiscard]] Iterator begin() noexcept;
    /// @brief Returns a past-the-end sentinel iterator.
    [[nodiscard]] Iterator end() noexcept;

    /**
      @brief   Finds an entry by key.
      @param   _key  Pointer to the key value to search for.
      @retval  Iterator to the matching entry, or end() if not found.
    **/
    [[nodiscard]] Iterator find(const void *_key) noexcept;

    /**
      @brief   Returns true if an entry with the given key exists.
      @param   _key  Pointer to the key value to check.
    **/
    [[nodiscard]] bool contains(const void *_key) const noexcept;

    /**
      @brief   Move-insert or overwrite an entry at _entrySrc.
      @details Takes a pointer to a fully constructed Entry living in the
               caller's storage. On return, the value at _entrySrc has been
               moved-from (still destructible). The returned pointer refers
               to the in-buffer slot.
    **/
    void *insert(void *_entrySrc);

    /**
      @brief   Removes the entry with the given key.
      @param   _key  Pointer to the key of the entry to remove.
      @retval  true if the entry existed and was removed; false if not found.
    **/
    bool erase(const void *_key) noexcept;

    /// @brief Removes all entries. Capacity is retained.
    void clear() noexcept;

    [[nodiscard]] size_t size() const noexcept { return m_size; }
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }
    [[nodiscard]] size_t capacity() const noexcept { return m_capacity; }

  private:

    static constexpr size_t NPOS = static_cast<size_t>(-1);
    static constexpr size_t INITIAL_CAPACITY = 8;

    [[nodiscard]] uint8_t *stateAt(size_t _idx) noexcept;
    [[nodiscard]] const uint8_t *stateAt(size_t _idx) const noexcept;
    [[nodiscard]] void *entryAt(size_t _idx) noexcept;
    [[nodiscard]] const void *entryAt(size_t _idx) const noexcept;
    [[nodiscard]] const void *keyInEntry(const void *_entry) const noexcept;

    [[nodiscard]] size_t findSlot(const void *_key) const noexcept;
    [[nodiscard]] size_t probeForInsert(const void *_key) noexcept;

    void ensureCapacity();
    void rehash(size_t _newCapacity);
    void destroyAllEntries() noexcept;
    void reset() noexcept;

    const TypeOps *m_ops{nullptr};
    RawBuffer m_stateBuffer{};
    RawBuffer m_entryBuffer{};
    size_t m_capacity{0};
    size_t m_size{0};
  };

  /**

      @struct  Hasher
      @brief   Default hash functor used by TypedHashMap.
      @details Bit-hashes the key via FNV1A over its raw bytes. This is only
               meaningful for trivially-copyable keys (POD / UUID / scalars)
               where byte-equality matches value-equality. Any key type with
               indirection (BasicString, List, etc.) needs its own Hasher
               specialization so the hash reflects the logical content,
               otherwise two equal keys with different heap pointers will
               hash to different buckets.

  **/
  template<typename K>
  struct Hasher {
    static_assert(std::is_trivially_copyable_v<K>,
                  "Default ROSE::Hasher<K> only supports trivially-copyable "
                  "keys. Provide a specialization of ROSE::Hasher<K> for "
                  "types with indirection (strings, containers, etc.).");
    uint64_t operator()(const K &_key) const noexcept {
      return FNV1A64(&_key, sizeof(K));
    }
  };

  /**
      @brief   Content-based hash for BasicString keys.
      @details Hashes the string bytes, so two BasicString instances with the
               same text hash to the same bucket even if their m_data
               allocations differ. Required for TypedHashMap<String, V> to
               behave correctly across the insert/find boundary.
  **/
  template<Character CharT>
  struct Hasher<BasicString<CharT>> {
    uint64_t operator()(const BasicString<CharT> &_key) const noexcept {
      return FNV1A(_key.data(), _key.size() * sizeof(CharT));
    }
  };

  /**

      @class   TypedHashMap
      @brief   Type-safe thin wrapper over the precompiled HashMap.
      @tparam  K - key type (must be equality-comparable and trivially
                    relocatable)
      @tparam  V - value type (move-constructible; may be move-only)
      @tparam  Hash - hash functor; defaults to Hasher<K>

  **/
  template<typename K, typename V, typename Hash = Hasher<K>>
  class TypedHashMap {
  public:

    using Entry = Pair<K, V>;

    class Iterator {
      friend class TypedHashMap;
    public:
      [[nodiscard]] Entry &operator*() const noexcept {
        return *static_cast<Entry *>(m_it.get());
      }
      [[nodiscard]] Entry *operator->() const noexcept {
        return static_cast<Entry *>(m_it.get());
      }
      Iterator &operator++() noexcept {
        ++m_it;
        return *this;
      }
      [[nodiscard]] bool operator==(const Iterator &_other) const noexcept {
        return m_it == _other.m_it;
      }
      [[nodiscard]] bool operator!=(const Iterator &_other) const noexcept {
        return m_it != _other.m_it;
      }
    private:
      explicit Iterator(HashMap::Iterator _it) noexcept : m_it(_it) {}
      HashMap::Iterator m_it;
    };

    TypedHashMap() noexcept : m_map(&ops()) {}
    ~TypedHashMap() = default;

    TypedHashMap(const TypedHashMap &) = delete;
    TypedHashMap &operator=(const TypedHashMap &) = delete;

    TypedHashMap(TypedHashMap &&) noexcept = default;
    TypedHashMap &operator=(TypedHashMap &&) noexcept = default;

    /**
      @brief   Brace-list construction: TypedHashMap<K, V> m{{k0, v0}, {k1, v1}}.
      @details Requires both K and V to be copy-constructible. For move-only V
               (e.g. UniquePtr), build the map with repeated insert() /
               emplace() calls instead.
    **/
    TypedHashMap(std::initializer_list<Pair<K, V>> _init) : m_map(&ops()) {
      for (const Pair<K, V> &entry : _init) {
        Entry tmp{entry.first, entry.second};
        m_map.insert(&tmp);
      }
    }

    /// @brief Returns an iterator to the first entry.
    [[nodiscard]] Iterator begin() noexcept { return Iterator{m_map.begin()}; }
    /// @brief Returns a past-the-end sentinel iterator.
    [[nodiscard]] Iterator end() noexcept { return Iterator{m_map.end()}; }

    /**
      @brief   Finds an entry by key.
      @param   _key  Key to search for.
      @retval  Iterator to the matching entry, or end() if not found.
    **/
    [[nodiscard]] Iterator find(const K &_key) noexcept {
      return Iterator{m_map.find(&_key)};
    }

    /**
      @brief   Returns true if an entry with the given key exists.
      @param   _key  Key to check.
    **/
    [[nodiscard]] bool contains(const K &_key) const noexcept {
      return m_map.contains(&_key);
    }

    /**
      @brief   Inserts or overwrites an entry with the given key and move-constructed value.
      @retval  Iterator to the inserted/updated entry.
    **/
    Iterator insert(const K &_key, V &&_value) {
      Entry tmp{_key, Move(_value)};
      m_map.insert(&tmp);
      return Iterator{m_map.find(&_key)};
    }

    /**
      @brief   Inserts or overwrites an entry with the given key and copied value.
      @retval  Iterator to the inserted/updated entry.
    **/
    Iterator insert(const K &_key, const V &_value) {
      Entry tmp{_key, _value};
      m_map.insert(&tmp);
      return Iterator{m_map.find(&_key)};
    }

    /**
      @brief   Constructs a value in-place at the given key using the supplied arguments.
      @tparam  Args  Constructor argument types (deduced).
      @retval  Iterator to the inserted entry.
    **/
    template<class... Args>
    Iterator emplace(const K &_key, Args &&..._args) {
      Entry tmp{_key, V(Forward<Args>(_args)...)};
      m_map.insert(&tmp);
      return Iterator{m_map.find(&_key)};
    }

    /**
      @brief   Removes the entry with the given key.
      @retval  true if the entry existed and was removed; false if not found.
    **/
    bool erase(const K &_key) noexcept { return m_map.erase(&_key); }

    /// @brief Removes all entries. Capacity is retained.
    void clear() noexcept { m_map.clear(); }

    [[nodiscard]] size_t size() const noexcept { return m_map.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_map.empty(); }

  private:

    static const HashMap::TypeOps &ops() noexcept {
      static const HashMap::TypeOps value{
        sizeof(Entry),
        alignof(Entry),
        offsetof(Entry, first),
        offsetof(Entry, second),
        sizeof(K),
        +[](const void *_k) -> uint64_t {
          return Hash{}(*static_cast<const K *>(_k));
        },
        +[](const void *_a, const void *_b) -> bool {
          return *static_cast<const K *>(_a) == *static_cast<const K *>(_b);
        },
        +[](void *_dst, void *_src) {
          new (_dst) Entry(Move(*static_cast<Entry *>(_src)));
        },
        +[](void *_e) {
          static_cast<Entry *>(_e)->~Entry();
        },
      };
      return value;
    }

    HashMap m_map;
  };
}

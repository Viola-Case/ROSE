# RTL — the ROSE Template Library

`include/ROSE/Core/rtl.h` is an umbrella that pulls in, in this order:

```cpp
buffer.h  list.h  string.h  memory.h  array.h  pair.h  hashmap.h  bigint.h
```

Not in the umbrella but part of the same layer: `stdlib.h`, `typetraits.h`,
`utility.h`. `rtl.h` gets them transitively.

Everything is in namespace `ROSE` (no nested namespace, unlike math).

---

## Support layer

### `stdlib.h`

The single funnel for standard headers: `<cstring> <cstdlib> <cstddef> <cstdint>
<cmath> <utility> <format>`. `<cstdio>` is commented out. If a ROSE header needs
a standard facility, this is where it should come from.

### `typetraits.h` — concepts

```cpp
concept Character;      // char, signed/unsigned char, wchar_t, char8_t, char16_t
                        // NOTE: char32_t is commented out — deliberately excluded
concept StdScalar;      // std::is_arithmetic_v<T>
concept Scalar;         // Comp<...>, Quat<...>, or StdScalar
concept MultiByteType;  // sizeof(T) > 1
concept BehaviorType;   // std::derived_from<T, Behavior>
```

Forward-declares `ROSE::math::Comp` and `ROSE::math::Quat` so `Scalar` can name
them. `Scalar` is written as
`std::same_as<T, math::Comp<std::underlying_type_t<T>>> || …` — for a non-enum
`T`, `std::underlying_type_t` has no member and SFINAEs out to `false`, which is
why it works at all. It is fragile but currently correct.

### `utility.h`

```cpp
template <typename T> using Atomic = std::atomic<T>;   // deliberate: no custom atomics

constexpr T&&  Move(T&)                 // std::move equivalent
constexpr T&&  Forward(remove_ref_t<T>&)
constexpr void Swap(T& a, T& b)
constexpr T    Min(T a, T b)            // by value
constexpr T    Max(T a, T b)
constexpr T    Exchange(T& obj, U&& newval)

void           MemCpy(void*, const void*, size_t)   // out-of-line, in utility.cpp
constexpr void SmartMemCpy(T* dst, U* src, size_t count = 1)  // copies Min(sizeof(T),sizeof(U)) — ignores count
constexpr int  MemCmp(const T* a, const T* b, size_t count)   // count is in ELEMENTS; null-safe

constexpr uint16_t/32/64 ByteSwap(...)              // plus a ByteSwapResult-constrained cast form
constexpr size_t   NextPow2(size_t)
constexpr size_t   StrLen(const CharT*)             // null-safe, returns 0
constexpr uint32_t Tag(const char (&)[5])           // little-endian FourCC
constexpr int      ToLower(int)
constexpr uint64_t StrToULL(const char*)            // hex only; skips whitespace and 0x/0X

uint64_t FNV1A64(const void*, size_t);              // declared here AND in hashmap.h
uint64_t FNV1A64(const char*);
```

`MemCmp` compares **elements**, `std::memcmp` compares **bytes** — mixing the two
up is the cause of `known-issues.md` #4.

`utility.cpp` also *defines* `FNV1A64(const StringView&)` and
`FNV1A128(const StringView&)`, but no header declares them, so they are dead
symbols. `string.h` has the declaration commented out.

---

## `buffer.h` / `rtl/buffer.cpp` — `RawBuffer`

Move-only owner of an untyped `::operator new` allocation. The storage primitive
under `List` and `HashMap`.

```cpp
RawBuffer() noexcept;
explicit RawBuffer(size_t bytes);        // bytes == 0 leaves it null
RawBuffer(RawBuffer&&) noexcept;         // copy ctor/assign are = delete
~RawBuffer();

void  allocate(size_t bytes);            // discards old contents
void  reallocate(size_t bytes);          // preserves min(old, new) bytes via std::memcpy
void  free();
void* data() noexcept;                   // and a const overload
size_t size_bytes() const noexcept;
```

- Allocation is plain `::operator new`, so alignment is only
  `__STDCPP_DEFAULT_NEW_ALIGNMENT__` (16 on x64). **Over-aligned types are not
  supported** — relevant to `HashMap`, which records an `entryAlign` it never
  honours.
- No construction/destruction of elements; that is the caller's job.
- `reallocate(bytes)` on `bytes == m_size` is a no-op; on `0` it frees.

---

## `array.h` — `FixedArray<T, N>`

An aggregate-ish `T _data[N]` with `constexpr` accessors.

```cpp
constexpr FixedArray() noexcept = default;                     // elements INDETERMINATE, like std::array
template <size_t M> constexpr FixedArray(const T (&arr)[M]);   // static_asserts M == N
constexpr FixedArray(std::initializer_list<T>);                // ROSE_ASSERTs size, ignores extras

constexpr T*       data() noexcept;         // + const overload
constexpr size_t   size() const noexcept;
constexpr T&       operator[](size_t);      // + const overload
constexpr T*       begin()/end() noexcept;  // + const overloads
```

The default constructor **default-initializes**, so `FixedArray<int,3> a;` gives
three indeterminate ints — assign before reading. It exists at all only since the
`math::Mat` work; before that, declaring the two other constructors suppressed the
implicit one and made `math::Mat` and generic `math::Vec<T, N>` impossible to
construct (`known-issues.md` #2).

Includes `<ROSE/Core/macros.h>` for `ROSE_ASSERT_MSG`, which replaced an
ill-formed `static_assert(list.size() == N, …)` in the `initializer_list`
constructor — `list.size()` is not a constant expression, so instantiating that
constructor used to be a hard error. Both constructors work now.

---

## `pair.h` — `Pair<T1, T2>`

```cpp
T1 first; T2 second;
constexpr Pair() = default;                       // copy/move all defaulted
template <U1, U2> Pair(U1&& first, U2&& second);  // perfect-forwarding
bool operator==(const Pair&) const;
auto operator<=>(const Pair&) const;              // lexicographic
void swap(Pair&) noexcept;

template <T1, T2> Pair<T1,T2> MakePair(T1&&, T2&&);
```

`TypedHashMap::Entry` is `Pair<K, V>`, and the map takes `offsetof(Entry, first)`
/ `offsetof(Entry, second)` — so `Pair` must stay standard-layout.

---

## `list.h` — `List<T>`

The `std::vector` equivalent. `RawBuffer` storage, manual placement-new and
destructor calls, geometric growth.

```cpp
using value_type = T;  using size_type = size_t;

List() noexcept = default;
explicit List(size_t initial_capacity);        // reserve only, size stays 0
List(size_type count, const T& value);
List(std::initializer_list<T>);
template <size_t N> List(const FixedArray<T, N>&);
List(const List&);  List(List&&) noexcept;     // both assignment forms too

// capacity
constexpr size_t size()     const noexcept;
constexpr size_t capacity() const noexcept;    // derived: buffer bytes / sizeof(T)
constexpr bool   empty()    const noexcept;
void reserve(size_t);                          // no-op if <= capacity
void resize(size_t);                           // value-initialises new tail
void clear() noexcept;                         // destroys, keeps capacity

// access
constexpr T& operator[](size_t) noexcept;      // unchecked, + const overload
T& back() noexcept;                            // + const; UB when empty
constexpr T* begin()/end() noexcept;           // + const overloads
T* data() noexcept;                            // + const overload

// modifiers
void push_back(const T&);  void push_back(T&&);
template <Args...> T& emplace_back(Args&&...);
void pop_back();                               // safe no-op when empty

const bool operator==(const List&);            // BROKEN — see known-issues #4
```

Details worth remembering:

- Growth is `capacity ? capacity * 2 : 4`.
- Trivially-copyable `T` takes a `std::memcpy` fast path in copy-construct,
  copy-assign, and reallocate; otherwise it is element-wise placement-new with
  `std::move_if_noexcept` and explicit `~T()`.
- **Missing versus `std::vector`:** `insert`, `erase`, `front`, `at`,
  `shrink_to_fit`, reverse iterators, `assign`, `operator!=`, `swap`.
- `operator==` is non-`const` and returns `const bool`, which triggers
  `-Wambiguous-reversed-operator` on every use under C++20. It is also
  functionally wrong.

---

## `string.h` — `BasicString<CharT>` / `BasicStringView<CharT>`

```cpp
using String     = BasicString<char>;
using StringView = BasicStringView<char>;
```

The `WString`/`UString`/`WStringView`/`UStringView` aliases exist but are
commented out, so `char` is the only instantiation in use.

### `BasicString`

Three members: `m_data`, `m_size`, `m_capacity`. Always keeps a NUL terminator,
so `m_capacity` is `m_size + 1` after most operations. Allocation is
`::operator new` / `::operator delete` directly, **not** `RawBuffer`. No SSO.

```cpp
BasicString() noexcept;                         // null m_data, size 0, capacity 0
BasicString(const CharT* str);
BasicString(const BasicString&);  BasicString(BasicString&&) noexcept;
BasicString(const std::basic_string<CharT>&);   // interop, both ctor and operator=
BasicString& operator=(const CharT*);           // + copy/move assign

void swap(BasicString&) noexcept;
const CharT* data() const noexcept;             // + non-const; may be NULL
const CharT* c_str() const noexcept;            // never NULL — falls back to a static NUL
size_t size() const noexcept;
size_t capacity() const noexcept;
bool   empty() const noexcept;
void   reserve(size_t);
void   resize(size_t);                          // off-by-one, see known-issues #5
void   append(const CharT*);  void append(const BasicString&);
void   push_back(CharT);
constexpr const CharT* begin()/end() const noexcept;   // const only — no mutable iteration
CharT& operator[](size_t);  CharT operator[](size_t) const;
CharT& at(size_t);                              // bounds check is commented out; same as operator[]
BasicString  operator+(const BasicString&) const;
BasicString& operator+=(const BasicString&);
bool operator==/!=(const BasicString&) const noexcept;   // via ROSE::MemCmp
```

- `append` grows by doubling until it fits; `reserve` allocates exactly.
- **Missing versus `std::string`:** `find`, `substr`, `insert`, `erase`,
  `starts_with`, `clear`, `pop_back`, `front`/`back`, `operator<`/`<=>`,
  `operator+` with `const char*` (build a `String` first).
- Use `c_str()` rather than `data()` for anything C-facing; a default-constructed
  string has `data() == nullptr`.

### `BasicStringView`

Non-owning `{const CharT* m_data; size_t m_size;}`. Implicitly constructible
from `BasicString` (friend access to its privates) and from `const CharT*`
(calls `StrLen`), or explicitly from pointer + size. Provides
`begin`/`end`/`size`/`data`/`c_str` and a hidden-friend `operator==`.

`c_str()` is **not** guaranteed NUL-terminated — it is just `m_data`. Only safe
when the view was built from a whole `BasicString` or a literal.

### Formatting

`std::formatter<BasicString<CharT>, CharT>` is specialised at the bottom of the
header by delegating to `std::formatter<std::basic_string<CharT>>` — so the full
standard spec grammar works, at the cost of a temporary `std::basic_string` per
format call. Verified: `std::format("{}", String("rose")) == "rose"`.

---

## `hashmap.h` + `rtl/hashmap.cpp` — `HashMap` / `TypedHashMap`

Open-addressed map with **linear probing**, tombstones, and a 75% load factor.
Split deliberately in two so template bloat stays small: the type-erased
`HashMap` holds all the logic and is precompiled; `TypedHashMap<K, V, Hash>` is a
thin inline wrapper that hands it a static `TypeOps` vtable.

### Hash functions (declared here, defined in `utility.cpp`)

```cpp
uint64_t  FNV1A64 (const void* data, size_t len);
uint64_t  FNV1A64 (const char* str);
uint128_t FNV1A128(const void* data, size_t len);
uint128_t FNV1A128(const char* str);
```

### `HashMap` (type-erased)

```cpp
struct TypeOps {
  size_t entrySize, entryAlign, keyOffset, valueOffset, keySize;
  uint64_t (*hash)(const void* key);
  bool     (*equal)(const void* a, const void* b);
  void     (*moveConstructEntry)(void* dst, void* src);
  void     (*destroyEntry)(void* entry);
};
struct BucketState { enum Value : uint8_t { Empty = 0, Occupied = 1, Tombstone = 2 }; };
```

Two parallel `RawBuffer`s: one byte of state per slot, plus `entrySize` bytes per
slot. `INITIAL_CAPACITY = 8`, capacity always a power of two (mask-based
wraparound), `NPOS = size_t(-1)`.

```cpp
explicit HashMap(const TypeOps*) noexcept;      // move-only
Iterator begin()/end() noexcept;
Iterator find(const void* key) noexcept;
bool     contains(const void* key) const noexcept;
void*    insert(void* entrySrc);                // moves FROM caller storage; overwrites on dup key
bool     erase(const void* key) noexcept;       // leaves a Tombstone
void     clear() noexcept;                      // destroys entries, resets states, KEEPS capacity
size_t   size()/capacity() const noexcept;  bool empty() const noexcept;
```

Behaviour to keep in mind:

- Growth trigger is `(m_size + 1) * 4 >= m_capacity * 3`, doubling each time.
  `rehash` drops tombstones and never re-checks equality (it assumes the source
  is already deduplicated).
- `insert` takes a pointer to a **fully constructed** `Entry` in caller storage
  and move-constructs from it; on return the caller's object is moved-from but
  still needs destroying. Duplicate key ⇒ old entry destroyed, new one moved in,
  `m_size` unchanged.
- `~HashMap` calls `destroyAllEntries()` only; the buffers are released by
  `RawBuffer`'s own destructor. The private `reset()` is currently unreferenced.
- `entryAlign` is stored in `TypeOps` but never used — over-aligned entry types
  will be under-aligned (see `RawBuffer` above).
- Iterators are raw index-into-owner; any insert that rehashes invalidates them.

### `Hasher<K>`

Default is a byte-wise `FNV1A64(&key, sizeof(K))`, guarded by
`static_assert(std::is_trivially_copyable_v<K>)`. A specialisation for
`BasicString<CharT>` hashes the characters instead, so `TypedHashMap<String, V>`
works. Any other key with indirection needs its own specialisation.

### `TypedHashMap<K, V, Hash = Hasher<K>>`

```cpp
using Entry = Pair<K, V>;

TypedHashMap() noexcept;                        // move-only; copy is = delete
TypedHashMap(std::initializer_list<Pair<K,V>>); // requires K and V copy-constructible

Iterator begin()/end() noexcept;                // Entry& / Entry* on deref
Iterator find(const K&) noexcept;
bool     contains(const K&) const noexcept;
Iterator insert(const K&, V&&);                 // + const V& overload
template <Args...> Iterator emplace(const K&, Args&&...);
bool     erase(const K&) noexcept;
void     clear() noexcept;
size_t   size() const noexcept;  bool empty() const noexcept;
```

- **No `operator[]`.** Look up with `find` and compare against `end()`.
- `insert`/`emplace` build a temporary `Entry` on the stack, hand its address to
  `HashMap::insert`, then do a **second lookup** to build the returned iterator.
  Two probes per insert.
- `emplace(key, args...)` constructs `V(args...)` eagerly, then moves — it is not
  the in-place construction the name implies.
- Move-only `V` works with `insert`/`emplace` but not with the initializer-list
  constructor. In-engine use is exactly this: `TypedHashMap<UUID, UniquePtr<Object>>`
  in `scene.h`, `TypedHashMap<UUID, UniquePtr<Behavior>>` in `object.h`.

Verified working end-to-end: insert, duplicate-key overwrite, `find`, range-`for`
iteration, `erase`, `contains`, with `String` keys.

---

## `memory.h` — smart pointers

### `UniquePtr<T>`

`std::unique_ptr` minus the deleter. Single `T* m_ptr` member.

```cpp
constexpr UniquePtr() noexcept;  constexpr UniquePtr(std::nullptr_t) noexcept;
explicit UniquePtr(T*) noexcept;
template <U> UniquePtr(UniquePtr<U>&&);         // converting move, if U* → T*
~UniquePtr();                                   // plain `delete m_ptr`

T* get() [[nodiscard]];  // + const overload returning const T*
T& operator*() const;  T* operator->() const;
explicit operator bool() const noexcept;
void reset(T* = nullptr) noexcept;
T*   release() [[nodiscard]] noexcept;
void swap(UniquePtr&) noexcept;

template <T, Args...> UniquePtr<T> MakeUnique(Args&&...);
template <T> void Swap(UniquePtr<T>&, UniquePtr<T>&) noexcept;
// ==/!= against another UniquePtr and against nullptr
```

Destruction is `delete`, not `delete[]` — **no array specialisation**. A base-class
`UniquePtr<Base>` needs a virtual destructor on `Base` (which is what `Object` and
`Behavior` provide).

### `SharedPtr<T>` / `WeakPtr<T>`

Intrusive-ish control block, allocated separately from the object:

```cpp
struct ControlBlock { size_t strong_count{1}, weak_count{0}; T* m_ptr; };
```

`MakeShared` is `SharedPtr<T>(new T(...))` — two allocations, not the single
combined one `std::make_shared` does. Counts are plain `size_t`, **not atomic**:
these are single-thread-only.

```cpp
// SharedPtr
explicit SharedPtr(T*);  SharedPtr(const SharedPtr&) noexcept;  SharedPtr(SharedPtr&&) noexcept;
T* get() const [[nodiscard]];  T& operator*() const;  T* operator->() const;
size_t use_count() const;  bool unique() const;  explicit operator bool() const;
void reset();  void reset(T*);  void swap(SharedPtr&);
template <T, Args...> SharedPtr<T> MakeShared(Args&&...);

// WeakPtr
WeakPtr(const SharedPtr<T>&) noexcept;          // + copy/move, + operator= from SharedPtr
bool expired() const;  size_t use_count() const;
SharedPtr<T> lock() const [[nodiscard]];        // empty SharedPtr if expired
void reset();  void swap(WeakPtr&);
```

- Last strong reference deletes the object; the control block survives until the
  weak count also hits zero.
- No converting constructors between `SharedPtr<Derived>` and `SharedPtr<Base>`,
  no aliasing constructor, no `SharedPtr(UniquePtr&&)`, no `enable_shared_from_this`.
- `WeakPtr<T>` reaches into `SharedPtr<T>::ControlBlock`, so `WeakPtr<Base>` from
  `SharedPtr<Derived>` does not work.

---

## `bigint.h` — 128-bit integers

```cpp
#if defined(__clang__) || defined(__GNUC__)
  #define INT_128_EXISTS
  using int128_t  = __int128_t;      // global namespace, not ROSE::
  using uint128_t = __uint128_t;
#else
  #error "THIS IS NOT A FUNCTIONAL IMPLEMENTATION!"
#endif
```

The `#else` branch holds a partial hand-written `ROSE::uint128_t` (add, sub, mul,
bitwise, shifts, `<=>`, a bit-by-bit `divmod`) but `operator/=` is commented out
and much of `int128_t` is unimplemented — hence the hard `#error`. **MSVC cannot
compile this header.**

### Literals and parsing

```cpp
constexpr uint128_t parse128(const char* str, size_t idx);   // consteval-only: it throws
constexpr int128_t  operator""_lll (const char*);            // handles a leading '-'
constexpr uint128_t operator""_ulll(const char*);
constexpr int128_t  operator""_128 (const char*);            // aliases for the above
constexpr uint128_t operator""_u128(const char*);
```

`parse128` handles `0x…` hex, `0b…` binary, `0…` octal and plain decimal, skips
`'` digit separators, and `throw`s `std::invalid_argument` on a bad digit — which
is why it must never run at runtime. Clang emits a spurious
"cannot convert unsigned long long to const char*" on these literals; the header
says to ignore it.

### Formatting

Full `std::formatter<uint128_t>` and `std::formatter<int128_t>` specialisations
supporting `d`/`x`/`X`/`b`/`B`/`o`, `#` alt-form, `0` zero-pad, width, and (signed
only) `+`/space sign modes. Verified: `std::format("{:#x}", FNVPRIME128)` gives
`0x1000000000000000000013b`.

---

## Quick "does the RTL have…?" table

| Want | Answer |
|---|---|
| `std::vector` | `List<T>` — but no `insert`/`erase`/`front` |
| `std::array` | `FixedArray<T,N>` — no default ctor |
| `std::string` | `String` — no `find`/`substr`/`clear` |
| `std::string_view` | `StringView` — `c_str()` may not be NUL-terminated |
| `std::unordered_map` | `TypedHashMap<K,V>` — no `operator[]` |
| `std::map` / ordered | nothing |
| `std::unique_ptr` | `UniquePtr<T>` — no array form, no deleter |
| `std::shared_ptr` | `SharedPtr<T>` — non-atomic counts |
| `std::optional` / `variant` / `span` / `function` | nothing |
| `std::pair` | `Pair<T1,T2>` |
| `__int128` | `int128_t` / `uint128_t`, Clang/GCC only |
| deque, list, set, algorithms | nothing |

See [`known-issues.md`](known-issues.md) before relying on `List::operator==`,
`String::resize`, or `FixedArray`'s default constructor.

# Known issues — RTL and math

Defects and sharp edges found while surveying the headers on **2026-07-23**
(`master` @ `8510b49`). Every item below was **verified by compiling and/or
running it**, not inferred from reading. This file is a record of the trip
hazards, not a changelog — an item that gets fixed is struck through and kept
only where the shape of the old problem still explains the code around it.

Ordered roughly by how likely they are to bite.

---

## 1. `Vec::operator[]` does not compile in a `_DEBUG` build

`include/ROSE/Core/math/vector.h` — every specialisation, e.g. lines 225-226.

```cpp
constexpr T &operator[](const size_t idx) {
  ROSE_ASSERT(idx < N);
  ROSE_ASSERT_MSG(data != nullptr, "Vector storage must not be null");
  return data[idx];
}
```

`data` is a `FixedArray<T,N>`, not a pointer, so `data != nullptr` is ill-formed.
It survives in release only because `ROSE_ASSERT_MSG` expands to `((void)0)` and
the expression is never parsed.

Two separate failures fall out of this:

```sh
# (a) vector.h uses ROSE_ASSERT but never includes macros.h
$ printf '#include <ROSE/Core/math.h>\nusing namespace ROSE::math;\nint main(){Vec3f v(1,2,3);return (int)v[0];}' > t.cpp
$ clang++ -std=c++20 -I include -fsyntax-only t.cpp
vector.h:225:7: error: use of undeclared identifier 'ROSE_ASSERT'

# (b) with macros.h included first, it still fails once _DEBUG is on
$ clang++ -std=c++20 -D_DEBUG -I include -fsyntax-only t.cpp   # (t.cpp now includes macros.h first)
vector.h:226:28: error: invalid operands to binary expression ('FixedArray<float, N>' and 'std::nullptr_t')
```

Without `_DEBUG` and with `macros.h` first, it compiles clean.

**Why it has gone unnoticed:** nothing in the engine calls `Vec::operator[]` —
`paramview.cpp` and `scene.cpp` both use `.x/.y/.z`. And `CMakeLists.txt:129`
defines `_DEBUG` only `PRIVATE` on `ROSE_Core`, so example and tool targets never
see it.

**Fix:** include `<ROSE/Core/macros.h>` in `vector.h`, and drop the second assert
(or make it `data.data() != nullptr`, though for a `FixedArray` it can never be
null). Applies to all four copies.

---

## 2. ~~`FixedArray` has no default constructor, so `Mat` and `Vec<T,N≥5>` cannot be constructed~~ — FIXED

**Fixed in `include/ROSE/Core/array.h`.** Kept here because the shape of the
problem explains a lot of the surrounding code.

Declaring the array-reference and `initializer_list` constructors suppressed the
implicit default constructor, which propagated to every aggregate holding one, so
`FixedArray<int,3> a;`, `Vec<float,5> v;` and `Mat<float,4,4> m;` were all "call
to implicitly-deleted default constructor". `Mat` had no other constructor, so no
`Mat` could be created at all. (`Vec2f`/`Vec3f`/`Vec4f` escaped it — their own
constructors have defaulted params.)

Separately the `initializer_list` constructor was ill-formed if ever
instantiated, because `list.size()` is not a constant expression:

```cpp
static_assert(list.size() == N, "Size mismatch");   // ill-formed
```

`array.h` now has `constexpr FixedArray() noexcept = default;`, `const` overloads
of `data()`/`size()`, mutable `begin()`/`end()`, and a runtime `ROSE_ASSERT_MSG`
in place of that `static_assert`. It includes `<ROSE/Core/macros.h>` for the
assert. Elements are still left **default-initialized**, like `std::array` —
`FixedArray<int,3> a;` gives you three indeterminate ints, so assign before
reading. `Mat`'s own default constructor zeroes explicitly rather than relying on
this.

Still true: `Mat::col()`/`row()` return a `Vec`, and `Vec` is constrained to
`N > 1`, so those two are constrained `requires(Rows > 1)` / `requires(Cols > 1)`
rather than existing for every shape.

---

## 3. `Vec4`'s constructor only accepts two components

`include/ROSE/Core/math/vector.h:261` — copy-pasted from the `Vec<T,2>` case:

```cpp
constexpr Vec(T _x = T {}, T _y = T {}) : x(_x), y(_y) {}
```

`z` and `w` are not in the member-init list, so they are left indeterminate, and
`Vec4f(1.f,2.f,3.f,4.f)` is a compile error:

```
Vec4f v(1.f,2.f,3.f,4.f);  ->  error: no matching constructor for initialization of 'Vec4f'
```

`Quat`'s `explicit Quat(Vec4<T>)` constructor therefore reads two indeterminate
values. (In a quick test the uninitialised components happened to read as `0.0`,
but nothing guarantees that.)

**Fix:** `constexpr Vec(T _x = T{}, T _y = T{}, T _z = T{}, T _w = T{}) : x(_x), y(_y), z(_z), w(_w) {}`.
While in there, `Vec<T,2>` also leaves nothing uninitialised but `Vec<T,3>` uses
`T{0}` where 2 and 4 use `T{}` — worth making consistent.

---

## 4. `List::operator==` compares the wrong number of bytes

`include/ROSE/Core/list.h:239`:

```cpp
const bool operator==(const List &rhs) {
  return m_count == rhs.m_count && memcmp(m_buffer.data(), rhs.m_buffer.data(), m_count) == 0;
}
```

`m_count` is an **element** count but `memcmp` wants **bytes**. For any `T`
larger than one byte this compares only the first `m_count / sizeof(T)` elements
and reports unequal lists as equal.

```
List<int> d{1,2,3,4,5}, e{1,2,3,4,5};  d[4] = 999;
d == e   ->  1     (expected 0; only the first 5 of 20 bytes are compared)
```

The byte-wise comparison is also wrong for any `T` with padding or indirection —
`List<String>` would compare pointers.

Two more problems in the same line: the method is non-`const`, so every call site
gets `-Wambiguous-reversed-operator` under C++20; and the `const bool` return
type is meaningless.

**Fix:**

```cpp
bool operator==(const List &rhs) const {
  if (m_count != rhs.m_count) return false;
  for (size_t i = 0; i < m_count; ++i)
    if (!(data()[i] == rhs.data()[i])) return false;
  return true;
}
```

---

## 5. `BasicString::resize` writes one byte past the allocation

`include/ROSE/Core/string.h:132`:

```cpp
void resize(size_t newSize) {
  if (newSize > m_capacity) reserve(newSize + 1);   // should be newSize + 1 > m_capacity
  ...
  m_data[m_size] = CharT(0);                        // index newSize, buffer is m_capacity long
}
```

When `newSize == m_capacity` the guard does not fire, and the terminator is
written at `m_data[m_capacity]` — a one-byte heap overflow.

```
String s = "hello";   // size 5, capacity 6
s.resize(6);          // no reallocation; writes m_data[6] into a 6-byte block
```

It does not crash in practice (allocator slack absorbs it) but it is a real
out-of-bounds write.

**Fix:** `if (newSize + 1 > m_capacity) reserve(newSize + 1);`

---

## 6. `std::formatter<Vec<T,N>>` does not compile

`include/ROSE/Core/math/vector.h:447` — `format_cartesian` does
`*--out++ = ')';` on an output iterator that is not decrementable:

```
error: cannot decrement value of type 'back_insert_iterator<std::_Fmt_buffer<char>>'
```

`format()` calls both `format_tuple` and `format_cartesian` from a `switch`, so
**both branches instantiate** and *any* `std::format("{}", vec)` fails to
compile, not just the `c` form.

Even if that were fixed, the formatter has more wrong with it:

- `format()` emits `'('` itself and *then* calls `format_tuple`, which emits
  another `'('` — double parentheses, and the closing paren doubles too.
- `parse()` never advances `it` in its flag loop, so any flag spins forever;
  only an immediately-terminating spec escapes.
- `naked`, `multiline`, `verbose` and `incZero` are declared but no flag ever
  sets them, and `verbose` is never read.
- `format_cartesian` picks between `coordbuf`/`quatstylebuf` into a local `data`
  that it then never uses (`-Wunused-but-set-variable`), and indexes
  `quatstylebuf` directly instead.
- The loop condition `i < N && (val[i] != 0 || incZero)` stops at the first zero
  component rather than skipping it.
- `format_cartesian` also calls `val.operator[](i)`, which drags in issue #1.

The `Comp`, `BasicString`, `uint128_t` and `int128_t` formatters all work
correctly — `Vec` is the only broken one.

**Fix:** rewrite against `format_to`/`std::copy` on a plain output iterator; the
`Comp` formatter in `complex.h` is a good model.

---

## 7. `Comp::explicit operator T()` has an empty body

`include/ROSE/Core/math/complex.h:143`:

```cpp
explicit operator T() const {}
```

Falling off the end of a non-`void` function is UB. Clang warns
(`-Wreturn-type`) and the generated code traps:

```
float f = static_cast<float>(Compf(3.f, 4.f));   ->  Illegal instruction (exit 132)
```

**Fix:** either delete the operator or make it return `Re` (and say so — the
intuitive reading is "magnitude", which would be `hypot(Re, Im)`).

---

## 8. `PI`, `E`, `PHI`, `TAU` only carry float precision

`include/ROSE/Core/math/constants.h:18-21` — declared `double` but initialised
from `float` literals:

```cpp
constexpr double PI  = 3.141592653589793f;   // f suffix rounds to float, then widens
constexpr double TAU = 2.f * PI;             // inherits the error
```

Measured:

```
math::PI = 3.14159274101257324219
true π   = 3.14159265358979311600     -> error ≈ 8.7e-8
```

`SQRT2` is written without the suffix and is correct to full double precision,
which is what makes the other four look like slips rather than intent.

An 8.7e-8 relative error is invisible in float rendering maths but will show up
in anything accumulating over time — the `Vec3d`/`Quatd` physics path in
`motion.h` is exactly the kind of code that would.

**Fix:** drop the `f` suffixes; make `TAU` `2.0 * PI`.

---

## 9. `HashMap` ignores the alignment it records

`rtl/hashmap.cpp` never reads `TypeOps::entryAlign`, and `RawBuffer` allocates
through plain `::operator new`, which only guarantees
`__STDCPP_DEFAULT_NEW_ALIGNMENT__` (16 bytes on x64). A `TypedHashMap<K,V>` whose
`Pair<K,V>` is over-aligned (`alignas(32)`, an AVX type, …) gets under-aligned
storage.

Nothing in the engine hits this today — the live instantiations are
`TypedHashMap<UUID, UniquePtr<Object>>` and `TypedHashMap<UUID, UniquePtr<Behavior>>`,
both 16-byte-aligned or less. Given `CMakeLists.txt` builds with `-mavx2 -mfma`,
it is worth remembering before someone puts a `Vec` batch in a map.

**Fix:** give `RawBuffer` an aligned `allocate(bytes, align)` using the aligned
`::operator new(size, std::align_val_t)`, and have `rehash` pass
`m_ops->entryAlign`.

---

## 10. `MemCmp` cannot be constant-evaluated on two string literals

`include/ROSE/Core/utility.h:120` — the early-out compares the two pointers:

```cpp
template <typename T>
constexpr int MemCmp(const T *a, const T *b, size_t count) noexcept {
  if (count == 0 || a == b) return 0;
```

Comparing the addresses of two *distinct* string literals is not a constant
expression. The implementation is free to merge identical literals into one
object or keep them separate, so `a == b` has no answer the compiler is willing
to commit to, and the whole call stops being usable in a constant expression:

```cpp
constexpr const char *Directive() { return "#version 450 core"; }
static_assert(MemCmp(Directive(), "#version 450 core", 17) == 0);
```

```
error: static assertion expression is not an integral constant expression
utility.h(120,25): note: comparison of addresses of potentially overlapping
                         literals has unspecified value
```

Found **2026-08-09** writing the GLSL version table in `openglrenderer.cpp`,
where the natural way to pin a table of string constants down is a `static_assert`
over them — which is exactly the shape that fails.

Runtime calls are unaffected, and so is constant evaluation whenever the compiler
*can* decide the comparison: two pointers into the same array, or either operand
null. It is only literal-vs-literal that breaks, and the `constexpr` on the
signature advertises support for it.

**Fix:** delete `a == b` from the condition, or keep it only where it is legal:

```cpp
if (count == 0) return 0;
if (!std::is_constant_evaluated() && a == b) return 0;
```

The early-out saves one pass over a loop that is already O(count), so dropping it
outright costs close to nothing. `<type_traits>` is already in scope — the same
function uses `std::is_integral_v` two lines down.

---

## 11. Smaller things

| Where | What |
|---|---|
| `quaternion.h:84` | `Quat::ToEuler` is a stub; every branch returns `{}` |
| `quaternion.h:47,58` | `AxisAngle`/`FromEuler` are `constexpr` but call `std::sin`/`std::cos`, so they can never be constant-evaluated before C++26 |
| `utility.h:59` | `SmartMemCpy(dst, src, count)` ignores `count` entirely; it copies `Min(sizeof(T), sizeof(U))` bytes once |
| `string.h:173` | `at()`'s bounds check is commented out — it is `operator[]` with a different name |
| `bigint.h:27` | The non-`__int128` fallback is a hard `#error`, so MSVC cannot compile the RTL at all |
| `mathfunctions.h` | `Sqrt` (and `Sin`/`Cos`/`Tan`) rely on `__builtin_*` — `__builtin_sqrt`/`sin`/`cos`/`tan`, `__builtin_bit_cast`, `__builtin_is_constant_evaluated` — with no MSVC path |
| `mathenum.h:62` | `LeviCivita`'s `static_assert` message still says `cse::math::` |
| `utility.h` vs `mathfunctions.h` | Two different `Min`/`Max` pairs; ambiguous if both namespaces are in scope |
| `hashmap.h:306` | `TypedHashMap::insert`/`emplace` probe twice per insert (once to insert, once to build the returned iterator) |
| `hashmap.cpp:252` | private `HashMap::reset()` is never called |
| `list.h`, `string.h` | `begin()`/`end()` on `BasicString` are const-only, so no mutable range-`for` over a string |

---

## Verification recipe

```sh
# math / RTL header-only checks
clang++ -std=c++20 -I include -fsyntax-only t.cpp
clang++ -std=c++20 -D_DEBUG -I include -fsyntax-only t.cpp

# runtime, RTL containers + smart pointers + math
clang++ -std=c++20 -w -I include t.cpp src/Core/rtl/buffer.cpp -o t.exe

# runtime, anything using HashMap (see README.md for the sed + FNV shim)
clang++ -std=c++20 -w -I include t.cpp hashmap_local.cpp fnv_shim.cpp \
        src/Core/rtl/buffer.cpp -o t.exe
```

# Known issues — RTL and math

Defects and sharp edges found while surveying the headers, first on **2026-07-23**
(`master` @ `8510b49`) and re-checked on **2026-09-02** (`master` @ `de3eafa`).
Every item below was **verified by compiling and/or running it**, not inferred
from reading. This file is a record of the trip hazards, not a changelog — an item
that gets fixed is struck through and kept only where the shape of the old problem
still explains the code around it.

Ordered roughly by how likely they are to bite.

---

## 1. ~~`Vec::operator[]` does not compile in a `_DEBUG` build~~ — FIXED

**Fixed by the `vector.h` rewrite.** Kept because the layering half of it is why
`README.md` used to call the `ROSE.h` include order load-bearing.

`operator[]` carried a second assert, `ROSE_ASSERT_MSG(data != nullptr, …)`, on a
`data` that is a `FixedArray<T,N>` and not a pointer — ill-formed, and invisible in
release only because `ROSE_ASSERT_MSG` expands to `((void)0)` so the expression is
never parsed. Two failures fell out of it: `vector.h` spelled `ROSE_ASSERT` without
including `macros.h`, so `#include <ROSE/Core/math.h>` on its own failed with *use
of undeclared identifier*; and with `macros.h` included first it still failed under
`_DEBUG`, on the `FixedArray`-versus-`nullptr_t` comparison.

Both are gone. `vector.h` now includes `<ROSE/Core/rtl.h>`, which reaches `macros.h`
through `array.h`, and the null assert is deleted — only `ROSE_ASSERT(idx < N)` is
left. Verified at `de3eafa` with a two-line `t.cpp`:

```cpp
#include <ROSE/Core/math.h>
int main() { ROSE::math::Vec3d a(1, 2, 3); return (int)a[0]; }
```

```sh
$ clang++ -std=c++20 -fsyntax-only -I include t.cpp             # clean
$ clang++ -std=c++20 -fsyntax-only -D_DEBUG -I include t.cpp    # clean
```

Still true, and still worth knowing: `CMakeLists.txt:403` defines `_DEBUG` only
`PRIVATE` on `ROSE_Core`, under `$<$<CONFIG:Debug>:…>`, so assertions are compiled
out in every example and tool target.

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

## 3. ~~`Vec4`'s constructor only accepts two components~~ — FIXED

**Fixed by the `vector.h` rewrite**, which replaced the four hand-written
specialisations with one `Vec<T,N>` sitting on a `detail::VecStorage<T,N>` base.
Only the storage layout and its constructors vary with `N` now, so the constructor
can no longer drift between the sizes the way it had:

```cpp
template <Scalar T> struct VecStorage<T, 4> {
  union { FixedArray<T, 4> data; struct { T x, y, z, w; }; };
  constexpr VecStorage() noexcept : data {} {}
  explicit constexpr VecStorage(NoInit) noexcept {}
  constexpr VecStorage(T _x, T _y = T {}, T _z = T {}, T _w = T {}) noexcept : data { _x, _y, _z, _w } {}
};
```

`Vec4f(1,2,3,4)` compiles and reads back `4` in `w`, so `Quat`'s
`explicit Quat(Vec4<T>)` no longer reads indeterminate components. Two related
notes from the rewrite:

- The component constructors initialise `data`, which makes the array the **active
  union member**. That is deliberate: it is what lets the whole-vector operations
  fold in a constant expression, at the cost of `v.x` not being readable in one.
  Reading `x`/`y`/`z`/`w` still works at runtime, as an extension every compiler
  implements. `matrix.h`'s note about `Vec`-taking members not being `constexpr` is
  the mirror image of this and is now obsolete.
- `VecStorage<T,3>` stores **four** elements and names the fourth `w` for SIMD
  padding; the generic case stores `NextPow2(N)`. `sizeof(Vec3f)` is 16, not 12 —
  do not assume tight packing when handing a `Vec3` array to a graphics API.
- There is an `explicit VecStorage(NoInit)` tag constructor that skips
  zero-initialisation. The default constructor still zeroes.

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

`include/ROSE/Core/string.h:145`:

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

## 6. ~~`std::formatter<Vec<T,N>>` does not compile~~ — FIXED

**Rewritten**, and now the most capable formatter in the tree. The old one
decremented a `back_insert_iterator`, emitted doubled parentheses, never advanced
`it` in `parse()`'s flag loop, and declared four flags nothing set — so *every*
`std::format("{}", vec)` failed to compile, not only the `c` form.

The replacement documents its own grammar, `{:[flags][|scalar-spec]}`:

| Flag | Meaning |
|---|---|
| `t` | tuple form, `(1, 2, 3)` — the default |
| `c` | cartesian form, `(1x -2y)`; only defined for `N <= 4` |
| `q` | label components `i`/`j`/`k` instead of `x`/`y`/`z`; cartesian only |
| `n` | naked: no enclosing parentheses |
| `m` | one component per line |
| `z` | keep zero components, which cartesian form drops by default |

Anything past the `|` is the spec each component is formatted with. Verified at
`de3eafa` against `Vec3d(1, -2, 0)`:

```
{}        -> (1, -2, 0)
{:c}      -> (1x -2y)
{:cz}     -> (1x -2y +0z)
{:cq}     -> (1i -2j)
{:n}      -> 1, -2, 0
{:c|.3f}  -> (1.000x -2.000y)
```

`parse()` throws `std::format_error` on an unknown flag, on `c` with `N > 4`, and
on `q` without `c`. That is inside the no-exceptions rule — `std::formatter::parse`
is one of the sanctioned sites (see `conventions.md` §5).

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

## 8. ~~`PI`, `E`, `PHI`, `TAU` only carry float precision~~ — FIXED

`include/ROSE/Core/math/constants.h` — the `f` suffixes are gone and `TAU` is
`2. * PI`, so all five constants now carry full double precision:

```cpp
constexpr double PI    = 3.14159265358979323846;
constexpr double E     = 2.71828182845904523536;
constexpr double PHI   = 1.61803398874989484820;
constexpr double TAU   = 2. * PI;
constexpr double SQRT2 = 1.41421356237309504880;
```

Kept because it is the reason to be suspicious of a bare `f` suffix on anything
feeding the `Vec3d`/`Quatd` path in `motion.h`: the old 8.7e-8 relative error was
invisible in float rendering maths and would only have surfaced as drift once it
accumulated.

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

`include/ROSE/Core/utility.h:121` — the early-out compares the two pointers:

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

Re-checked at `de3eafa`; line numbers are current.

| Where | What |
|---|---|
| `utility.h:108` | `SmartMemCpy(dst, src, count)` ignores `count` entirely; it copies `Min(sizeof(T), sizeof(U))` bytes once. Carries its own `// TODO` |
| `string.h:186` | `at()`'s bounds check is commented out — it is `operator[]` with a different name |
| `bigint.h:28` | The non-`__int128` fallback is a hard `#error`, so MSVC cannot compile the RTL at all |
| `mathfunctions.h` | `Sqrt` (and `Sin`/`Cos`/`Tan`) rely on `__builtin_*` — `__builtin_sqrt`/`sin`/`cos`/`tan`, `__builtin_bit_cast`, `__builtin_is_constant_evaluated` — with no MSVC path |
| `mathenum.h:63` | `LeviCivita`'s `static_assert` message still says `cse::math::` |
| `utility.h` vs `mathfunctions.h` | Two different `Min`/`Max` pairs; ambiguous if both namespaces are in scope |
| `hashmap.h:270-287` | `TypedHashMap::insert`/`emplace` probe twice per insert — `m_map.insert(&tmp)` then `m_map.find(&_key)` to build the returned iterator |
| `hashmap.cpp:252` | private `HashMap::reset()` is still never called |
| `list.h`, `string.h` | `begin()`/`end()` on `BasicString` are const-only, so no mutable range-`for` over a string |
| `vector.h:265-268` | The free `operator*(T lhs, const Vec<T,N> &rhs)` is declared to return `T` but returns `rhs * lhs`, a `Vec`. Being a template it only fails on first instantiation, so `2.0 * someVec3d` is `error: no viable conversion from returned value of type 'Vec<double, 3>' to function return type 'double'`. Fix the return type to `Vec<T,N>` |
| `tuple.h` | `Get<I>` has no `const` overload, so `Get<0>(someConstTuple)` does not compile. There is no `MakeTuple`, no comparison, and no structured-binding support |

### Fixed since the last pass

| Where | What |
|---|---|
| `quaternion.h` | `Quat::ToEuler` was a stub returning `{}` from every branch. Implemented in `6706679`, recovered from the rotation matrix, with a per-type `kGimbalEpsilon` at `sqrt(machineEpsilon)` and a comment explaining why you must not tune it by round-tripping random rotations |
| `mathfunctions.h:95` | `SinCosConst`'s comment used to claim a couple of ulps. It now says "near-double accuracy", which is honest: measured worst absolute error is **3.888e-13**, at the ends of the reduced range near π/4. The series is unchanged (r¹³ for sin, r¹² for cos); two more terms in each polynomial would still reach full double precision |

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

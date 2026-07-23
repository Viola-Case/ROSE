# Math — `ROSE::math`

`include/ROSE/Core/math.h` pulls in, in this order:

```cpp
math/mathenum.h  math/vector.h  math/matrix.h  math/complex.h
math/quaternion.h  math/mathfunctions.h  math/constants.h
```

Everything lives in **`ROSE::math`**, not `ROSE`.

## What is re-exported into `ROSE`

`math.h` ends with exactly ten `using` declarations:

```cpp
Compd Compf  Quatd Quatf
Vec2d Vec2f  Vec3d Vec3f  Vec4d Vec4f
```

`using math::Mat;` is present but **commented out**. So:

- ✅ `ROSE::Vec3d`, `ROSE::Quatf`, … work unqualified inside `namespace ROSE`.
- ❌ `Mat`, the `Vec2`/`Vec3`/`Vec4` template aliases, `Sign`, `EulerOrder`,
  `Sqrt`, `Clamp`, `PI`, `LeviCivita` all need a `math::` qualifier.

Where the engine actually uses these: `Transform` (`Vec3d position/scale`,
`Quatd rotation`), `mesh.h` (`Vec3d position/normal`), `motion.h` (four `Vec3d`
derivatives), `input.h` (`Vec2f GetStickAxes`), `paramview.h` (`GetVec3d`).
Note the engine standardises on **double** for spatial data and float only for
input axes.

---

## `mathenum.h`

```cpp
struct Sign {
  enum Value : int8_t { Negative = -1, Zero = 0, Positive = 1 } value;
  constexpr Sign(Value);
  constexpr Sign(float);                       // implicit — 0 → Zero, >0 → Positive, else Negative
  constexpr operator int8_t() const noexcept;
};

constexpr Sign SignOf(float) noexcept;
constexpr bool IsPositive(Sign, bool incZero = true)  noexcept;   // note asymmetric defaults
constexpr bool IsNegative(Sign, bool incZero = false) noexcept;

template <typename T>      constexpr bool KDelta(T a, T b);       // Kronecker delta; just `a == b`
template <typename... A>   constexpr Sign LeviCivita(A... args);  // permutation sign
```

`LeviCivita` `static_assert`s that every argument is convertible to `size_t`,
returns `Zero` unless the arguments are exactly a permutation of `0..N-1`, and
otherwise counts inversions. The error message still says `cse::math::` from a
previous name for the library.

`Sign`'s `float` constructor is implicit, so `Sign s = 3.5f;` compiles.

---

## `vector.h` — `Vec<T, N>`

```cpp
template <Scalar T, size_t N> requires (N > 1) struct Vec;
```

A primary template plus **full specialisations for N = 2, 3, 4**. The
specialisations wrap an anonymous union giving both `data` (a `FixedArray<T,N>`)
and named components:

| N | components | constructor |
|---|---|---|
| 2 | `x, y` | `Vec(T _x = T{}, T _y = T{})` |
| 3 | `x, y, z` | `Vec(T _x = T{0}, T _y = T{0}, T _z = T{0})` |
| 4 | `x, y, z, w` | `Vec(T _x = T{}, T _y = T{})` ← **only two parameters** |

The N = 4 constructor is a copy-paste of the N = 2 one: you cannot write
`Vec4f(1,2,3,4)`, and `z`/`w` are never initialised. See `known-issues.md` #3.

### Common members (present on all four)

```cpp
constexpr T    dot(const Vec&) const noexcept;
constexpr Vec& operator+=/-=(const Vec&) noexcept;
constexpr Vec& operator*=(T) noexcept;                 // scalar only
constexpr Vec  operator+/-(const Vec&) const noexcept;
constexpr Vec  operator*(T) const noexcept;
constexpr T&   operator[](size_t);                     // + const overload; asserts idx < N
template <size_t I> requires (I < N) constexpr T& at() noexcept;
template <Scalar U> constexpr operator Vec<U, N>();    // element-wise static_cast; non-const
```

`Vec<T,3>` additionally has:

```cpp
constexpr Vec<T,3> cross(const Vec<T,3>&) const noexcept;   // verified correct
```

### Aliases

```cpp
template <Scalar T> using Vec2 = Vec<T,2>;   // Vec3, Vec4 likewise
using Vec2f/Vec2d/Vec3f/Vec3d/Vec4f/Vec4d = Vec<float|double, 2|3|4>;
```

### Missing

No `Length`/`Norm`/`Normalize`, no `operator-` (unary negate), no `operator/`,
no `operator==`, no `operator*=` by another `Vec` (component-wise), no
`operator*` with the scalar on the left, no `Lerp`, no swizzles. The conversion
operator is non-`const`, so a `const Vec3f` will not convert to `Vec3d`.

### Two hazards

1. `operator[]` **does not compile in a `_DEBUG` build** and does not compile at
   all without `macros.h` already included. The whole codebase uses `.x/.y/.z`
   instead. See `known-issues.md` #1.
2. The `std::formatter<Vec<T,N>>` specialisation at the bottom of the header
   **does not compile**, so `std::format("{}", someVec)` is unusable. See
   `known-issues.md` #6.

---

## `matrix.h` — `Mat<T, Rows, Cols>`

The least finished file in the library. A primary template plus a square
`Mat<T, Dimension, Dimension>` specialisation, both storing
`FixedArray<T, Rows*Cols>` in **row-major** order.

```cpp
static constexpr size_t size = Rows * Cols;
constexpr Mat() noexcept = default;                  // IMPLICITLY DELETED (see below)
constexpr Mat(const Mat&) noexcept = default;

constexpr Vec<T, Rows> col(size_t idx) const noexcept;
constexpr Vec<T, Cols> row(size_t idx) const noexcept;
constexpr T&   operator()(size_t row, size_t col) noexcept;   // non-const only
constexpr Mat& operator+=/-=(const Mat&) noexcept;
constexpr Mat  operator*(const Mat&) const noexcept;          // EMPTY BODY, no return
```

State of it:

- **Not default-constructible** — `FixedArray` has no default constructor, so
  `Mat<float,4,4> m;` fails to compile (`known-issues.md` #2). There is also no
  constructor that takes elements, so a `Mat` is currently impossible to create
  except by copy from another (unobtainable) `Mat`.
- `operator*` has an empty double loop and **no return statement**.
- The two specialisations are duplicated verbatim; only `Cols`/`Rows` constants
  differ.
- No identity, transpose, determinant, inverse, `Mat*Vec`, scalar multiply, or
  translation/rotation/projection builders.
- `col()`/`row()` return a `Vec`, which caps usable dimensions at 4 anyway (and
  requires the returned `Vec` to be default-constructible, which for N > 4 it is
  not).

Treat `Mat` as a placeholder. `Transform` deliberately stores position +
quaternion + scale rather than a matrix.

---

## `complex.h` — `Comp<T>`

```cpp
template <StdScalar T> struct Comp {
  union { struct { T Re, Im; }; T data[2]; };
};
using Compf = Comp<float>;
using Compd = Comp<double>;
```

Note the **capitalised** members `Re` and `Im`, and that the raw array here is a
plain `T data[2]`, not a `FixedArray` (unlike `Vec`).

```cpp
constexpr Comp() = default;                     // NOT zeroed — Re/Im indeterminate
constexpr Comp(T re);                           // implicit real → complex
constexpr Comp(T re, T im);
constexpr explicit Comp(Vec2<T>);               // R² → C

constexpr Comp& operator+=/-=/*=(const Comp&) noexcept;   // and against a bare T
constexpr Comp& operator/=(const Comp&) noexcept;         // no zero check
constexpr const Comp operator+/-/*//(const Comp&) const;  // and against a bare T
template <StdScalar U> constexpr operator Comp<U>() const;

explicit operator T() const {}                  // BROKEN: no return statement
```

Free operators for scalar-on-the-left: `operator+(const T&, const Comp<U>&)` and
`operator-(const T&, const Comp<T>&)`. There is **no** left-hand `*` or `/`.

### Literals

```cpp
constexpr Comp<long double>        operator""_i(long double);
constexpr Comp<unsigned long long> operator""_i(unsigned long long);
```

`constants.h` uses this as `constexpr Compd I = 0 + 1_i;`.

### Missing

No `abs`/`norm`/`arg`/`conj`, no `exp`/`log`/`pow`/`sqrt`, no `operator==`, no
unary minus. `explicit operator T()` is a trap — calling it traps at runtime
(`known-issues.md` #7).

### Formatting

`std::formatter<Comp<T>>` works and is the nicest formatter in the codebase.
Flags, combinable, before an optional `|` + nested scalar spec:

| flag | effect |
|---|---|
| `p` | wrap in parentheses |
| `z` / `Z` | emit a zero real / imaginary part even when it is zero |
| `v` | verbose — `Comp{Re=…, Im=…}` |
| `e` | Euler form — `mag e^(arg i)` |
| `c` | cis form — `mag cis(arg)` |
| `\|spec` | apply `spec` to each scalar |

`e`/`c` conflict and throw `std::format_error`. Both use `ROSE_HYPOT` /
`ROSE_ATAN2`, which default to `std::hypot` / `std::atan2` and can be overridden
by defining the macros first. Verified output for `Compf(1,-2)`:

```
{}         -> 1-2i
{:p}       -> (1-2i)
{:v}       -> Comp{Re=1, Im=-2}
{:|.3f}    -> 1.000-2.000i
```

Define `ROSE_MATH_NO_FORMAT` to drop the formatter (and its `<algorithm>`,
`<cmath>`, `<format>`, `<string>`, `<string_view>` includes).

---

## `quaternion.h` — `Quat<T>`

```cpp
enum class EulerOrder { XYZ, XZY, YXZ, YZX, ZXY, ZYX };

template <StdScalar T> struct Quat {
  union { struct { T w, x, y, z; }; T data[4]; };   // NOTE: w FIRST
};
using Quatf = Quat<float>;
using Quatd = Quat<double>;
```

Storage order is **w, x, y, z** — scalar first. Watch this when interoperating
with anything that uses xyzw.

```cpp
constexpr Quat() noexcept;                       // identity (1,0,0,0), unlike Vec/Comp
constexpr Quat(T w, T x = {}, T y = {}, T z = {}) noexcept;
constexpr Quat(Comp<T> c, T y = {}, T z = {}) noexcept;   // Re→w, Im→x
constexpr explicit Quat(Vec4<T>);                // takes vec.w as w

static constexpr Quat  AxisAngle(T angle, T ax, T ay, T az);   // angle in RADIANS, axis assumed unit
static constexpr Quat  FromEuler(Vec3<T>, EulerOrder = XYZ);
static constexpr Quat<T> Identity();

constexpr T     Norm() const noexcept;           // magnitude via math::Sqrt, NOT squared norm
constexpr Quat& Normalize() noexcept;            // in place; degenerate → Identity, never NaN
constexpr Quat  Normalized() const noexcept;
constexpr Quat& operator*=(const Quat&) noexcept;
constexpr Quat  operator*(const Quat&) const noexcept;   // Hamilton product

Vec3<T> ToEuler(EulerOrder = ZYX);               // STUB — every branch returns {}
```

Notes:

- `Norm()` is the **length**, not the squared length. `Normalize()` guards
  `!(n > 0)` (so NaN also falls through to identity).
- `AxisAngle` and `FromEuler` are marked `constexpr` but call `std::sin`/`std::cos`,
  which are not `constexpr` before C++26 — they only ever run at runtime. Verified
  correct at runtime: `AxisAngle(π/2, 0,0,1)` → `(0.7071, 0, 0, 0.7071)`, norm 1.
- `FromEuler` default order is `XYZ`; `ToEuler` default order is `ZYX`. They do
  not round-trip — and `ToEuler` returns a zero vector regardless.
- **Missing:** conjugate, inverse, dot, Slerp/Nlerp, vector rotation
  (`q * v * q⁻¹`), quaternion→matrix, `operator+`, `operator==`, scalar multiply.
  Rotating a point currently has to be written by hand at the call site.

---

## `mathfunctions.h`

```cpp
template <StdScalar T> constexpr T Clamp(T value, T min, T max) noexcept;

constexpr double Sqrt(double) noexcept;
constexpr float  Sqrt(float)  noexcept;

template <T> constexpr const T& Min(const T& a, const T& b) noexcept requires std::is_arithmetic_v<T>;
template <T> constexpr const T& Max(const T& a, const T& b) noexcept requires std::is_arithmetic_v<T>;
```

`Sqrt` branches on `__builtin_is_constant_evaluated()`: at runtime it is
`__builtin_sqrt(f)`, which lowers to a single hardware instruction; at compile
time it falls back to `detail::SqrtConst`, a Quake-style inverse-sqrt seed
(`SQRTMAGIC64` / `SQRTMAGIC32`) refined with Newton steps, then one classical
step to land on the rounded result. Handles ±0, NaN, negatives (→ NaN), infinity
(`MAXFINITE64` / `MAXFINITE32`), and rescales subnormals (`MINNORMAL*`,
`SUBNORMALSCALE*`, `SUBNORMALUNSCALE*`) into the normal range first. All of those
live in `constants.h`; `mathfunctions.h` includes it for them.

**Clang/GCC only** — the `__builtin_*` calls have no MSVC fallback.

Accuracy differs between the two paths. The runtime builtin is correctly rounded;
the constant-evaluated path is **not always** — `Sqrt(2.0)` folds to
`1.4142135623730949` at compile time against a correctly-rounded
`1.4142135623730951`, one ulp low. Exact powers of four are fine
(`static_assert(Sqrt(4.0) == 2.0)` holds). Don't `static_assert` a constant-folded
`Sqrt` against a decimal literal you got from elsewhere.

⚠️ `math::Min`/`Max` and `ROSE::Min`/`Max` (from `utility.h`) both exist, with
different signatures — `ROSE::` takes by value and is unconstrained, `math::`
takes by const reference and requires an arithmetic type. With both namespaces in
scope an unqualified call is ambiguous.

Missing: `Abs`, `Floor`/`Ceil`/`Round`, `Lerp`, `Pow`, trig, `ToRadians`/`ToDegrees`.

---

## `constants.h`

```cpp
constexpr double    PI    = 3.141592653589793f;   // ⚠ float literal, see below
constexpr double    E     = 2.718281828459045f;   // ⚠
constexpr double    PHI   = 1.618033988749895f;   // ⚠
constexpr double    TAU   = 2.f * PI;             // ⚠ inherits PI's error
constexpr double    SQRT2 = 1.41421356237309504;  // ✅ correct (no f suffix)
constexpr Compd     I     = 0 + 1_i;

constexpr uint32_t  FNVPRIME32   = 0x01000193;
constexpr uint64_t  FNVPRIME64   = 0x00000100000001b3;
constexpr uint32_t  FNVOFFSET32  = 0x811c9dc5;
constexpr uint64_t  FNVOFFSET64  = 0xcbf29ce484222325;
constexpr uint128_t FNVPRIME128  = 0x0000000001000000000000000000013B_u128;
constexpr uint128_t FNVOFFSET128 = 0x6C62272E07BB014262B821756295C58D_u128;

// float-format limits and inverse-sqrt seeds — used by detail::SqrtConst
constexpr double   MAXFINITE64 = 1.7976931348623157e308;   // DBL_MAX
constexpr float    MAXFINITE32 = 3.4028234663852886e38f;   // FLT_MAX
constexpr double   MINNORMAL64 = 2.2250738585072014e-308;  // DBL_MIN
constexpr float    MINNORMAL32 = 1.1754943508222875e-38f;  // FLT_MIN
constexpr uint64_t SQRTMAGIC64 = 0x5FE6EB50C7B537A9;
constexpr uint32_t SQRTMAGIC32 = 0x5F3759DF;               // the original Quake constant
constexpr double   SUBNORMALSCALE64   = 0x1p106;  // UNSCALE is 1/sqrt(SCALE),
constexpr double   SUBNORMALUNSCALE64 = 0x1p-53;  // so the pair cancels around the root
constexpr float    SUBNORMALSCALE32   = 0x1p50f;
constexpr float    SUBNORMALUNSCALE32 = 0x1p-25f;
```

**`PI`, `E`, `PHI` and `TAU` carry only float precision** despite being `double`
— the literals have an `f` suffix, so the value is rounded to float and then
widened. Measured: `PI == 3.14159274101257324219` against a true
`3.14159265358979311600`, an error of ~8.7e-8. Do not use them for anything that
needs double precision. Details in `known-issues.md` #8.

Only `constants.h` and `bigint.h` provide the FNV magic numbers; `utility.cpp`
and `hashmap.cpp` both reach into `math::` for them.

---

## Quick "does math have…?" table

| Want | Answer |
|---|---|
| vector add/sub/dot/cross | ✅ (cross is `Vec3` only) |
| vector length / normalize | ❌ write it by hand |
| vector `operator/`, unary `-`, `==` | ❌ |
| `Vec4` with four arguments | ❌ constructor only takes two |
| `vec[i]` | ⚠️ fails to compile in `_DEBUG` — use `.x/.y/.z` |
| `std::format` a `Vec` | ❌ formatter does not compile |
| matrices | ⚠️ placeholder; not even constructible |
| complex arithmetic + formatting | ✅ |
| quaternion product, normalize, axis-angle, from-euler | ✅ |
| quaternion inverse / slerp / rotate-a-vector / to-euler | ❌ |
| `constexpr` sqrt | ✅ `math::Sqrt` |
| clamp / min / max | ✅ (mind the `ROSE::` vs `math::` overload clash) |
| abs, floor, lerp, trig, deg↔rad | ❌ |
| π to double precision | ❌ `math::PI` is float-precision |

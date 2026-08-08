# Math — `ROSE::math`

`include/ROSE/Core/math.h` pulls in, in this order:

```cpp
math/mathenum.h  math/vector.h  math/matrix.h  math/complex.h
math/quaternion.h  math/mathfunctions.h  math/constants.h
```

Everything lives in **`ROSE::math`**, not `ROSE`.

## What is re-exported into `ROSE`

`math.h` ends with sixteen `using` declarations:

```cpp
Compd Compf  Quatd Quatf
Vec2d Vec2f  Vec3d Vec3f  Vec4d Vec4f
Mat2d Mat2f  Mat3d Mat3f  Mat4d Mat4f
```

So:

- ✅ `ROSE::Vec3d`, `ROSE::Mat4d`, `ROSE::Quatf`, … work unqualified inside
  `namespace ROSE`.
- ❌ The `Vec2`/`Vec3`/`Vec4` and `Mat2`/`Mat3`/`Mat4` template aliases, the `Mat`
  and `Vec` templates themselves, `Sign`, `EulerOrder`, `Sqrt`, `Clamp`, `PI` and
  `LeviCivita` all need a `math::` qualifier.

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

```cpp
template <Scalar T, size_t Rows, size_t Cols> requires (Rows > 0 && Cols > 0) struct Mat;
```

One template, no square specialisation — square-only operations carry
`requires(Rows == Cols)` and simply do not exist on a non-square instantiation.
Storage is a `FixedArray<T, Rows*Cols>` in **row-major** order: element (r, c) is
`data[r * Cols + c]`.

```cpp
using ValueType = T;
static constexpr size_t rowCount = Rows, colCount = Cols, size = Rows * Cols;

constexpr Mat() noexcept;                            // the ZERO matrix, not indeterminate
constexpr Mat(Args... args) noexcept;                // one arg per element, row-major; requires sizeof...==size

static constexpr Mat Zero()      noexcept;
static constexpr Mat Filled(T)   noexcept;
static constexpr Mat Identity()  noexcept requires (Rows == Cols);
static constexpr Mat Diagonal(const Vec<T,Rows>&) noexcept requires (Rows == Cols && Rows > 1);

constexpr T& operator()(size_t r, size_t c) noexcept;              // + const overload, both assert bounds
template <size_t R, size_t C> constexpr T& at() noexcept;          // + const overload
constexpr Vec<T,Cols> row(size_t) const noexcept requires (Cols > 1);
constexpr Vec<T,Rows> col(size_t) const noexcept requires (Rows > 1);
constexpr void SetRow/SetCol(size_t, const Vec<…>&) noexcept;

constexpr Mat& operator+=/-=(const Mat&) noexcept;
constexpr Mat& operator*=//=(T) noexcept;                          // /= asserts nonzero
constexpr Mat  operator+/-(const Mat&) const noexcept;
constexpr Mat  operator*//(T) const noexcept;                      // and a free T * Mat
constexpr Mat  operator-() const noexcept;                         // unary negate
constexpr bool operator==(const Mat&) const noexcept;              // exact; != is synthesised

template <size_t K> constexpr Mat<T,Rows,K> operator*(const Mat<T,Cols,K>&) const noexcept;
constexpr Vec<T,Rows> operator*(const Vec<T,Cols>&) const noexcept requires (Rows > 1 && Cols > 1);
constexpr Mat& operator*=(const Mat&) noexcept requires (Rows == Cols);

constexpr Mat<T,Cols,Rows> Transposed() const noexcept;
constexpr Mat& Transpose()   noexcept requires (Rows == Cols);     // in place, square only
constexpr T    Trace()       noexcept requires (Rows == Cols);
constexpr T    Determinant() const noexcept requires (Rows == Cols);
constexpr bool Invert()      noexcept requires (Rows == Cols && floating point);
constexpr Mat  Inverted()    const noexcept requires (Rows == Cols && floating point);
```

The inner dimension of `operator*` is enforced by the parameter type, not an
assert: `Mat<T,2,3> * Mat<T,3,4>` gives `Mat<T,2,4>`, and a mismatch is a
deduction failure.

`Determinant()` uses a closed form up to 3×3 and the **Bareiss** algorithm above
that — fraction-free, so it stays exact for integral `T`. It pivots only to step
over a zero pivot, not for magnitude, so a large ill-conditioned float matrix
would be better served by LU with partial pivoting; at 4×4 it does not matter.
`Invert()` is Gauss-Jordan **with** partial pivoting, floating-point only; it
returns `false` and leaves the matrix **unchanged** when singular. `Inverted()`
returns `Identity()` in that case and asserts, so use `Invert()` if you have to
tell the two apart.

### Aliases

```cpp
template <Scalar T> using Mat2 = Mat<T,2,2>;   // Mat3, Mat4 likewise
using Mat2f/Mat2d/Mat3f/Mat3d/Mat4f/Mat4d;
```

All six concrete aliases are re-exported into `ROSE` by `math.h`.

### Two things to know

1. **Everything that touches only `data` is usable in a constant expression.**
   The members that take or return a `Vec` — `row()`, `col()`, `SetRow()`,
   `SetCol()`, `Diagonal()`, `operator*(Vec)` — are *not*, for `Vec<T, N ≤ 4>`:
   those specialisations put their storage in an anonymous union, and touching
   `Vec::data` while `x`/`y`/`z`/`w` is the active member is not a constant
   expression. They are correct at runtime. This is a `vector.h` limitation, not
   a matrix one.
2. `matrix.h` ends with a block of `static_assert`s covering the indexing, the
   algebra, both determinant paths and the inverse. They compile away to nothing;
   define `ROSE_MATH_NO_SELFTEST` to drop them. Every value in there is exact in
   binary floating point on purpose — don't add a case that isn't.

Still missing: translation/rotation/scale/`LookAt`/projection builders, a
quaternion→matrix conversion, and a `std::formatter`. `Transform` deliberately
stores position + quaternion + scale rather than a matrix, so nothing in the
engine consumes `Mat` yet.

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

constexpr double Sin(double)  noexcept;   constexpr float Sin(float) noexcept;
constexpr double Cos(double)  noexcept;   constexpr float Cos(float) noexcept;
constexpr double Tan(double)  noexcept;   constexpr float Tan(float) noexcept;

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

`Sin`/`Cos`/`Tan` follow the exact same `__builtin_is_constant_evaluated()`
pattern: at runtime they lower to `__builtin_sin`/`cos`/`tan` (+`f` variants),
which match libm bit-for-bit; at compile time they go through a shared
`detail::SinCosConst`, which does Cody–Waite range reduction into
\f$[-\pi/4, \pi/4]\f$ and a Horner-form Taylor series, returning sin and cos
together (the quadrant dispatch produces both; `Tan` is `sin/cos`). Arguments are
in **radians**. Note the reduction uses full-precision π/2 literals baked into
`SinCosConst`, **not** `math::PI` — `math::PI` is only float-precise (see #8) and
would poison it. Same accuracy caveat as `Sqrt`: the two paths need not agree in
the last bit (constexpr `Sin(2.0)` is ~1 ulp off libm), and the compile-time path
loses low bits for very large arguments (roughly beyond \f$2^{20}\f$). Verified:
`static_assert(Sin(0.0) == 0.0)` and `Cos(0.0) == 1.0` hold; constexpr errors stay
≤ ~1 ulp, including `Sin(100.0)` after reduction.

⚠️ `math::Min`/`Max` and `ROSE::Min`/`Max` (from `utility.h`) both exist, with
different signatures — `ROSE::` takes by value and is unconstrained, `math::`
takes by const reference and requires an arithmetic type. With both namespaces in
scope an unqualified call is ambiguous.

Missing: `Abs`, `Floor`/`Ceil`/`Round`, `Lerp`, `Pow`, inverse trig
(`Asin`/`Acos`/`Atan`/`Atan2`), `ToRadians`/`ToDegrees`.

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
| matrix add/sub/scale/multiply, `Mat*Vec`, transpose, trace | ✅ |
| matrix identity / determinant / inverse | ✅ square only; inverse is floating-point only |
| matrix transform builders (translate/rotate/project) | ❌ |
| complex arithmetic + formatting | ✅ |
| quaternion product, normalize, axis-angle, from-euler | ✅ |
| quaternion inverse / slerp / rotate-a-vector / to-euler | ❌ |
| `constexpr` sqrt | ✅ `math::Sqrt` |
| `constexpr` sin / cos / tan | ✅ `math::Sin`/`Cos`/`Tan` (radians; runtime builtin, constexpr fallback) |
| clamp / min / max | ✅ (mind the `ROSE::` vs `math::` overload clash) |
| abs, floor, lerp, deg↔rad, inverse trig | ❌ |
| π to double precision | ❌ `math::PI` is float-precision |

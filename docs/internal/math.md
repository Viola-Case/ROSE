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
- ❌ The `Vec2`/`Vec3`/`Vec4`/`Vec7` and `Mat2`/`Mat3`/`Mat4` template aliases, the
  `Mat` and `Vec` templates themselves, the `Vec7f`/`Vec7d` concrete aliases,
  `Sign`, `EulerOrder`, `Sqrt`, `Clamp`, `PI`, `LeviCivita`, `FanoSign`,
  `Orthographic` and `Perspective` all need a `math::` qualifier.

Where the engine actually uses these: `Transform` (`Vec3d position/scale`,
`Quatd rotation`), `mesh.h` (`Vec3d position/normal`), `motion.h` (four `Vec3d`
derivatives), `input.h` (`Vec2f GetStickAxes`), `paramview.h` (`GetVec3d`,
`GetVec4d`), `camera.h`/`gfx.h`/`application.h` (`Mat4` view-projection),
`renderable.h` (`Vec4f` tints and colours).
Note the engine standardises on **double** for spatial data, and float for input
axes and anything heading for a graphics API.

Checked against the headers on **2026-09-02** (`master` @ `de3eafa`). `vector.h`
was rewritten since the previous pass and its section here is new; `matrix.h` and
`quaternion.h` both gained members.

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

There is also a 7D counterpart, added for `Vec7::Cross`:

```cpp
constexpr Sign FanoSign(size_t i, size_t j, size_t k) noexcept;
```

Same totally-antisymmetric role as `LeviCivita`, but supported on the seven lines
of the Fano plane rather than on permutations of `0..N-1`. It locates each index
within its line and hands the positions to `LeviCivita`, so the parity logic lives
in one place.

`LeviCivita` `static_assert`s that every argument is convertible to `size_t`,
returns `Zero` unless the arguments are exactly a permutation of `0..N-1`, and
otherwise counts inversions. The error message still says `cse::math::` from a
previous name for the library.

`Sign`'s `float` constructor is implicit, so `Sign s = 3.5f;` compiles.

---

## `vector.h` — `Vec<T, N>`

```cpp
template <Scalar T, size_t N> requires (N > 1) struct Vec : detail::VecStorage<T, N>;
```

**Rewritten.** There is now one primary template and no per-`N` specialisations of
`Vec` itself. Only the storage varies with `N`, in `detail::VecStorage<T, N>`;
everything else — the arithmetic, indexing, conversion, `Dot`, `Cross`, `Norm`,
`Unit` — is written once, in `Vec`.

### Storage

| `N` | Layout | Constructor |
|---|---|---|
| generic | `FixedArray<T, NextPow2(N)> data` | up to `N` args, rest zeroed |
| 2 | union of `FixedArray<T,2> data` and `{ T x, y; }` | `(T _x, T _y = T{})` |
| 3 | union of `FixedArray<T,4> data` and `{ T x, y, z, w; }` | `(T _x, T _y = T{}, T _z = T{})` |
| 4 | union of `FixedArray<T,4> data` and `{ T x, y, z, w; }` | `(T _x, T _y = T{}, T _z = T{}, T _w = T{})` |

Three things fall out of that table:

1. **`Vec3` stores four elements.** The fourth is named `w` and is SIMD padding, so
   `sizeof(Vec3f)` is 16, not 12. The generic case pads to `NextPow2(N)` for the
   same reason. Do not assume tight packing when handing an array of `Vec3` to a
   graphics API.
2. **The component constructors initialise `data`, not the named members**, which
   makes the array the active union member. That is deliberate: the whole-vector
   operations all go through `data`, so they fold in a constant expression, at the
   cost of `v.x` not being readable in one. Reading `x`/`y`/`z`/`w` still works at
   runtime, as an extension every compiler implements.
3. The default constructor zeroes. `explicit VecStorage(NoInit)` skips that, for
   the case where you are about to overwrite everything anyway.

### Members

```cpp
static constexpr size_t size = N;

constexpr Vec() noexcept = default;                       // zeroed

constexpr Vec& operator+=/-=(const Vec&) noexcept;
constexpr Vec& operator*=(T) noexcept;                    // scalar only
constexpr Vec  operator+/-(const Vec&) const noexcept;
constexpr Vec  operator*(T) const noexcept;

constexpr T&       operator[](size_t);                    // + const overload; asserts idx < N
template <size_t I> requires (I < N) constexpr T& at() noexcept;   // + const overload
template <Scalar U> constexpr operator Vec<U, N>() const; // element-wise static_cast

constexpr T   Dot(const Vec&) const noexcept;
constexpr Vec Cross(const Vec&) const noexcept requires (N == 3 || N == 7);
constexpr T   Norm();                                     // Euclidean length; non-const
constexpr Vec Unit();                                     // normalised copy; non-const

static constexpr T   DotProduct(const Vec&, const Vec&);
static constexpr Vec CrossProduct(const Vec&, const Vec&);
```

Note the casing: `Dot`/`Cross` are `PascalCase` now, not the old `dot`/`cross`.
`Norm()` and `Unit()` are **not** `const`, so neither works on a `const Vec` — as
with `operator Vec<U,N>` before it, that is an oversight rather than a design.

### The cross product

Defined for `N == 3` **and `N == 7`** — the only two dimensions admitting a
bilinear cross product. Both go through one contraction, `(a × b)_i = φ_ijk a_j b_k`,
with φ the Levi-Civita symbol in 3D and the octonion structure constant (supported
on the seven lines of the Fano plane, via `FanoSign` in `mathenum.h`) in 7D.

The symbol is mostly zeros — two surviving terms per row in 3D, six in 7D — so
`detail::MakeCrossTable<N>()` sieves the non-zero terms at compile time into
`detail::crossTable<N>` and the runtime loop visits only those. It is `consteval`,
and it `throw`s if the symbol it was handed does not have the support the table was
sized for; that turns a wrong symbol into a compile error and never runs at runtime.

### Aliases

```cpp
template <Scalar T> using Vec2 = Vec<T,2>;   // Vec3, Vec4, Vec7 likewise
using Vec2f/Vec2d/Vec3f/Vec3d/Vec4f/Vec4d/Vec7f/Vec7d;
```

Only the six `Vec2`/`Vec3`/`Vec4` concrete aliases are re-exported into `ROSE`;
`Vec7f`/`Vec7d` need a `math::` qualifier.

### Formatting

`std::formatter<Vec<T,N>>` works and has a real spec grammar,
`{:[flags][|scalar-spec]}` — see `known-issues.md` #6 for the flag table and
verified output. `{}` gives `(1, -2, 0)`; `{:c}` gives cartesian `(1x -2y)`.

### Missing

No `operator-` (unary negate), no `operator/`, no `operator==`, no component-wise
`operator*`, no `Lerp`, no swizzles. `operator*` with the scalar **on the left**
exists but does not compile — it is declared to return `T` and returns a `Vec`
(`known-issues.md` #11). The `Vec<U,N>` conversion operator, `Norm()` and `Unit()`
are all non-`const`.

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
static constexpr Mat Translation(const Vec<T,3>&) noexcept;   // 4x4 affine
static constexpr Mat Scaling(const Vec<T,3>&)     noexcept;   // 4x4 affine

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

1. **Everything is usable in a constant expression, including the members that
   take or return a `Vec`** — `row()`, `col()`, `SetRow()`, `SetCol()`,
   `Diagonal()`, `operator*(Vec)`. This used to be false: the small `Vec`
   specialisations kept `x`/`y`/`z`/`w` as the active union member, and reading
   `Vec::data` through the inactive one is not a constant expression. The
   `vector.h` rewrite flipped which member the constructors activate — `data` is
   now the active one — so these fold. The cost landed on the other side: `v.x` is
   what is no longer readable in a constant expression. See `math.md`'s `vector.h`
   section and `known-issues.md` #3.
2. `matrix.h` ends with a block of `static_assert`s covering the indexing, the
   algebra, both determinant paths and the inverse. They compile away to nothing;
   define `ROSE_MATH_NO_SELFTEST` to drop them. Every value in there is exact in
   binary floating point on purpose — don't add a case that isn't.

### Transform and projection builders

`Mat::Translation(Vec3)` and `Mat::Scaling(Vec3)` build 4×4 affine matrices, and
two free functions in the `projections` region build the clip transforms:

```cpp
template <Scalar T> constexpr Mat<T,4,4> Orthographic(T left, T right, T bottom, T top, T near, T far) noexcept;
template <Scalar T> constexpr Mat<T,4,4> Perspective(T fovY, T aspect, T near, T far) noexcept;
```

Both map to the **OpenGL clip volume** — x, y and z all in [-1, 1] — and both use
the column-vector convention of `Mat::Translation`. `z` is negated because the eye
looks down -z, so a point in front of the camera has negative view-space z and
comes out with positive depth. A backend whose clip space differs (D3D's z in
[0, 1]) wants its own builder here, not a fixup at the call site. `Orthographic`
asserts against a degenerate volume.

The rotation half comes from the quaternion side: `Quat::ToMat4()`. There is still
no `LookAt` and no `std::formatter<Mat>`.

`Mat` is no longer unused by the engine — `camera.h`, `gfx.h` and `application.h`
all traffic in `Mat4`, and `Camera::GetViewProjection` composes `Perspective` or
`Orthographic` with the view matrix each frame. `Transform` still stores position +
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

static constexpr Quat  AxisAngle(T angle, Vec3<T> axis);      // angle in RADIANS, axis assumed unit
static constexpr Quat  FromEuler(Vec3<T>, EulerOrder = XYZ);
static constexpr Quat<T> Identity();

constexpr T       Norm() const noexcept;         // magnitude via math::Sqrt, NOT squared norm
constexpr Quat&   Normalize() noexcept;          // in place; degenerate → Identity, never NaN
constexpr Quat    Normalized() const noexcept;
constexpr Mat4<T> ToMat4() const noexcept;       // assumes unit length
constexpr Quat&   operator*=(const Quat&) noexcept;
constexpr Quat    operator*(const Quat&) const noexcept;   // Hamilton product

Vec3<T> ToEuler(EulerOrder = ZYX) const noexcept;
static constexpr T kGimbalEpsilon;               // 3.5e-4 for float, 1.5e-8 for double
```

Notes:

- `Norm()` is the **length**, not the squared length. `Normalize()` guards
  `!(n > 0)` (so NaN also falls through to identity).
- `AxisAngle` and `FromEuler` are genuinely constant-evaluable: they go through
  `math::Sin`/`Cos`, not `std::sin`/`std::cos`. The angle is promoted to `double`
  (or stays `float` for `Quat<float>`) first, since `T` may be integral and the
  `Sin` overloads would otherwise be ambiguous. Verified correct at runtime:
  `AxisAngle(π/2, 0,0,1)` → `(0.7071, 0, 0, 0.7071)`, norm 1.
- The folded and runtime results are **not** bit-identical — the constant-evaluated
  path carries up to ~3.8e-13 absolute error (see `mathfunctions.h`), against a
  runtime path that matches libm exactly. Fine for rotations, but don't
  `static_assert` a folded component against a decimal literal, and don't cache a
  folded quaternion expecting it to equal the same call made at runtime.
- `FromEuler` default order is `XYZ`; `ToEuler` default order is `ZYX`. **Pass the
  same order to both** — the defaults differ, so `q.ToEuler()` fed back into
  `FromEuler()` unqualified does not round-trip.
- `ToEuler` is **implemented** (it was a stub returning `{}` until `6706679`). It
  recovers the angles from `Normalized().ToMat4()` rather than from the components:
  every order here is a Tait-Bryan triple, so one extraction serves all six, with
  the axis triple and its parity selecting which matrix entries to read.
  Component `n` of the result is always the angle about axis `n` — `[0]` is X
  whichever order asked for it — matching how `FromEuler` reads its argument.
- Two warnings the header spells out and that are worth repeating. `ToEuler` is
  **not a bijection and cannot be**: Euler angles are three-to-one onto rotations,
  so the angles that come back need not be the ones that went in. Round-trip the
  *rotation*, never the *value*. And at the poles only the sum (or difference) of
  the outer and inner angles survives; the inner one is pinned to zero and the
  whole of it is reported on the outer.
- `kGimbalEpsilon` is the cosine of how near the middle rotation may come to a
  pole before the locked branch takes over. It sits at `sqrt(machineEpsilon)`,
  where the two branches' errors meet, which is also the worst-case error of the
  whole function in radians. **Do not tune it by round-tripping random rotations**
  — uniform angles land near a pole with probability proportional to the threshold
  itself, so a random sweep never samples the case it governs. Sample `pi/2 - delta`
  directly.
- **Missing:** conjugate, inverse, dot, Slerp/Nlerp, vector rotation
  (`q * v * q⁻¹`), `operator+`, `operator==`, scalar multiply. Rotating a point
  still has to be written by hand at the call site — `Transform::RotateAroundPoint`
  in `transform.h` is the worked example, expanding the unit-quaternion sandwich so
  it needs neither a conjugate nor a matrix.

---

## `mathfunctions.h`

```cpp
template <StdScalar T> constexpr T Clamp(T value, T min, T max) noexcept;

constexpr double Sqrt(double) noexcept;
constexpr float  Sqrt(float)  noexcept;

constexpr double Sin(double)  noexcept;   constexpr float Sin(float) noexcept;
constexpr double Cos(double)  noexcept;   constexpr float Cos(float) noexcept;
constexpr double Tan(double)  noexcept;   constexpr float Tan(float) noexcept;

inline double Asin(double)  noexcept;         inline float Asin(float) noexcept;
inline double Atan2(double y, double x) noexcept;  inline float Atan2(float, float) noexcept;

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

`Asin` and `Atan2` were added for `Quat::ToEuler` and are the one place the pattern
breaks: they are `inline`, **not `constexpr`**, and lower straight to
`__builtin_asin`/`__builtin_atan2`. A constant-evaluated path would need its own
range reduction and series the way `detail::SinCosConst` does, and nothing has
wanted an arcsine inside a constant expression yet — the header asks that it be
added here rather than at a call site if that changes. `Atan2(y, x)` takes the
conventional argument order and is quadrant-correct across all four, which is why
`ToEuler` uses it instead of `Asin` to recover angles from matrix entry pairs.
`Asin` gives NaN rather than a clamped angle outside [-1, 1], so clamp first.

Missing: `Abs`, `Floor`/`Ceil`/`Round`, `Lerp`, `Pow`, `Acos`/`Atan`,
`ToRadians`/`ToDegrees`.

---

## `constants.h`

```cpp
constexpr double    PI    = 3.14159265358979323846;
constexpr double    E     = 2.71828182845904523536;
constexpr double    PHI   = 1.61803398874989484820;
constexpr double    TAU   = 2. * PI;
constexpr double    SQRT2 = 1.41421356237309504880;
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

`PI`, `E`, `PHI` and `TAU` used to carry only float precision — the literals had an
`f` suffix, so each value was rounded to float and then widened, for a measured
~8.7e-8 relative error. **Fixed**; all five now carry full double precision. See
`known-issues.md` #8 for why a bare `f` suffix is worth being suspicious of
anywhere near the `Vec3d`/`Quatd` path.

`detail::SinCosConst` still bakes its own full-precision π/2 literals rather than
reaching for `math::PI`, which is the right call independent of the fix: its
range reduction needs π/2 split into a high and low part, which no single constant
provides.

Only `constants.h` and `bigint.h` provide the FNV magic numbers; `utility.cpp`
and `hashmap.cpp` both reach into `math::` for them.

---

## Quick "does math have…?" table

| Want | Answer |
|---|---|
| vector add/sub/dot/cross | ✅ `Dot`/`Cross`; cross is `Vec3` **and `Vec7`** |
| vector length / normalize | ✅ `Norm()` / `Unit()` — but both are non-`const` |
| vector `operator/`, unary `-`, `==` | ❌ |
| scalar `*` on the left (`2.0 * v`) | ⚠️ declared, but returns `T` and does not compile — `known-issues.md` #11 |
| `Vec4` with four arguments | ✅ fixed |
| `vec[i]` | ✅ fixed, `_DEBUG` included |
| `std::format` a `Vec` | ✅ with a flag grammar — `known-issues.md` #6 |
| `Vec3` packed tight (12 bytes) | ❌ it pads to 16 for SIMD |
| matrix add/sub/scale/multiply, `Mat*Vec`, transpose, trace | ✅ |
| matrix identity / determinant / inverse | ✅ square only; inverse is floating-point only |
| matrix translate / scale builders | ✅ `Mat::Translation`, `Mat::Scaling` |
| projection builders | ✅ `Orthographic`, `Perspective` — OpenGL clip volume |
| `LookAt` | ❌ |
| quaternion → matrix | ✅ `Quat::ToMat4()` |
| `std::format` a `Mat` | ❌ |
| complex arithmetic + formatting | ✅ |
| quaternion product, normalize, axis-angle, from-euler, to-euler | ✅ |
| quaternion inverse / slerp / rotate-a-vector | ❌ (see `Transform::RotateAroundPoint`) |
| `constexpr` sqrt | ✅ `math::Sqrt` |
| `constexpr` sin / cos / tan | ✅ `math::Sin`/`Cos`/`Tan` (radians; runtime builtin, constexpr fallback) |
| `Asin` / `Atan2` | ✅ runtime only — **not** `constexpr` |
| clamp / min / max | ✅ (mind the `ROSE::` vs `math::` overload clash) |
| abs, floor, lerp, deg↔rad, `Acos`/`Atan` | ❌ |
| π to double precision | ✅ fixed |

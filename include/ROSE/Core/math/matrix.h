/**

  @file      matrix.h
  @brief
  @details   ~
  @author    Viola Case
  @date      11.07.2026
  @copyright © Viola Case, 2026. All right reserved.

**/
#pragma once

#include <ROSE/Core/macros.h>
#include <ROSE/Core/math/vector.h>

namespace ROSE::math {

  namespace detail {
    /*! Local |x|. math::Abs does not exist yet and std::abs is not constexpr before C++23. */
    template <Scalar T>
    constexpr T MatAbs(const T _value) noexcept {
      return _value < T { 0 } ? -_value : _value;
    }
  } // namespace detail

  /*!
   * Dense fixed-size matrix stored in **row-major** order: element (r, c) lives at `data[r * Cols + c]`.
   *
   * Square-only operations - Identity(), Diagonal(), Trace(), Determinant(), Invert(), Inverted(), in-place
   * Transpose() and `operator*=(Mat)` - carry `requires(Rows == Cols)` and simply do not exist on a non-square
   * instantiation. That is why there is no separate square specialisation to keep in sync.
   *
   * @note Everything that touches only `data` is usable in a constant expression. The members that take or return a
   *       `Vec` - row(), col(), SetRow(), SetCol(), Diagonal() and `operator*(Vec)` - are not, for `Vec<T, N <= 4>`:
   *       those specialisations put their storage in an anonymous union, and reading or writing `Vec::data` when the
   *       named `x`/`y`/`z`/`w` member is the active one is not a constant expression. They are correct at runtime.
   *
   * @tparam T    element type; any Scalar
   * @tparam Rows row count, nonzero
   * @tparam Cols column count, nonzero
   */
  template <Scalar T, size_t Rows, size_t Cols>
    requires(Rows > 0 && Cols > 0)
  struct Mat {
    using ValueType = T;

    static constexpr size_t rowCount = Rows;
    static constexpr size_t colCount = Cols;
    static constexpr size_t size = Rows * Cols;

    FixedArray<T, size> data;

    /*! The zero matrix. */
    constexpr Mat() noexcept {
      for (size_t i { 0 }; i < size; ++i)
        data[i] = T { 0 };
    }

    constexpr Mat(const Mat &) noexcept = default;
    constexpr Mat &operator=(const Mat &) noexcept = default;

    /*! Row-major element list, one argument per element: `Mat2d m { 1, 2, 3, 4 }` is the matrix [[1, 2], [3, 4]]. */
    template <typename... Args>
      requires(sizeof...(Args) == size && size > 1 && (std::is_convertible_v<Args, T> && ...))
    constexpr Mat(Args... _args) noexcept {
      const T values[size] { static_cast<T>(_args)... };
      for (size_t i { 0 }; i < size; ++i)
        data[i] = values[i];
    }

#pragma region factories

    /*! All elements zero. Identical to a default-constructed Mat; spelled out for call sites that want to say so. */
    static constexpr Mat Zero() noexcept { return Mat {}; }

    /*! Every element set to @p _value. */
    static constexpr Mat Filled(const T _value) noexcept {
      Mat result;
      for (size_t i { 0 }; i < size; ++i)
        result.data[i] = _value;
      return result;
    }

    /*! The multiplicative identity: ones down the main diagonal, zeroes elsewhere. */
    static constexpr Mat Identity() noexcept
      requires(Rows == Cols)
    {
      Mat result;
      for (size_t d { 0 }; d < Rows; ++d)
        result.data[d * Cols + d] = T { 1 };
      return result;
    }

    /*! @p _diagonal down the main diagonal, zeroes elsewhere. */
    static constexpr Mat Diagonal(const Vec<T, Rows> &_diagonal) noexcept
      requires(Rows == Cols && Rows > 1)
    {
      Mat result;
      for (size_t d { 0 }; d < Rows; ++d)
        result.data[d * Cols + d] = _diagonal.data[d];
      return result;
    }

#pragma endregion

#pragma region element access

    constexpr T &operator()(const size_t _row, const size_t _col) noexcept {
      ROSE_ASSERT(_row < Rows);
      ROSE_ASSERT(_col < Cols);
      return data[_row * Cols + _col];
    }

    constexpr const T &operator()(const size_t _row, const size_t _col) const noexcept {
      ROSE_ASSERT(_row < Rows);
      ROSE_ASSERT(_col < Cols);
      return data[_row * Cols + _col];
    }

    template <size_t R, size_t C>
      requires(R < Rows && C < Cols)
    constexpr T &at() noexcept {
      return data[R * Cols + C];
    }

    template <size_t R, size_t C>
      requires(R < Rows && C < Cols)
    constexpr const T &at() const noexcept {
      return data[R * Cols + C];
    }

    /*! Row @p _idx as a vector of Cols components. */
    constexpr Vec<T, Cols> row(const size_t _idx) const noexcept
      requires(Cols > 1)
    {
      ROSE_ASSERT(_idx < Rows);
      Vec<T, Cols> result;
      for (size_t c { 0 }; c < Cols; ++c)
        result.data[c] = data[_idx * Cols + c];
      return result;
    }

    /*! Column @p _idx as a vector of Rows components. */
    constexpr Vec<T, Rows> col(const size_t _idx) const noexcept
      requires(Rows > 1)
    {
      ROSE_ASSERT(_idx < Cols);
      Vec<T, Rows> result;
      for (size_t r { 0 }; r < Rows; ++r)
        result.data[r] = data[r * Cols + _idx];
      return result;
    }

    constexpr void SetRow(const size_t _idx, const Vec<T, Cols> &_row) noexcept
      requires(Cols > 1)
    {
      ROSE_ASSERT(_idx < Rows);
      for (size_t c { 0 }; c < Cols; ++c)
        data[_idx * Cols + c] = _row.data[c];
    }

    constexpr void SetCol(const size_t _idx, const Vec<T, Rows> &_col) noexcept
      requires(Rows > 1)
    {
      ROSE_ASSERT(_idx < Cols);
      for (size_t r { 0 }; r < Rows; ++r)
        data[r * Cols + _idx] = _col.data[r];
    }

#pragma endregion

#pragma region arithmetic

    constexpr Mat &operator+=(const Mat &_rhs) noexcept {
      for (size_t i { 0 }; i < size; ++i)
        data[i] += _rhs.data[i];
      return *this;
    }

    constexpr Mat &operator-=(const Mat &_rhs) noexcept {
      for (size_t i { 0 }; i < size; ++i)
        data[i] -= _rhs.data[i];
      return *this;
    }

    constexpr Mat &operator*=(const T _rhs) noexcept {
      for (size_t i { 0 }; i < size; ++i)
        data[i] *= _rhs;
      return *this;
    }

    constexpr Mat &operator/=(const T _rhs) noexcept {
      ROSE_ASSERT_MSG(_rhs != T { 0 }, "Matrix scalar division by zero");
      for (size_t i { 0 }; i < size; ++i)
        data[i] /= _rhs;
      return *this;
    }

    constexpr Mat operator+(const Mat &_rhs) const noexcept {
      Mat result(*this);
      return (result += _rhs);
    }

    constexpr Mat operator-(const Mat &_rhs) const noexcept {
      Mat result(*this);
      return (result -= _rhs);
    }

    constexpr Mat operator*(const T _rhs) const noexcept {
      Mat result(*this);
      return (result *= _rhs);
    }

    constexpr Mat operator/(const T _rhs) const noexcept {
      Mat result(*this);
      return (result /= _rhs);
    }

    constexpr Mat operator-() const noexcept {
      Mat result;
      for (size_t i { 0 }; i < size; ++i)
        result.data[i] = -data[i];
      return result;
    }

    /*!
     * Matrix product. `Mat<T, Rows, Cols> * Mat<T, Cols, RhsCols>` yields `Mat<T, Rows, RhsCols>`; the inner
     * dimensions have to agree, which the parameter type enforces rather than an assert.
     */
    template <size_t RhsCols>
    constexpr Mat<T, Rows, RhsCols> operator*(const Mat<T, Cols, RhsCols> &_rhs) const noexcept {
      Mat<T, Rows, RhsCols> result;
      for (size_t r { 0 }; r < Rows; ++r) {
        for (size_t c { 0 }; c < RhsCols; ++c) {
          T sum { 0 };
          for (size_t k { 0 }; k < Cols; ++k)
            sum += data[r * Cols + k] * _rhs.data[k * RhsCols + c];
          result.data[r * RhsCols + c] = sum;
        }
      }
      return result;
    }

    /*! Treats @p _rhs as a Cols x 1 column vector and returns the Rows x 1 result. */
    constexpr Vec<T, Rows> operator*(const Vec<T, Cols> &_rhs) const noexcept
      requires(Rows > 1 && Cols > 1)
    {
      Vec<T, Rows> result;
      for (size_t r { 0 }; r < Rows; ++r) {
        T sum { 0 };
        for (size_t c { 0 }; c < Cols; ++c)
          sum += data[r * Cols + c] * _rhs.data[c];
        result.data[r] = sum;
      }
      return result;
    }

    /*! Square only - for a non-square product the result has a different type, so use the binary form. */
    constexpr Mat &operator*=(const Mat &_rhs) noexcept
      requires(Rows == Cols)
    {
      return (*this = *this * _rhs);
    }

    /*! Exact element-wise equality. `operator!=` is synthesised from this. */
    constexpr bool operator==(const Mat &_rhs) const noexcept {
      for (size_t i { 0 }; i < size; ++i)
        if (data[i] != _rhs.data[i]) return false;
      return true;
    }

#pragma endregion

#pragma region linear algebra

    /*! The transpose, as a new Cols x Rows matrix. */
    constexpr Mat<T, Cols, Rows> Transposed() const noexcept {
      Mat<T, Cols, Rows> result;
      for (size_t r { 0 }; r < Rows; ++r) {
        for (size_t c { 0 }; c < Cols; ++c)
          result.data[c * Rows + r] = data[r * Cols + c];
      }
      return result;
    }

    /*! Transpose in place. Square only - a non-square transpose changes the type, so use Transposed(). */
    constexpr Mat &Transpose() noexcept
      requires(Rows == Cols)
    {
      for (size_t r { 0 }; r < Rows; ++r) {
        for (size_t c { r + 1 }; c < Cols; ++c) {
          const T swap = data[r * Cols + c];
          data[r * Cols + c] = data[c * Cols + r];
          data[c * Cols + r] = swap;
        }
      }
      return *this;
    }

    /*! Sum of the main diagonal. */
    constexpr T Trace() const noexcept
      requires(Rows == Cols)
    {
      T sum { 0 };
      for (size_t d { 0 }; d < Rows; ++d)
        sum += data[d * Cols + d];
      return sum;
    }

    /*!
     * The determinant. Closed form up to 3x3; 4x4 and larger go through the Bareiss algorithm, which is
     * fraction-free - every division it performs divides exactly, so the result stays exact for integral T instead of
     * accumulating the rounding that a plain LU factorisation would.
     *
     * @note Bareiss pivots only to step over a zero pivot, not for magnitude. For a large ill-conditioned
     *       floating-point matrix an LU factorisation with partial pivoting would be more accurate; at the sizes this
     *       class gets used at (4x4 transforms) the difference does not show up.
     */
    constexpr T Determinant() const noexcept
      requires(Rows == Cols)
    {
      if constexpr (Rows == 1) {
        return data[0];
      } else if constexpr (Rows == 2) {
        return data[0] * data[3] - data[1] * data[2];
      } else if constexpr (Rows == 3) {
        return data[0] * (data[4] * data[8] - data[5] * data[7]) - data[1] * (data[3] * data[8] - data[5] * data[6]) +
               data[2] * (data[3] * data[7] - data[4] * data[6]);
      } else {
        Mat work(*this);
        T previous { 1 };
        T sign { 1 };
        for (size_t k { 0 }; k + 1 < Rows; ++k) {
          if (work.data[k * Cols + k] == T { 0 }) {
            /* Nothing to eliminate with. Pull up any row below that has a nonzero here; a row swap flips the sign. */
            size_t pivot { k + 1 };
            while (pivot < Rows && work.data[pivot * Cols + k] == T { 0 })
              ++pivot;
            if (pivot == Rows) return T { 0 };
            for (size_t c { k }; c < Cols; ++c) {
              const T swap = work.data[k * Cols + c];
              work.data[k * Cols + c] = work.data[pivot * Cols + c];
              work.data[pivot * Cols + c] = swap;
            }
            sign = -sign;
          }
          for (size_t r { k + 1 }; r < Rows; ++r) {
            for (size_t c { k + 1 }; c < Cols; ++c) {
              work.data[r * Cols + c] = (work.data[r * Cols + c] * work.data[k * Cols + k] -
                                         work.data[r * Cols + k] * work.data[k * Cols + c]) /
                                        previous;
            }
          }
          previous = work.data[k * Cols + k];
        }
        return sign * work.data[(Rows - 1) * Cols + (Rows - 1)];
      }
    }

    /*!
     * Invert in place by Gauss-Jordan elimination with partial pivoting.
     *
     * @return true on success. A singular matrix is left **unchanged** and false is returned, so a caller that has to
     *         tell the two apart should use this rather than Inverted().
     */
    constexpr bool Invert() noexcept
      requires(Rows == Cols && std::is_floating_point_v<T>)
    {
      Mat work(*this);
      Mat result = Identity();

      for (size_t k { 0 }; k < Rows; ++k) {
        /* Partial pivoting: eliminating with the largest remaining magnitude in the column keeps the division below
         * from amplifying rounding error, and picks up an exact zero column as the singular case. */
        size_t pivot { k };
        T best = detail::MatAbs(work.data[k * Cols + k]);
        for (size_t r { k + 1 }; r < Rows; ++r) {
          const T candidate = detail::MatAbs(work.data[r * Cols + k]);
          if (candidate > best) {
            best = candidate;
            pivot = r;
          }
        }
        if (best == T { 0 }) return false;

        if (pivot != k) {
          for (size_t c { 0 }; c < Cols; ++c) {
            T swap = work.data[k * Cols + c];
            work.data[k * Cols + c] = work.data[pivot * Cols + c];
            work.data[pivot * Cols + c] = swap;
            swap = result.data[k * Cols + c];
            result.data[k * Cols + c] = result.data[pivot * Cols + c];
            result.data[pivot * Cols + c] = swap;
          }
        }

        const T inversePivot = T { 1 } / work.data[k * Cols + k];
        for (size_t c { 0 }; c < Cols; ++c) {
          work.data[k * Cols + c] *= inversePivot;
          result.data[k * Cols + c] *= inversePivot;
        }

        for (size_t r { 0 }; r < Rows; ++r) {
          if (r == k) continue;
          const T factor = work.data[r * Cols + k];
          if (factor == T { 0 }) continue;
          for (size_t c { 0 }; c < Cols; ++c) {
            work.data[r * Cols + c] -= factor * work.data[k * Cols + c];
            result.data[r * Cols + c] -= factor * result.data[k * Cols + c];
          }
        }
      }

      *this = result;
      return true;
    }

    /*!
     * The inverse, leaving this matrix alone.
     *
     * @return the inverse, or Identity() if the matrix is singular. The singular case asserts, but assertions are
     *         inert outside a `_DEBUG` build of ROSE_Core - use Invert() when the caller has to handle it.
     */
    constexpr Mat Inverted() const noexcept
      requires(Rows == Cols && std::is_floating_point_v<T>)
    {
      Mat result(*this);
      if (!result.Invert()) {
        ROSE_ASSERT_MSG(false, "Cannot invert a singular matrix");
        return Identity();
      }
      return result;
    }

#pragma endregion
  };

  /*! Scalar on the left, so `2.0 * m` reads the same as `m * 2.0`. */
  template <Scalar T, size_t Rows, size_t Cols>
  constexpr Mat<T, Rows, Cols> operator*(const T _lhs, const Mat<T, Rows, Cols> &_rhs) noexcept {
    return _rhs * _lhs;
  }

  template <Scalar T>
  using Mat2 = Mat<T, 2, 2>;
  template <Scalar T>
  using Mat3 = Mat<T, 3, 3>;
  template <Scalar T>
  using Mat4 = Mat<T, 4, 4>;

  using Mat2f = Mat<float, 2, 2>;
  using Mat2d = Mat<double, 2, 2>;
  using Mat3f = Mat<float, 3, 3>;
  using Mat3d = Mat<double, 3, 3>;
  using Mat4f = Mat<float, 4, 4>;
  using Mat4d = Mat<double, 4, 4>;

#ifndef ROSE_MATH_NO_SELFTEST

  namespace detail {
    /* Compile-time sanity checks on the algebra and the row-major indexing above. They cost nothing at runtime and
     * fail the build the moment an index or a sign in this header is wrong. Define ROSE_MATH_NO_SELFTEST to drop them.
     * Every value here is exact in binary floating point, so `==` is the right comparison; do not add a case whose
     * intermediate results are not. Vec is deliberately absent - see the note on Mat about its union storage. */
    constexpr Mat2d kMatTestA { 1, 2, 3, 4 };
    constexpr Mat<double, 2, 3> kMatTestB { 1, 2, 3, 4, 5, 6 };
    constexpr Mat2d kMatTestC { 1, 2, 0, 4 };
    constexpr Mat3d kMatTestI3 = Mat3d::Identity();
    constexpr Mat4d kMatTestD4 { 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 4, 0, 0, 0, 0, 5 };

    static_assert(kMatTestA(0, 0) == 1 && kMatTestA(0, 1) == 2 && kMatTestA(1, 0) == 3 && kMatTestA(1, 1) == 4,
                  "the element list fills the matrix row by row");
    static_assert(Mat2d {} == Mat2d::Filled(0), "a default-constructed Mat is the zero matrix");
    static_assert(kMatTestI3 * kMatTestI3 == kMatTestI3, "the identity is idempotent under multiplication");
    static_assert(kMatTestA * Mat2d::Identity() == kMatTestA, "the identity is the multiplicative unit");
    static_assert(Mat2d::Identity() * kMatTestA == kMatTestA, "on both sides");
    static_assert((kMatTestA + kMatTestA) == kMatTestA * 2.0, "doubling by addition matches scaling");
    static_assert(2.0 * kMatTestA == kMatTestA * 2.0, "the scalar commutes");
    static_assert((kMatTestA - kMatTestA) == Mat2d::Zero(), "subtracting a matrix from itself zeroes it");
    static_assert(-kMatTestA + kMatTestA == Mat2d::Zero(), "unary negate is the additive inverse");

    static_assert(kMatTestA.Transposed() == Mat2d { 1, 3, 2, 4 }, "transpose swaps the off-diagonal");
    static_assert(kMatTestB.Transposed()(2, 1) == 6.0, "a non-square transpose changes shape");
    static_assert(kMatTestB.Transposed().Transposed() == kMatTestB, "transposing twice is a no-op");
    static_assert((kMatTestB * kMatTestB.Transposed())(0, 0) == 14.0, "1*1 + 2*2 + 3*3");
    static_assert(kMatTestA.Trace() == 5.0, "trace sums the diagonal");

    static_assert(kMatTestA.Determinant() == -2.0, "2x2 determinant");
    static_assert(kMatTestI3.Determinant() == 1.0, "3x3 identity determinant");
    static_assert(Mat3d { 6, 1, 1, 4, -2, 5, 2, 8, 7 }.Determinant() == -306.0, "3x3 determinant");
    static_assert(Mat4d::Identity().Determinant() == 1.0, "the Bareiss path agrees on the identity");
    static_assert(kMatTestD4.Determinant() == 120.0, "4x4 determinant is the product of the diagonal");
    static_assert(Mat4d { 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }.Determinant() == -1.0,
                  "Bareiss handles a zero pivot and counts the row swap");
    static_assert(Mat4d {}.Determinant() == 0.0, "a singular matrix has determinant zero");
    static_assert(Mat<int, 4, 4> { 1, 0, 2, -1, 3, 0, 0, 5, 2, 1, 4, -3, 1, 0, 5, 0 }.Determinant() == 30,
                  "Bareiss stays exact over the integers");

    static_assert(kMatTestC.Inverted() == Mat2d { 1, -0.5, 0, 0.25 }, "2x2 inverse");
    static_assert(kMatTestC.Inverted() * kMatTestC == Mat2d::Identity(), "inverse round trip");
    static_assert(kMatTestD4.Inverted() * kMatTestD4 == Mat4d::Identity(), "4x4 inverse round trip");
    static_assert(kMatTestI3.Inverted() == kMatTestI3, "the identity is its own inverse");
  } // namespace detail

#endif

} // namespace ROSE::math

#ifndef THINKS_POISSONDISKSAMPLING_HPP_INCLUDED
#define THINKS_POISSONDISKSAMPLING_HPP_INCLUDED

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <vector>

namespace thinks {
namespace detail {

typedef std::uint16_t IndexType;

//! Generic, looks for static constant size in type A.
template<typename A>
struct array_size
{
  static const size_t size = A::size;
};

//! Specialized for std::array, which does not provide compile time
//! access to array size (at least not without full support for constexpr).
template<typename T, std::size_t N>
struct array_size<std::array<T, N>>
{
  static const size_t size = N;
};

//! Returns a repeatable stateless pseduo-random number in
//! the range [0, 0xFFFFFFFF].
inline std::uint32_t randHash(const std::uint32_t seed)
{
  using namespace std;
  uint32_t i =
    (seed ^ static_cast<uint32_t>(12345391)) *
      static_cast<uint32_t>(2654435769);
  i ^= (i << 6) ^ (i >> 26);
  i *= static_cast<uint32_t>(2654435769);
  i += (i << 5) ^ (i >> 12);
  return i;
}

//! Returns a repeatable stateless pseduo-random number in the range [0, 1].
template <typename T> inline T normRandHash(const std::uint32_t seed)
{
  using namespace std;
  static_assert(is_floating_point<T>::value, "T must be floating point");
  const T norm_scale = static_cast<T>(1) / numeric_limits<uint32_t>::max();
  return norm_scale * randHash(seed);
}

//! Returns a repeatable stateless pseduo-random number in the
//! range [@a min_value, @a max_value].
template <typename T> inline
T rangeRandHash(const T min_value, const T max_value, const std::uint32_t seed)
{
  using namespace std;
  static_assert(is_floating_point<T>::value, "T must be floating point");
  return min_value + (max_value - min_value) * normRandHash<T>(seed);
}

//! Returns an instance of type V where each element has been assigned
//! using @ref rangeRandHash, with bounds taken from the corresponding index
//! in @a min_values and @a max_values. Note that @a seed is incremented
//! for each assigned element.
template <typename V> inline
V arrayRangeRandHash(const V& min_values, const V& max_values,
                     std::uint32_t* seed)
{
  using namespace std;
  assert(seed != nullptr);
  V v;
  for (size_t i = 0; i < array_size<V>::size; ++i) {
    v[i] = rangeRandHash(min_values[i], max_values[i], *seed);
    *seed += 1;
  }
  return v;
}

//! Returns a linear index into an N-dimensional array with @a dimensions
//! for integer coordinate @a x.
//! Note that this function does not check for integer overflow!
template <typename V> inline
std::size_t linearIndex(const V& dimensions, const V& x)
{
  using namespace std;
  size_t k = x[0];
  size_t d = 1;
  for (size_t i = 1; i < array_size<V>::size; ++i) {
    d *= dimensions[i - 1];
    k += x[i] * d;
  }
  return k;
}

//! Returns @a x squared.
template <typename T> inline T squared(const T x)
{
  return x * x;
}

//! Returns the squared magnitude of the vector @a v.
template <typename V> inline
typename V::value_type squaredMagnitude(const V& v)
{
  using namespace std;
  auto r = squared(v[0]);
  for (size_t i = 1; i < array_size<V>::size; ++i) {
    r += squared(v[i]);
  }
  return r;
}

//! Returns the squared distance between the vectors @a u and @a v.
template <typename V> inline
typename V::value_type squaredDistance(const V& u, const V& v)
{
  using namespace std;
  auto d = squared(u[0] - v[0]);
  for (size_t i = 1; i < array_size<V>::size; ++i) {
    d += squared(u[i] - v[i]);
  }
  return d;
}

//! Returns a pseudo-random coordinate that is guaranteed be at a
//! distance [@a radius, 2 * @a radius] from @a center.
template<typename V> inline
V sampleAnnulus(const typename V::value_type radius, const V& center,
                std::uint32_t* seed)
{
  using namespace std;
  typedef typename V::value_type ValueType;
  assert(seed != nullptr);
  V offset;
  V rand_min;
  V rand_max;
  fill_n(&rand_min[0], array_size<V>::size, static_cast<ValueType>(-2));
  fill_n(&rand_max[0], array_size<V>::size, static_cast<ValueType>(+2));
  for (;;) {
    // Generate a random component in the range [-2, 2] for each dimension.
    offset = arrayRangeRandHash(rand_min, rand_max, seed);
    // The randomized offset is not guaranteed to be within the radial
    // distance that we need to guarantee. If we found an offset with
    // magnitude in the range (1, 2] we are done, otherwise generate a new
    // offset. Continue until finished.
    const auto r2 = squaredMagnitude(offset);
    if (static_cast<ValueType>(1) < r2 && r2 <= static_cast<ValueType>(4)) {
      break;
    }
  }

  // Add the offset scaled by radius to the center coordinate to produce the
  // final sample.
  V sample;
  for (size_t i = 0; i < array_size<V>::size; ++i) {
    sample[i] = center[i] + radius * offset[i];
  }
  return sample;
}

//! Returns grid integer coordinates from a sample.
template <typename V> inline
std::array<IndexType, array_size<V>::size>
gridIndexFromSample(const V& x_sample, const V& x_min,
                    const typename V::value_type dx_grid_inv)
{
  using namespace std;
  static_assert(is_floating_point<typename V::value_type>::value,
                "V::value_type must be floating point");
  std::array<IndexType, array_size<V>::size> grid_index;
  for (size_t i = 0; i < array_size<V>::size; ++i) {
    grid_index[i] =
      static_cast<IndexType>((x_sample[i] - x_min[i]) * dx_grid_inv);
  }
  return grid_index;
}

//! Returns true if @a x is in the range (@a x_min, @a x_max) in
//! all dimensions.
template <typename V> inline
bool exclusiveInside(const V& x_min, const V& x_max, const V& x)
{
  using namespace std;
  for (size_t i = 0; i < array_size<V>::size; ++i) {
    if (x_min[i] > x[i] || x[i] > x_max[i]) {
      return false;
    }
  }
  return true;
}

//! Returns a value in the range [@a min_value, @a max_value].
template <typename T> inline
T clamp(const T min_value, const T max_value, const T value)
{
  return value < min_value ? min_value :
    (value > max_value ? max_value : value);
}

//! Pseudo-erase the value at @a index in the vector @v. The vector @a v is
//! guaranteed to decrese in size by one. Note that the ordering of elements
//! in @a v may change as a result of calling this function.
//! Assumes that @a v is non-null and not empty.
template<class T> inline
void eraseUnordered(const std::size_t index, std::vector<T>* v)
{
  assert(v != nullptr && !v->empty());
  (*v)[index] = v->back();
  v->pop_back();
}

} // namespace detail

//! Returns a list of samples that are guaranteed to:
//! * No two samples are closer to each other than radius.
//! * No samples is outside the region [@a x_min, @a x_max].
//!
//! The type V is some vector implementation, an extremely simple example
//! being:
//!
//! template <typename T, std::size_t N>
//! class Vec
//! {
//! public:
//!   typedef T value_type;
//!   static const std::size_t size = N;
//!   Vec() {}
//!   T& operator[](std::size_t i) { return _data[i]; }
//!   const T& operator[](std::size_t i) const { return _data[i]; }
//! private:
//!   T _data[N];
//! };
//!
//! Note that the built-in std::array satisfies this interface (apart from
//! the static size constant which is handled separately).
template <typename V> inline
std::vector<V>
poissonDiskSampling(const typename V::value_type radius,
                    const V& x_min, const V& x_max,
                    const std::uint32_t max_sample_attempts = 30,
                    const std::uint32_t seed = 0)
{
  using namespace std;
  typedef typename V::value_type ValueType;
  typedef array<detail::IndexType, detail::array_size<V>::size> IndexV;
  static_assert(is_floating_point<ValueType>::value,
                "ValueType must be floating point");

  vector<V> samples;
  vector<uint32_t> active_indices;

  // Acceleration grid.
  // A grid cell this size can have at most one sample in it.
  const auto dx_grid =
    static_cast<ValueType>(0.999) * radius /
      sqrt(static_cast<ValueType>(detail::array_size<V>::size));
  const auto dx_grid_inv = static_cast<ValueType>(1) / dx_grid;

  // Compute grid dimensions using grid cell size.
  IndexV grid_dimensions;
  size_t dim_index = 0;
  for_each(begin(grid_dimensions), end(grid_dimensions),
            [&](detail::IndexType& grid_dimension) {
              grid_dimension = static_cast<detail::IndexType>(
                ceil((x_max[dim_index] - x_min[dim_index]) * dx_grid_inv));
              ++dim_index;
            });

  // Allocate and initialize grid.
  // -1 indicates no sample there; otherwise index of sample point.
  const auto grid_linear_size =
    accumulate(begin(grid_dimensions), end(grid_dimensions),
               static_cast<size_t>(1), multiplies<size_t>());
  vector<int32_t> grid(grid_linear_size, -1);

  // First sample.
  auto local_seed = seed;
  auto x_sample = detail::arrayRangeRandHash(x_min, x_max, &local_seed);
  auto j = detail::gridIndexFromSample(x_sample, x_min, dx_grid_inv);
  samples.push_back(x_sample);
  active_indices.push_back(0);
  auto k = detail::linearIndex(grid_dimensions, j);
  grid[k] = 0;

  while (!active_indices.empty()) {
    const auto rand_active_index = static_cast<size_t>(detail::rangeRandHash(
      static_cast<ValueType>(0),
      static_cast<ValueType>(active_indices.size() - 0.0001),
      local_seed++));
    const auto rand_sample_index = active_indices[rand_active_index];
    bool found_sample = false;
    IndexV j_min;
    IndexV j_max;
    for (uint32_t attempt = 0; attempt < max_sample_attempts; ++attempt) {
      x_sample = detail::sampleAnnulus(radius, samples[rand_sample_index],
                                       &local_seed);
      // Check if sample is within bounds.
      if (!detail::exclusiveInside(x_min, x_max, x_sample)) {
        goto reject_sample;
      }

      // Check proximity to nearby samples.
      for (size_t i = 0; i < detail::array_size<V>::size; ++i) {
        j_min[i] = static_cast<detail::IndexType>(detail::clamp(
          static_cast<int32_t>(0),
          static_cast<int32_t>(grid_dimensions[i] - 1),
          static_cast<int32_t>(
            (x_sample[i] - x_min[i] - radius) * dx_grid_inv)));
        j_max[i] = static_cast<detail::IndexType>(detail::clamp(
          static_cast<int32_t>(0),
          static_cast<int32_t>(grid_dimensions[i] - 1),
          static_cast<int32_t>(
            (x_sample[i] - x_min[i] + radius) * dx_grid_inv)));
      }

      for (j = j_min;;) {
        // Check if there's a sample at j that's too close to the current
        // candidate.
        k = detail::linearIndex(grid_dimensions, j);
        if (grid[k] >= 0 && grid[k] != rand_sample_index) {
          // if there is a sample point different from p
          const auto d = detail::squaredDistance(x_sample, samples[grid[k]]);
          if (d < detail::squared(radius)) {
            goto reject_sample;
          }
        }
        // Move on to next j.
        for (size_t i = 0; i < detail::array_size<V>::size; ++i) {
          ++j[i];
          if (j[i] <= j_max[i]) {
            break;
          }
          else {
            if (i == (detail::array_size<V>::size - 1)) {
              goto done_j_loop;
            }
            else {
              j[i] = j_min[i];
            }
          }
        }
      }

      // No existing samples where found to be too close to the current
      // candidate, no further attempts necessary.
      done_j_loop:
      found_sample = true;
      break;

      // The current candidate is too close to an existing sample,
      // move on to next attempt.
      reject_sample:
      ;
    }

    if (found_sample) {
      const auto q = samples.size(); // New sample index.
      samples.push_back(x_sample);
      active_indices.push_back(q);
      j = detail::gridIndexFromSample(x_sample, x_min, dx_grid_inv);
      k = detail::linearIndex(grid_dimensions, j);
      grid[k] = static_cast<int32_t>(q);
    }
    else {
      // No valid sample was found on the disk of the current sample,
      // remove the current sample from the active list.
      detail::eraseUnordered(rand_active_index, &active_indices);
    }
  }

  return samples;
}

} // namespace thinks

#endif // THINKS_POISSONDISKSAMPLING_HPP_INCLUDED

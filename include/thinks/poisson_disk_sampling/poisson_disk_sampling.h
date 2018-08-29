// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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
namespace poisson_disk_sampling {
namespace detail {

namespace util {

/// Returns @a x squared.
template <typename T> T squared(const T x) { return x * x; }


/// Returns the squared magnitude of the vector @a v.
template <typename FloatT, std::size_t N>
typename std::array<FloatT, N>::value_type SquaredMagnitude(
  const std::array<FloatT, N>& a)
{
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims >= 1, "dimensions must be >= 1");

  auto r = squared(a[0]);
  for (auto i = std::size_t{ 1 }; i < kDims; ++i) {
    r += squared(a[i]);
  }
  return r;
}


/// Returns the squared distance between the vectors @a u and @a v.
template <typename VecTraitsT, typename VecT>
typename VecTraitsT::ValueType SquaredDistance(const VecT& u, const VecT& v)
{
  static_assert(VecTraitsT::kSize >= 1, "dimensions must be >= 1");

  auto d = squared(u[0] - v[0]);
  for (auto i = std::size_t{ 1 }; i < VecTraitsT::kSize; ++i) {
    d += squared(u[i] - v[i]);
  }
  return d;
}


/// Returns true if @a x is in the range (@a x_min, @a x_max) in
/// all dimensions.
template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
bool ExclusiveInside(
  const VecT& x,
  const std::array<FloatT, N>& x_min, 
  const std::array<FloatT, N>& x_max)
{
  //static_assert(VecTraitsT::kSize == ArraySize(x_min), "todo");

  for (auto i = std::size_t{ 0 }; i < VecTraitsT::kSize; ++i) {
    const auto xi = VecTraitsT::Get(x, i);
    assert(x_min[i] < x_max[i] && "min < max");
    if (x_min[i] > xi || xi > x_max[i]) { // TODO: >= || <= ???
      return false;
    }
  }
  return true;
}


/// Returns a value in the range [@a min_value, @a max_value].
template <typename T>
T clamp(const T min_value, const T max_value, const T value)
{
  return value < min_value ?
    min_value :
    (value > max_value ? max_value : value);
}


/// Pseudo-erase the value at @a index in the vector @v. The vector @a v is
/// guaranteed to decrese in size by one. Note that the ordering of elements
/// in @a v may change as a result of calling this function.
/// Assumes that @a v is non-null and not empty.
template <class T>
void EraseUnordered(const std::size_t index, std::vector<T>* const v)
{
  assert(v != nullptr);
  assert(!v->empty() && index < v->size());

  (*v)[index] = v->back();
  v->pop_back(); // O(1).
}

} // namespace util

namespace rand {

/// Stateless and repeatable function that returns a 
/// pseduo-random number in the range [0, 0xFFFFFFFF].
inline
std::uint32_t Hash(const std::uint32_t seed)
{
  // So that we can use unsigned int literals, e.g. 42u.
  static_assert(sizeof(unsigned int) == sizeof(std::uint32_t),
    "integer size mismatch");

  auto i = std::uint32_t{ (seed ^ 12345391u) * 2654435769u };
  i ^= (i << 6u) ^ (i >> 26u);
  i *= 2654435769u;
  i += (i << 5u) ^ (i >> 12u);
  return i;
}


/// Returns a pseduo-random number in the range [0, 1].
template <typename FloatT>
FloatT NormRand(const std::uint32_t seed)
{
  static_assert(std::is_floating_point<FloatT>::value,
    "FloatT must be floating point");

  constexpr auto scale = FloatT{ 1 } / std::numeric_limits<std::uint32_t>::max();

  return scale * Hash(seed);
}


/// Returns a pseduo-random number in the 
/// range [@a offset, @a offset + @a range].
template <typename FloatT>
FloatT RangeRand(
  const FloatT offset, 
  const FloatT range, 
  const std::uint32_t seed)
{
  return offset + range * NormRand<FloatT>(seed);
}


/// Returns an array where each element has been assigned using @ref RangeRand, 
/// with bounds taken from the corresponding element
/// in @a x_min and @a x_max. 
/// Note that @a seed is incremented for each assigned element.
template <typename FloatT, std::size_t N>
std::array<FloatT, N> ArrayRangeRand(
  const std::array<FloatT, N>& x_min,
  const std::array<FloatT, N>& x_max,
  std::uint32_t* const seed)
{
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;

  assert(seed != nullptr);

  auto a = std::array<FloatT, N>{};
  for (auto i = std::size_t{ 0 }; i < kDims; ++i) {
    assert(x_min[i] < x_max[i] && "min < max");

    const auto offset = x_min[i];
    const auto range = x_max[i] - x_min[i];

    // Not worrying about seed "overflow" since it is unsigned.
    a[i] = RangeRand(offset, range, (*seed)++);
  }
  return a;
}


/// See @ref ArrayRangeRand.
template <typename VecT, typename VecTraitsT, typename FloatT, std::size_t N>
VecT VecRangeRand(
  const std::array<FloatT, N>& x_min,
  const std::array<FloatT, N>& x_max,
  std::uint32_t* const seed)
{
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsT::kSize, "todo");

  auto v = VecT{};
  const auto a = ArrayRangeRand(x_min, x_max, seed);
  for (auto i = std::size_t{ 0 }; i < kDims; ++i) {
    VecTraitsT::Set(&v, i, static_cast<VecTraitsT::ValueType>(a[i]));
  }
  return v;
}


/// Returns a pseudo-random index in the range [0, @a size - 1].
inline
std::size_t IndexRand(const std::size_t size, std::uint32_t* const seed)
{
  return static_cast<std::size_t>(
    RangeRand(
      float{ 0 },
      size - static_cast<float>(0.0001),
      (*seed)++));
}

} // namespace rand

namespace grid {

template <typename FloatT, std::size_t N>
class Grid
{
public:
  typedef std::array<std::uint16_t, N> IndexType;

  static constexpr auto kDims = N;

  Grid(
    const FloatT sample_radius, 
    const std::array<FloatT, N>& x_min,
    const std::array<FloatT, N>& x_max)
    : sample_radius_(sample_radius)
    , dx_(GetDx_(sample_radius_))
    , dx_inv_(FloatT{ 1 } / dx_)
    , x_min_(x_min)
    , x_max_(x_max)
    , size_(GetGridSize_(x_min_, x_max_, dx_inv_))
    , cells_(GetCells_(size_))
  {
  }

  FloatT sample_radius() const
  {
    return sample_radius_;
  }

  IndexType size() const
  {
    return size_;
  }

  template <typename FloatT>
  typename IndexType::value_type AxisIndex(
    const std::size_t i,
    const FloatT pos) const
  {
    typedef typename IndexType::value_type IndexValueType;

    return static_cast<IndexValueType>((pos - x_min_[i]) * dx_inv_);
  }

  template <typename VecTraitsT, typename VecT>
  IndexType IndexFromSample(const VecT& sample) const
  {
    typedef typename IndexType::value_type IndexValueType;

    static_assert(kDims == VecTraitsT::kSize, "todo");

    auto index = IndexType{};
    for (auto i = std::size_t{ 0 }; i < kDims; ++i) {
      index[i] = AxisIndex(i, VecTraitsT::Get(sample, i));
    }
    return index;
  }

  std::int32_t Cell(const IndexType& index) const
  {
    return cells_[LinearIndex(index)];
  }

  std::int32_t& Cell(const IndexType& index)
  {
    return cells_[LinearIndex(index)];
  }

private:
  FloatT sample_radius_;
  FloatT dx_;
  FloatT dx_inv_;
  std::array<FloatT, N> x_min_;
  std::array<FloatT, N> x_max_;
  IndexType size_;
  std::vector<std::int32_t> cells_;

  /// Returns a linear index into an N-dimensional array 
  /// for integer coordinates @a index.
  std::size_t LinearIndex(const IndexType& index) const
  {
    assert(index[0] < size_[0] && "index outside grid");
    auto k = static_cast<std::size_t>(index[0]);
    auto d = std::size_t{ 1 };
    for (auto i = std::size_t{ 1 }; i < kDims; ++i) {
      assert(index_[i] < size_[i] && "index outside grid");

      // Note: Not checking for "overflow".
      d *= size_[i - 1];
      k += index[i] * d;
    }
    return k;
  }

  static FloatT GetDx_(const FloatT sample_radius)
  {
    if (!(sample_radius > FloatT{ 0 })) {
      throw std::invalid_argument("sample radius must be > 0");
    }

    // A grid cell this size can have at most one sample in it.
    return static_cast<FloatT>(0.999) * sample_radius /
      std::sqrt(static_cast<FloatT>(N));
  }

  static IndexType GetGridSize_(
    const std::array<FloatT, N>& x_min,
    const std::array<FloatT, N>& x_max,
    const FloatT dx_inv)
  {
    constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;

    static_assert(kDims >= 1, "grid dimensionality must be >= 1");

    if (!(dx_inv > FloatT{ 0 })) {
      throw std::invalid_argument("todo");
    }

    // Compute size in each dimension using grid cell size (dx).
    auto grid_size = IndexType{};
    for (auto i = std::size_t{ 0 }; i < kDims; ++i) {
      if (!(x_max[i] > x_min[i])) {
        throw std::invalid_argument("todo");
      }

      grid_size[i] = static_cast<typename decltype(grid_size)::value_type>(
        std::ceil((x_max[i] - x_min[i]) * dx_inv));
    }
    return grid_size;
  }

  static std::vector<std::int32_t> GetCells_(
    const IndexType& size)
  {
    // -1 indicates no sample there; otherwise index of sample point.
    const auto linear_size = std::accumulate(
      std::begin(size), 
      std::end(size),
      std::size_t{ 1 },
      std::multiplies<std::size_t>());
    return std::vector<std::int32_t>(linear_size, -1);
  }
};

/// Named contructor that helps with type deduction.
template <typename FloatT, std::size_t N>
Grid<FloatT, N> MakeGrid(
  const FloatT sample_radius, 
  const std::array<FloatT, N>& x_min,
  const std::array<FloatT, N>& x_max)
{
  return Grid<FloatT, N>(sample_radius, x_min, x_max);
}

} // namespace grid


template <typename VecT>
struct ActiveSample
{
  VecT position;
  std::uint32_t index;
};


template <typename VecT>
ActiveSample<VecT> RandActiveSample(
  const std::vector<std::uint32_t>& active_indices,
  const std::vector<VecT>& samples,
  std::uint32_t* const seed)
{
  const auto rand_active_index = 
    rand::IndexRand(active_indices.size(), seed);
  const auto active_sample_index = active_indices[rand_active_index];
  const auto active_sample = samples[active_sample_index];
  return ActiveSample<VecT>{ active_sample, active_sample_index };
}


/// Returns a pseudo-random coordinate that is guaranteed be at a
/// distance [@a radius, 2 * @a radius] from @a center.
template<typename VecTraitsT, typename VecT, typename FloatT>
VecT RandAnnulusSample(
  const VecT& center,
  const FloatT radius, 
  std::uint32_t* const seed)
{
  typedef typename VecTraitsT::ValueType VecValueType;

  assert(seed != nullptr && "seed is null");

  // Initialize bounds.
  auto rand_min = std::array<FloatT, VecTraitsT::kSize>{};
  auto rand_max = std::array<FloatT, VecTraitsT::kSize>{};
  std::fill(std::begin(rand_min), std::end(rand_min), FloatT{ -2 });
  std::fill(std::begin(rand_max), std::end(rand_max), FloatT{ 2 });

  auto p = VecT{};
  for (;;) {
    // Generate a random component in the range [-2, 2] for each dimension.
    const auto offset = rand::ArrayRangeRand(rand_min, rand_max, seed);

    // The randomized offset is not guaranteed to be within the radial
    // distance that we need to guarantee. If we found an offset with
    // magnitude in the range (1, 2] we are done, otherwise generate a new
    // offset. Continue until a valid offset is found.
    const auto r2 = util::SquaredMagnitude(offset);
    if (FloatT{ 1 } < r2 && r2 <= FloatT{ 4 }) {
      // Found a valid offset.
      // Add the offset scaled by radius to the center coordinate to 
      // produce the final sample.
      for (auto i = std::size_t{ 0 }; i < VecTraitsT::kSize; ++i) {
        p[i] = static_cast<VecValueType>(
          VecTraitsT::Get(center, i) + radius * offset[i]);
      }
      break;
    }
  }
  return p;
}


template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
void AddSample(
  const VecT& sample,
  std::vector<VecT>* const samples, 
  std::vector<std::uint32_t>* const active_indices,
  grid::Grid<FloatT, N>* const grid)
{
  const auto sample_index = samples->size(); // New sample index.
  samples->push_back(sample);
  active_indices->push_back(static_cast<std::uint32_t>(sample_index));
  const auto grid_index = grid->IndexFromSample<VecTraitsT>(sample);
  grid->Cell(grid_index) = static_cast<std::int32_t>(sample_index); // No range check!
}


template <typename IndexT>
struct GridIndexRange
{
  IndexT min_index;
  IndexT max_index;
};


template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
GridIndexRange<typename grid::Grid<FloatT, N>::IndexType> GridNeighborhood(
  const VecT& sample,
  const grid::Grid<FloatT, N>& grid)
{
  typedef typename std::decay<decltype(grid)>::type::IndexType GridIndexType;
  typedef GridIndexType::value_type GridIndexValueType;

  constexpr auto kDims = std::tuple_size<GridIndexType>::value;
  static_assert(kDims == VecTraitsT::kSize, "todo");

  auto min_index = GridIndexType{};
  auto max_index = GridIndexType{};
  const auto grid_size = grid.size();
  for (auto i = std::size_t{ 0 }; i < kDims; ++i) {
    const auto xi = VecTraitsT::Get(sample, i);
    min_index[i] = static_cast<GridIndexValueType>(util::clamp(
      std::int32_t{ 0 },
      static_cast<std::int32_t>(grid_size[i] - 1),
      static_cast<std::int32_t>(grid.AxisIndex(i, xi - grid.sample_radius()))));
    max_index[i] = static_cast<GridIndexValueType>(util::clamp(
      std::int32_t{ 0 },
      static_cast<std::int32_t>(grid_size[i] - 1),
      static_cast<std::int32_t>(grid.AxisIndex(i, xi + grid.sample_radius()))));
  }
  return GridIndexRange<GridIndexType>{ min_index, max_index };
}


template <typename VecT, typename FloatT, std::size_t N>
bool ExistingSampleWithinRadius(
  const VecT& sample,
  const std::uint32_t active_sample_index,
  const grid::Grid<FloatT, N>& grid,
  const typename grid::Grid<FloatT, N>::IndexType& min_index,
  const typename grid::Grid<FloatT, N>::IndexType& max_index)
{
#if 0
  constexpr auto kDims = 

  //foreach index in range:
  const auto cell_index = grid.Cell(i);
  if (cell_index >= 0 && cell_index != active_sample_index) {
    const auto d = util::SquaredDistance(sample, samples[cell_index]);
    if (d < detail::squared(radius)) {
      return true;
    }
  }
  return false;
#endif 
  return true;
}

} // namespace detail


template <typename VecT>
struct VecTraits;


template <typename FloatT, std::size_t N>
struct VecTraits<std::array<FloatT, N>>
{
  typedef typename std::array<FloatT, N>::value_type ValueType;

  static constexpr auto kSize = std::tuple_size<std::array<FloatT, N>>::value;

  static ValueType Get(const std::array<FloatT, N>& vec, const std::size_t i)
  {
    assert(i < kSize && "index out of bounds");
    return vec[i];
  }

  static void Set(
    std::array<FloatT, N>* const vec, 
    const std::size_t i,
    const ValueType value)
  {
    assert(i < kSize && "index out of bounds");
    (*vec)[i] = value;
  }
};


//! Returns a list of samples that are guaranteed to:
//! * No two samples are closer to each other than @a radius.
//! * No sample is outside the region [@a x_min, @a x_max].
//!
//! The type V is some vector implementation, an extremely simple example
//! being:
//!
//! template <typename T, std::size_t N>
//! struct Vec
//! {
//!   typedef T value_type;
//!   static constexpr std::size_t size = N;
//!   Vec() {}
//!   T& operator[](std::size_t i) { return _data[i]; }
//!   const T& operator[](std::size_t i) const { return _data[i]; }
//! private:
//!   T _data[N];
//! };
//!
//! Note that the built-in std::array satisfies this interface (apart from
//! the static size constant which is handled separately).
template <typename VecT, typename FloatT, std::size_t N, typename VecTraitsT = VecTraits<VecT>>
std::vector<VecT> PoissonDiskSampling(
  const FloatT radius,
  const std::array<FloatT, N>& x_min, 
  const std::array<FloatT, N>& x_max,
  const std::uint32_t max_sample_attempts = 30,
  const std::uint32_t seed = 0)
{
  using namespace detail;

  //constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  //typedef std::array<detail::GridIndexType, kDims> GridIndex;

  static_assert(std::is_floating_point<FloatT>::value,
    "FloatT must be floating point");

  // Check 
  // - radius > 0
  // - min < max
  // - max_sample_attempts??


  // Acceleration grid.
  auto grid = detail::grid::MakeGrid(radius, x_min, x_max);

  auto samples = std::vector<VecT>{};
  auto active_indices = std::vector<std::uint32_t>{};

  // First sample.
  auto local_seed = seed;
  const auto first_sample = 
    rand::VecRangeRand<VecT, VecTraitsT>(x_min, x_max, &local_seed);
  AddSample<VecTraitsT>(first_sample, &samples, &active_indices, &grid);

  while (!active_indices.empty()) {
    // Randomly choose an active sample. A sample is considered active
    // until failed attempts have been made to generate a new sample within
    // its annulus.
    const auto active_sample = 
      RandActiveSample(active_indices, samples, &local_seed);

    auto attempt_count = std::size_t{ 0 };
    while (attempt_count < max_sample_attempts) {
      // Randomly create a candidate sample inside the active sample's annulus.
      const auto cand_sample = RandAnnulusSample<VecTraitsT>(
        active_sample.position, 
        grid.sample_radius(), 
        &local_seed);

      // Check if candidate sample is within bounds.
      if (util::ExclusiveInside<VecTraitsT>(cand_sample, x_min, x_max)) {
        // Check proximity to nearby samples.
        const auto grid_neighbors = 
          GridNeighborhood<VecTraitsT>(cand_sample, grid);
        const auto existing_sample = ExistingSampleWithinRadius(
          cand_sample,
          active_sample.index,
          grid, 
          grid_neighbors.min_index,
          grid_neighbors.max_index);
        if (!existing_sample) {
          // No existing samples where found to be too close to the 
          // candidate sample, no further attempts necessary.
          AddSample<VecTraitsT>(cand_sample, &samples, &active_indices, &grid);
          break;
        }
        // Else: The candidate sample is too close to an existing sample,
        // move on to next attempt.
      }
      ++attempt_count;
    } 

    if (attempt_count == max_sample_attempts) {
      // No valid sample was found on the disk of the active sample,
      // remove it from the active list.
      util::EraseUnordered(active_sample.index, &active_indices);
    }
  }

  return samples;
}

#if 0
for (auto j = j_min;;) {
  // Check if there's a sample at j that's too close to 
  // the current candidate.
  k = detail::LinearIndex(grid_dimensions, j);
  if (grid[k] >= 0 && grid[k] != rand_sample_index) {
    // if there is a sample point different from p
    const auto d = detail::SquaredDistance(x_sample, samples[grid[k]]);
    if (d < detail::squared(radius)) {
      goto reject_sample;
    }
  }
  // Move on to next j.
  for (size_t i = 0; i < detail::VecSize<V>::size; ++i) {
    ++j[i];
    if (j[i] <= j_max[i]) {
      break;
    }
    else {
      if (i == (detail::VecSize<V>::size - 1)) {
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

#endif

} // namespace poisson_disk_sampling
} // namespace thinks

#endif // THINKS_POISSONDISKSAMPLING_HPP_INCLUDED

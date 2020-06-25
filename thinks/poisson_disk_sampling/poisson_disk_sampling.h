// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <type_traits>
#include <vector>

#if __cplusplus >= 201402L  // C++14 or later.
#define CONSTEXPR14 constexpr
#else
#define CONSTEXPR14 inline
#endif

namespace thinks {
namespace poisson_disk_sampling_internal {

// Assumes min_value <= max_value.
template <typename ArithT>
// NOLINTNEXTLINE
CONSTEXPR14 auto clamped(const ArithT min_value, const ArithT max_value,
                         const ArithT value) noexcept -> ArithT {
  static_assert(std::is_arithmetic<ArithT>::value, "ArithT must be arithmetic");
  return value < min_value ? min_value
                           : (value > max_value ? max_value : value);
}

// Returns x squared (not checking for overflow).
template <typename ArithT>
// NOLINTNEXTLINE
CONSTEXPR14 auto squared(const ArithT x) noexcept -> ArithT {
  static_assert(std::is_arithmetic<ArithT>::value, "ArithT must be arithmetic");
  return x * x;
}

// Returns the squared magnitude of x (not checking for overflow).
template <typename ArithT, std::size_t N>
CONSTEXPR14 auto SquaredMagnitude(const std::array<ArithT, N>& x) noexcept ->
    typename std::array<ArithT, N>::value_type {
  static_assert(std::is_arithmetic<ArithT>::value, "ArithT must be arithmetic");
  constexpr auto kDims = std::tuple_size<std::array<ArithT, N>>::value;
  static_assert(kDims >= 1, "dimensions must be >= 1");

  auto m = squared(x[0]);
  for (std::size_t i = 1; i < kDims; ++i) {
    m += squared(x[i]);
  }
  return m;
}

// Returns the squared distance between the vectors u and v.
template <typename VecTraitsT, typename VecT>
CONSTEXPR14 auto SquaredDistance(const VecT& u, const VecT& v) noexcept ->
    typename VecTraitsT::ValueType {
  static_assert(VecTraitsT::kSize >= 1, "dimensions must be >= 1");

  auto d = squared(VecTraitsT::Get(u, 0) - VecTraitsT::Get(v, 0));
  for (std::size_t i = 1; i < VecTraitsT::kSize; ++i) {
    d += squared(VecTraitsT::Get(u, i) - VecTraitsT::Get(v, i));
  }
  return d;
}

// Returns true if x is element-wise inclusively inside x_min and x_max,
// otherwise false. Assumes that x_min is element-wise less than x_max.
template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
CONSTEXPR14 auto InsideBounds(const VecT& sample,
                              const std::array<FloatT, N>& x_min,
                              const std::array<FloatT, N>& x_max) noexcept
    -> bool {
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(VecTraitsT::kSize == kDims, "dimensionality mismatch");

  for (std::size_t i = 0; i < VecTraitsT::kSize; ++i) {
    const auto xi = static_cast<FloatT>(VecTraitsT::Get(sample, i));
    if (!(x_min[i] <= xi && xi <= x_max[i])) {
      return false;
    }
  }
  return true;
}

// Erase the value at given index in the vector. The vector is
// guaranteed to decrease in size by one. Note that the ordering of elements
// may change as a result of calling this function.
//
// Assumes that the vector argument is non-null and that the index is valid.
// (Cannot be called on an empty vector since no valid indices exist)
template <typename T>
void EraseUnordered(std::vector<T>* const v, const std::size_t index) noexcept {
  // Element at index gets same value as last element.
  // Last element is removed, resulting in a vector that
  // has one fewer elements with the value that was previously
  // at index.
  (*v)[index] = v->back();
  v->pop_back();  // O(1).
}

// Increment index such that repeated calls to this function enumerate
// all iterations between min_index and max_index (inclusive).
// Assumes that min_index is element-wise less than or equal to max_index.
template <typename IntT, std::size_t N>
CONSTEXPR14 auto Iterate(const std::array<IntT, N>& min_index,
                         const std::array<IntT, N>& max_index,
                         std::array<IntT, N>* const index) noexcept -> bool {
  static_assert(std::is_integral<IntT>::value, "IntT must be integral");
  constexpr auto kDims = std::tuple_size<std::array<IntT, N>>::value;
  static_assert(kDims >= 1, "dimensions must be >= 1");

  std::size_t i = 0;
  for (; i < kDims; ++i) {
    (*index)[i]++;
    if ((*index)[i] <= max_index[i]) {
      break;
    }
    (*index)[i] = min_index[i];
  }
  return i == kDims ? false : true;
}

// Stateless and repeatable function that returns a
// pseduo-random number in the range [0, 0xFFFFFFFF].
CONSTEXPR14 auto Hash(const std::uint32_t seed) noexcept -> std::uint32_t {
  // So that we can use unsigned int literals, e.g. 42u.
  static_assert(sizeof(unsigned int) == sizeof(std::uint32_t),
                "integer size mismatch");

  auto i = std::uint32_t{(seed ^ 12345391U) * 2654435769U};  // NOLINT
  i ^= (i << 6U) ^ (i >> 26U);                               // NOLINT
  i *= 2654435769U;                                          // NOLINT
  i += (i << 5U) ^ (i >> 12U);                               // NOLINT
  return i;
}

// Returns a pseduo-random number in the range [0, 0xFFFFFFFF].
// Note that seed is incremented for each invokation.
CONSTEXPR14 auto Rand(std::uint32_t* const seed) noexcept -> std::uint32_t {
  // Not worrying about seed "overflow" since it is unsigned.
  return Hash((*seed)++);
}

// Returns a pseduo-random number in the range [0, 1].
template <typename FloatT>
CONSTEXPR14 auto NormRand(std::uint32_t* const seed) noexcept -> FloatT {
  static_assert(std::is_floating_point<FloatT>::value,
                "FloatT must be floating point");
  // TODO(thinks): clamped?
  return (1 / static_cast<FloatT>(std::numeric_limits<std::uint32_t>::max())) *
         static_cast<FloatT>(Rand(seed));
}

// Returns a pseduo-random number in the range [offset, offset + range].
// Assumes range > 0.
template <typename FloatT>
CONSTEXPR14 auto RangeRand(const FloatT offset, const FloatT range,
                           std::uint32_t* const seed) noexcept -> FloatT {
  return offset + range * NormRand<FloatT>(seed);
}

// Returns an array where each element has been assigned using RangeRand,
// with bounds taken from the corresponding element in x_min and x_max.
// Assumes that x_min[i] < x_max[i].
template <typename FloatT, std::size_t N>
CONSTEXPR14 auto ArrayRangeRand(const std::array<FloatT, N>& x_min,
                                const std::array<FloatT, N>& x_max,
                                std::uint32_t* const seed) noexcept
    -> std::array<FloatT, N> {
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;

  std::array<FloatT, N> a = {};
  for (std::size_t i = 0; i < kDims; ++i) {
    a[i] =
        RangeRand(/* offset */ x_min[i], /* range */ x_max[i] - x_min[i], seed);
  }
  return a;
}

// Returns an array where each element has been assigned using RangeRand,
// with bounds taken from x_min and x_max.
// Assumes that x_min < x_max.
template <std::size_t N, typename FloatT>
CONSTEXPR14 auto ArrayRangeRand(const FloatT x_min, const FloatT x_max,
                                std::uint32_t* const seed) noexcept
    -> std::array<FloatT, N> {
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;

  const auto range = x_max - x_min;
  std::array<FloatT, N> a = {};
  for (std::size_t i = 0; i < kDims; ++i) {
    a[i] = RangeRand(/* offset */ x_min, range, seed);
  }
  return a;
}

// See ArrayRangeRand.
template <typename VecT, typename VecTraitsT, typename FloatT, std::size_t N>
CONSTEXPR14 auto VecRangeRand(const std::array<FloatT, N>& x_min,
                              const std::array<FloatT, N>& x_max,
                              std::uint32_t* const seed) noexcept -> VecT {
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(VecTraitsT::kSize == kDims, "dimensionality mismatch");

  VecT v = {};
  const auto a = ArrayRangeRand(x_min, x_max, seed);
  for (std::size_t i = 0; i < kDims; ++i) {
    VecTraitsT::Set(&v, i, static_cast<typename VecTraitsT::ValueType>(a[i]));
  }
  return v;
}

// Returns a pseudo-random index in the range [0, size - 1].
CONSTEXPR14 auto IndexRand(const std::size_t size,
                           std::uint32_t* const seed) noexcept -> std::size_t {
  constexpr auto kEps = 0.0001F;
  return static_cast<std::size_t>(
      RangeRand(float{0}, static_cast<float>(size) - kEps, seed));
}

template <typename FloatT, std::size_t N>
class Grid {
 public:
  using CellType = std::int32_t;
  using IndexType = std::array<std::int32_t, N>;

  static constexpr auto kDims = std::tuple_size<IndexType>::value;

  static_assert(kDims >= 1, "grid dimensionality must be >= 1");
  static_assert(std::is_floating_point<FloatT>::value,
                "FloatT must be floating point");

  explicit Grid(const FloatT sample_radius, const std::array<FloatT, N>& x_min,
                const std::array<FloatT, N>& x_max) noexcept
      : sample_radius_(sample_radius),
        dx_(GetDx_(sample_radius_)),
        dx_inv_(FloatT{1} / dx_),
        x_min_(x_min),
        x_max_(x_max),
        size_(GetGridSize_(x_min_, x_max_, dx_inv_)),
        cells_(GetCells_(size_)) {}

  auto sample_radius() const noexcept -> FloatT {  // NOLINT
    return sample_radius_;
  }

  auto size() const noexcept -> IndexType { return size_; }  // NOLINT

  // Returns the index for a position along the i'th axis.
  // Note that the returned index may be negative.
  template <typename FloatT2>
  // NOLINTNEXTLINE
  auto AxisIndex(const std::size_t i, const FloatT2 pos) const noexcept ->
      typename IndexType::value_type {
    using IndexValueType = typename IndexType::value_type;

    return static_cast<IndexValueType>((static_cast<FloatT>(pos) - x_min_[i]) *
                                       dx_inv_);
  }

  // Note that the returned index elements may be negative.
  template <typename VecTraitsT, typename VecT>
  // NOLINTNEXTLINE
  auto IndexFromSample(const VecT& sample) const noexcept -> IndexType {
    static_assert(VecTraitsT::kSize == kDims, "dimensionality mismatch");

    IndexType index = {};
    for (std::size_t i = 0; i < kDims; ++i) {
      index[i] = AxisIndex(i, VecTraitsT::Get(sample, i));
    }
    return index;
  }

  // NOLINTNEXTLINE
  auto Cell(const IndexType& index) const noexcept -> CellType {
    return cells_[LinearIndex_(index)];
  }

  // NOLINTNEXTLINE
  auto Cell(const IndexType& index) noexcept -> CellType& {
    return cells_[LinearIndex_(index)];
  }

 private:
  FloatT sample_radius_;
  FloatT dx_;
  FloatT dx_inv_;
  std::array<FloatT, N> x_min_;
  std::array<FloatT, N> x_max_;
  IndexType size_;
  std::vector<CellType> cells_;

  // Assumes that all elements in index are >= 0.
  // NOLINTNEXTLINE
  auto LinearIndex_(const IndexType& index) const noexcept -> std::size_t {
    auto k = static_cast<std::size_t>(index[0]);
    auto d = std::size_t{1};
    for (auto i = std::size_t{1}; i < kDims; ++i) {
      // Note: Not checking for "overflow".
      d *= static_cast<std::size_t>(size_[i - 1]);
      k += static_cast<std::size_t>(index[i]) * d;
    }
    return k;
  }

  // Assumes sample_radius is > 0.
  static auto GetDx_(const FloatT sample_radius) noexcept -> FloatT {
    // The grid cell size should be such that each cell can only
    // contain one sample. We apply a scaling factor to avoid
    // numerical issues.
    constexpr auto kEps = static_cast<FloatT>(0.001);
    constexpr auto kScale = FloatT{1} - kEps;
    return kScale * sample_radius / std::sqrt(static_cast<FloatT>(N));
  }

  // Assumes that x_min is element-wise less than x_max.
  // Assumes dx_inv > 0.
  static auto GetGridSize_(const std::array<FloatT, N>& x_min,
                           const std::array<FloatT, N>& x_max,
                           const FloatT dx_inv) noexcept -> IndexType {
    // Compute size in each dimension using grid cell size (dx).
    auto grid_size = IndexType{};
    for (auto i = std::size_t{0}; i < kDims; ++i) {
      grid_size[i] = static_cast<typename IndexType::value_type>(
          std::ceil((x_max[i] - x_min[i]) * dx_inv));
    }
    return grid_size;
  }

  static auto GetCells_(const IndexType& size) noexcept
      -> std::vector<std::int32_t> {
    // Initialize cells with value -1, indicating no sample there.
    // Cell values are later set to indices of samples.
    const auto linear_size =
        std::accumulate(std::begin(size), std::end(size), std::size_t{1},
                        std::multiplies<std::size_t>());
    return std::vector<CellType>(linear_size, -1);
  }
};

// Named contructor to help with type deduction.
template <typename FloatT, std::size_t N>
auto MakeGrid(const FloatT sample_radius, const std::array<FloatT, N>& x_min,
              const std::array<FloatT, N>& x_max) noexcept -> Grid<FloatT, N> {
  return Grid<FloatT, N>(sample_radius, x_min, x_max);
}

template <typename ArithT, std::size_t N>
CONSTEXPR14 auto ValidBounds(const std::array<ArithT, N>& x_min,
                             const std::array<ArithT, N>& x_max) noexcept
    -> bool {
  static_assert(std::is_arithmetic<ArithT>::value, "type must be arithmetic");
  constexpr auto kDims = std::tuple_size<std::array<ArithT, N>>::value;
  static_assert(kDims >= 1, "bounds dimensionality must be >= 1");

  for (std::size_t i = 0; i < kDims; ++i) {
    if (!(x_max[i] > x_min[i])) {
      return false;
    }
  }
  return true;
}

template <typename VecT>
struct ActiveSample {
  VecT position;
  std::size_t active_index;
  std::uint32_t sample_index;
};

// Returns a pseudo-randomly selected active sample from the active indices.
template <typename VecT>
auto RandActiveSample(const std::vector<std::uint32_t>& active_indices,
                      const std::vector<VecT>& samples,
                      std::uint32_t* const seed) noexcept
    -> ActiveSample<VecT> {
  ActiveSample<VecT> active_sample = {};
  active_sample.active_index = IndexRand(active_indices.size(), seed);
  active_sample.sample_index = active_indices[active_sample.active_index];
  active_sample.position = samples[active_sample.sample_index];
  return active_sample;
}

// Returns a pseudo-random coordinate that is guaranteed be at a
// distance [radius, 2 * radius] from center.
template <typename VecTraitsT, typename VecT, typename FloatT>
CONSTEXPR14 auto RandAnnulusSample(const VecT& center, const FloatT radius,
                                   std::uint32_t* const seed) noexcept -> VecT {
  VecT p = {};
  for (;;) {
    // Generate a random component in the range [-2, 2] for each dimension.
    const auto offset =
        ArrayRangeRand<VecTraitsT::kSize>(FloatT{-2}, FloatT{2}, seed);

    // The randomized offset is not guaranteed to be within the radial
    // distance that we need to guarantee. If we found an offset with
    // magnitude in the range (1, 2] we are done, otherwise generate a new
    // offset. Continue until a valid offset is found.
    const auto r2 = SquaredMagnitude(offset);
    if (FloatT{1} < r2 && r2 <= FloatT{4}) {
      // Found a valid offset.
      // Add the offset scaled by radius to the center coordinate to
      // produce the final sample.
      for (std::size_t i = 0; i < VecTraitsT::kSize; ++i) {
        VecTraitsT::Set(&p, i,
                        static_cast<typename VecTraitsT::ValueType>(
                            static_cast<FloatT>(VecTraitsT::Get(center, i)) +
                            radius * offset[i]));
      }
      break;
    }
  }
  return p;
}

// Add a new sample and update data structures.
template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
void AddSample(const VecT& sample, std::vector<VecT>* const samples,
               std::vector<std::uint32_t>* const active_indices,
               Grid<FloatT, N>* const grid) noexcept {
  const auto sample_index = samples->size();  // New sample index.
  samples->push_back(sample);
  active_indices->push_back(static_cast<std::uint32_t>(sample_index));
  const auto grid_index = grid->template IndexFromSample<VecTraitsT>(sample);
  grid->Cell(grid_index) =
      static_cast<std::int32_t>(sample_index);  // No range check!
}

template <typename IndexT>
struct GridIndexRange {
  IndexT min_index;
  IndexT max_index;
};

// Returns the grid neighborhood of the sample using the radius of the grid.
template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
auto GridNeighborhood(const VecT& sample, const Grid<FloatT, N>& grid) noexcept
    -> GridIndexRange<typename Grid<FloatT, N>::IndexType> {
  using GridIndexType = typename std::decay<decltype(grid)>::type::IndexType;
  using GridIndexValueType = typename GridIndexType::value_type;

  constexpr auto kDims = std::tuple_size<GridIndexType>::value;
  static_assert(VecTraitsT::kSize == kDims, "dimensionality mismatch");

  auto min_index = GridIndexType{};
  auto max_index = GridIndexType{};
  const auto grid_size = grid.size();
  const auto radius = grid.sample_radius();
  for (auto i = std::size_t{0}; i < kDims; ++i) {
    const auto xi_min = GridIndexValueType{0};
    const auto xi_max = static_cast<GridIndexValueType>(grid_size[i] - 1);
    const auto xi = static_cast<FloatT>(VecTraitsT::Get(sample, i));
    const auto xi_sub = grid.AxisIndex(i, xi - radius);
    const auto xi_add = grid.AxisIndex(i, xi + radius);
    min_index[i] = clamped(xi_min, xi_max, xi_sub);
    max_index[i] = clamped(xi_min, xi_max, xi_add);
  }
  return GridIndexRange<GridIndexType>{min_index, max_index};
}

// Returns true if there exists another sample within the radius used to
// construct the grid, otherwise false.
template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
auto ExistingSampleWithinRadius(
    const VecT& sample, const std::uint32_t active_sample_index,
    const std::vector<VecT>& samples, const Grid<FloatT, N>& grid,
    const typename Grid<FloatT, N>::IndexType& min_index,
    const typename Grid<FloatT, N>::IndexType& max_index) noexcept -> bool {
  auto index = min_index;
  const auto r_squared = squared(grid.sample_radius());
  do {
    const auto cell_index = grid.Cell(index);
    if (cell_index >= 0 &&
        static_cast<std::uint32_t>(cell_index) != active_sample_index) {
      const auto cell_sample = samples[static_cast<std::uint32_t>(cell_index)];
      const auto d =
          static_cast<FloatT>(SquaredDistance<VecTraitsT>(sample, cell_sample));
      if (d < r_squared) {
        return true;
      }
    }
  } while (Iterate(min_index, max_index, &index));

  return false;
}

}  // namespace poisson_disk_sampling_internal

// Generic template for vector traits. Users may specialize this template
// for their own classes.
//
// Specializations must have the following static interface, here using
// an example type 'MyVec'.
//
// struct MyVecTraits<MyVec>
// {
//   using ValueType =  ...
//   static constexpr std::size_t kSize = ...
//
//   static ValueType Get(const MyVec&, std::size_t)
//   static void Get(MyVec* const, std::size_t, ValueType)
// }
//
// See specialization for std::array below as an example.
template <typename VecT>
struct VecTraits;

// Specialization of vector traits for std::array.
template <typename FloatT, std::size_t N>
struct VecTraits<std::array<FloatT, N>> {
  using ValueType = typename std::array<FloatT, N>::value_type;
  static_assert(std::is_floating_point<ValueType>::value,
                "ValueType must be floating point");

  static constexpr auto kSize = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kSize >= 1, "kSize must be >= 1");

  // No bounds checking.
  static CONSTEXPR14 auto Get(const std::array<FloatT, N>& vec,
                              const std::size_t i) noexcept -> ValueType {
    return vec[i];
  }

  // No bounds checking.
  static CONSTEXPR14 void Set(std::array<FloatT, N>* const vec,
                              const std::size_t i,
                              const ValueType value) noexcept {
    (*vec)[i] = value;
  }
};

// Returns a list of samples with the guarantees:
// - No two samples are closer to each other than radius.
// - No sample is outside the region [x_min, x_max].
//
// The algorithm tries to fit as many samples as possible
// into the region without violating the above requirements.
//
// If the arguments are invalid an empty vector is returned.
// The arguments are invalid if:
// - Radius is <= 0, or
// - x_min[i] >= x_max[i], or
// - max_sample_attempts == 0.
template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>,
          typename VecTraitsT = VecTraits<VecT>>
auto PoissonDiskSampling(const FloatT radius,
                         const std::array<FloatT, N>& x_min,
                         const std::array<FloatT, N>& x_max,
                         const std::uint32_t max_sample_attempts = 30,
                         const std::uint32_t seed = 0) noexcept
    -> std::vector<VecT> {
  namespace pds = poisson_disk_sampling_internal;

  // Validate input.
  if (!(radius > FloatT{0}) || !(max_sample_attempts > 0) ||
      !pds::ValidBounds(x_min, x_max)) {
    // Returning an empty list of samples indicates an error,
    // since for any valid input there is always at least one sample.
    return std::vector<VecT>{};
  }

  // Acceleration grid.
  auto grid = pds::MakeGrid(radius, x_min, x_max);

  auto samples = std::vector<VecT>{};
  auto active_indices = std::vector<std::uint32_t>{};
  auto local_seed = seed;

  // Add first sample randomly within bounds.
  // No need to check (non-existing) neighbors.
  pds::AddSample<VecTraitsT>(
      pds::VecRangeRand<VecT, VecTraitsT>(x_min, x_max, &local_seed), &samples,
      &active_indices, &grid);

  while (!active_indices.empty()) {
    // Randomly choose an active sample. A sample is considered active
    // until failed attempts have been made to generate a new sample within
    // its annulus.
    const auto active_sample =
        pds::RandActiveSample(active_indices, samples, &local_seed);

    auto attempt_count = std::uint32_t{0};
    while (attempt_count < max_sample_attempts) {
      // Randomly create a candidate sample inside the active sample's annulus.
      const auto cand_sample = pds::RandAnnulusSample<VecTraitsT>(
          active_sample.position, grid.sample_radius(), &local_seed);

      // Check if candidate sample is within bounds.
      if (pds::InsideBounds<VecTraitsT>(cand_sample, x_min, x_max)) {
        // Check candidate sample proximity to nearby samples.
        const auto grid_neighbors =
            pds::GridNeighborhood<VecTraitsT>(cand_sample, grid);
        const auto existing_sample =
            pds::ExistingSampleWithinRadius<VecTraitsT>(
                cand_sample, active_sample.sample_index, samples, grid,
                grid_neighbors.min_index, grid_neighbors.max_index);
        if (!existing_sample) {
          // No existing samples where found to be too close to the
          // candidate sample, no further attempts necessary.
          pds::AddSample<VecTraitsT>(cand_sample, &samples, &active_indices,
                                     &grid);
          break;
        }
        // Else: The candidate sample is too close to an existing sample,
        // move on to next attempt.
      }
      // Else: The candidate sample is out-of-bounds.

      ++attempt_count;
    }

    if (attempt_count == max_sample_attempts) {
      // No valid sample was found on the disk of the active sample,
      // remove it from the active list.
      pds::EraseUnordered(&active_indices, active_sample.active_index);
    }
  }

  return samples;
}

}  // namespace thinks

#undef CONSTEXPR14

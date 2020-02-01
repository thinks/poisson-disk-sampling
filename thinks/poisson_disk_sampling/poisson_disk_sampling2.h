// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>

namespace thinks {

template <typename FloatT, std::size_t N>
struct Samples {
  constexpr auto kDim = N;

  std::unique_ptr<FloatT[]> positions;
  std::size_t count;
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
template <typename FloatT, std::size_t N>
auto PoissonDiskSampling(const FloatT radius,
                         const std::array<FloatT, N>& x_min,
                         const std::array<FloatT, N>& x_max,
                         const std::uint32_t max_sample_attempts = 30,
                         const std::uint32_t seed = 0) noexcept
    -> Samples<FloatT, N> {
  namespace pds = poisson_disk_sampling_internal;
  
  // Validate input.
  if (!(radius > FloatT{0}) || !(max_sample_attempts > 0) ||
      !pds::ValidBounds(x_min, x_max)) {
    // Returning an empty list of samples indicates an error,
    // since for any valid input there is always at least one sample.
    return {};
  }        



}


} // namespace thinks


// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <fstream>
#include <type_traits>

#include <tph/poisson_disk_sampling.hpp>

int main(int /*argc*/, char* /*argv*/[]) { // NOLINT
  constexpr auto kRadius = 2.F;
  constexpr auto kXMin = std::array<float, 2U>{{-10.F, -10.F}};
  constexpr auto kXMax = std::array<float, 2U>{{10.F, 10.F}};

  // Minimal parameter set passed to sampling function.
  const auto samples = tph::PoissonDiskSampling(kRadius, kXMin, kXMax);

  std::ofstream ofs{"./simple_example.txt"};
  if (!ofs) {
    return 1;
  }

  const auto sample_count = samples.size() / 2U;
  for (std::remove_cv_t<decltype(sample_count)> i = 0; i < sample_count; ++i) {
    ofs << samples[2 * i] << ", " << samples[2 * i + 1] << '\n';
  }
  ofs.close();

  return 0;
}

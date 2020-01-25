// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <cstdlib>
#include <fstream>

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

int main(int /*argc*/, char* /*argv*/[]) {  // NOLINT
  constexpr auto kRadius = 2.F;
  constexpr std::array<float, 2> kXMin = {-10.F, -10.F};
  constexpr std::array<float, 2> kXMax = {10.F, 10.F};

  // Minimal amount of information provided to sampling function.
  const auto samples = thinks::PoissonDiskSampling(kRadius, kXMin, kXMax);

  std::ofstream ofs{"./simple_example.txt"};
  if (!ofs) {
    return EXIT_FAILURE;
  }

  for (const auto& sample : samples) {
    ofs << sample[0] << ", " << sample[1] << '\n';
  }
  ofs.close();

  return EXIT_SUCCESS;
}

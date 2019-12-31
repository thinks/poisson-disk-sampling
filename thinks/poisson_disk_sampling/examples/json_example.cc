// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>

#include "nlohmann/json.hpp"
#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

int main(int /*argc*/, char* /*argv*/[]) {  // NOLINT
  using json = nlohmann::json;

  constexpr auto kRadius = 3.F;
  constexpr std::array<float, 2> kXMin = {-10.F, -10.F};
  constexpr std::array<float, 2> kXMax = {10.F, 10.F};
  constexpr auto kMaxSampleAttempts = std::uint32_t{30};
  constexpr auto kSeed = std::uint32_t{0};

  const auto samples = thinks::PoissonDiskSampling(kRadius, kXMin, kXMax,
                                                   kMaxSampleAttempts, kSeed);

  json j;
  j["min"] = kXMin;
  j["max"] = kXMax;
  j["radius"] = kRadius;
  j["samples"] = samples;

  std::ofstream ofs{"./json_example.json"};
  if (!ofs) {
    return EXIT_FAILURE;
  }
  ofs << std::setw(4) << j;
  ofs.close();

  return EXIT_SUCCESS;
}
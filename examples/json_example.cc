// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "json_example.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>

#include "nlohmann/json.hpp"
#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

namespace examples {

void JsonExample(const std::string& filename) {
  using json = nlohmann::json;

  constexpr auto radius = 3.f;
  constexpr std::array<float, 2> x_min = {-10.f, -10.f};
  constexpr std::array<float, 2> x_max = {10.f, 10.f};
  constexpr auto max_sample_attempts = std::uint32_t{30};
  constexpr auto seed = std::uint32_t{0};

  auto samples = thinks::PoissonDiskSampling(radius, x_min, x_max,
                                             max_sample_attempts, seed);

  json j;
  j["min"] = x_min;
  j["max"] = x_max;
  j["radius"] = radius;
  j["samples"] = samples;

  std::ofstream ofs{filename};
  assert(ofs);
  ofs << std::setw(4) << j;
  ofs.close();
}

}  // namespace examples

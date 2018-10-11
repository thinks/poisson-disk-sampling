// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <json_example.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iomanip>

#include <nlohmann/json.hpp>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>


namespace examples {

void JsonExample(const std::string& filename)
{
  namespace pds = thinks::poisson_disk_sampling;
  using json = nlohmann::json;

  auto radius = 3.f;
  auto x_min = std::array<float, 2>{ -10.f, -10.f };
  auto x_max = std::array<float, 2>{ 10.f, 10.f };
  constexpr auto max_sample_attempts = std::uint32_t{ 300 };
  constexpr auto seed = std::uint32_t{ 0 };

  auto samples = pds::PoissonDiskSampling(
    radius, 
    x_min, 
    x_max, 
    max_sample_attempts, 
    seed);

  json j;
  j["min"] = x_min;// { x_min[0], x_min[1] };
  j["max"] = x_max;// { x_max[0], x_max[1] };
  j["radius"] = radius;
  j["samples"] = samples;

  auto ofs = std::ofstream(filename);
  assert(ofs);
  ofs << std::setw(4) << j;
  ofs.close();
}

} // namespace examples

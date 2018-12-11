// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <simple_example.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>

namespace examples {

void SimpleExample(const std::string& filename) {
  namespace pds = thinks::poisson_disk_sampling;

  // Minimal amount of information provided to sampling function.
  constexpr auto radius = 2.f;
  const auto x_min = std::array<float, 2>{{-10.f, -10.f}};
  const auto x_max = std::array<float, 2>{{10.f, 10.f}};
  const auto samples = pds::PoissonDiskSampling(radius, x_min, x_max);

  auto ofs = std::ofstream(filename);
  assert(ofs);
  for (const auto& sample : samples) {
    ofs << sample[0] << ", " << sample[1] << std::endl;
  }
  ofs.close();
}

}  // namespace examples

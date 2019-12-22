// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "vec_traits_passed_in_example.h"

#include <cassert>
#include <cstdint>
#include <fstream>

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

namespace {

struct Vec3 {
  float v[3];  // NOLINT
};

struct Vec3Traits {
  using ValueType = float;

  static constexpr auto kSize = 3;

  static auto Get(const Vec3& v, const std::size_t i) -> ValueType {
    return v.v[i];
  }

  static void Set(Vec3* const v, const std::size_t i, const ValueType val) {
    v->v[i] = val;
  }
};

}  // namespace

namespace examples {

void VecTraitsPassedInExample(const std::string& filename) {
  constexpr auto radius = 2.F;
  const std::array<float, 3> x_min = {-10.F, -10.F, -10.F};
  const std::array<float, 3> x_max = {10.F, 10.F, 10.F};
  const auto samples = thinks::PoissonDiskSampling<float, 3, Vec3, Vec3Traits>(
      radius, x_min, x_max);

  std::ofstream ofs{filename};
  assert(ofs);
  for (const auto& sample : samples) {
    ofs << sample.v[0] << ", " << sample.v[1] << ", " << sample.v[2] << '\n';
  }
  ofs.close();
}

}  // namespace examples

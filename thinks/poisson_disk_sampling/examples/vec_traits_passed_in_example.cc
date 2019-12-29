// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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

int main(int /*argc*/, char* /*argv*/[]) {  // NOLINT
  constexpr auto kRadius = 2.F;
  constexpr std::array<float, 3> kXMin = {-10.F, -10.F, -10.F};
  constexpr std::array<float, 3> kXMax = {10.F, 10.F, 10.F};
  const auto samples = thinks::PoissonDiskSampling<float, 3, Vec3, Vec3Traits>(
      kRadius, kXMin, kXMax);

  std::ofstream ofs{"./vec_traits_passed_in_example.txt"};
  assert(ofs);
  for (const auto& sample : samples) {
    ofs << sample.v[0] << ", " << sample.v[1] << ", " << sample.v[2] << '\n';
  }
  ofs.close();

  return 0;
}

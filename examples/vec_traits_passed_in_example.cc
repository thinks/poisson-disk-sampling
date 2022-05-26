// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cstdint>
#include <cstdlib>
#include <fstream>

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

#if __cplusplus >= 201402L  // C++14 or later.
#define PDS_CEXPR constexpr
#else
#define PDS_CEXPR inline
#endif

namespace {

struct Vec3 {
  float v[3];  // NOLINT
};

// Traits specialized in namespace other than thinks::
struct Vec3Traits {
  using ValueType = float;

  static constexpr auto kSize = 3;

  static PDS_CEXPR auto Get(const Vec3& v, const std::size_t i) -> ValueType {
    return v.v[i];
  }

  static PDS_CEXPR void Set(Vec3* const v, const std::size_t i,
                            const ValueType val) {
    v->v[i] = val;
  }
};

}  // namespace

int main(int /*argc*/, char* /*argv*/[]) {  // NOLINT
  constexpr auto kRadius = 2.F;
  constexpr std::array<float, 3> kXMin = {-10.F, -10.F, -10.F};
  constexpr std::array<float, 3> kXMax = {10.F, 10.F, 10.F};

  // Traits explicitly provided here.
  const auto samples = thinks::PoissonDiskSampling<float, 3, Vec3, Vec3Traits>(
      kRadius, kXMin, kXMax);

  std::ofstream ofs{"./vec_traits_passed_in_example.txt"};
  if (!ofs) {
    return EXIT_FAILURE;
  }

  for (const auto& sample : samples) {
    ofs << sample.v[0] << ", " << sample.v[1] << ", " << sample.v[2] << '\n';
  }
  ofs.close();

  return EXIT_SUCCESS;
}

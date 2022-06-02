// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <cstdint> // std::uint32_t, etc.
#include <fstream>
#include <type_traits> // std::decay

#include <nlohmann/json.hpp>

#include <tph/poisson_disk_sampling.hpp>

int main(int /*argc*/, char* /*argv*/[]) { // NOLINT
  constexpr auto kRadius = 3.F;
  constexpr auto kXMin = std::array<float, 2U>{-10.F, -10.F};
  constexpr auto kXMax = std::array<float, 2U>{10.F, 10.F};
  constexpr auto kMaxSampleAttempts = std::uint32_t{30};
  constexpr auto kSeed = std::uint32_t{0};

  const auto samples = tph::PoissonDiskSampling(kRadius, kXMin, kXMax, kMaxSampleAttempts, kSeed);

  const auto sample_count = samples.size() / 2U;
  std::vector<std::array<float, 2U>> sample_vecs(sample_count);
  for (std::decay_t<decltype(sample_count)> i = 0; i < sample_count; ++i) {
    sample_vecs[i][0] = samples[2 * i];
    sample_vecs[i][1] = samples[2 * i + 1];
  }

  nlohmann::json json;
  json["min"] = kXMin;
  json["max"] = kXMax;
  json["radius"] = kRadius;
  json["samples"] = sample_vecs;

  std::ofstream ofs{"./json_example.json"};
  if (!ofs) {
    return 1;
  }
  ofs << json.dump(/*indent=*/2);
  ofs.close();

  return 0;
}

// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "catch2/catch.hpp"
#include "thinks/poisson_disk_sampling/test/catch_utils.h"

namespace {

template <typename T, std::size_t N>
struct Vec {
  T m[N];
};

}  // namespace

namespace thinks {

// Specializations of VecTraits in thinks:: namespace.
template <typename T, std::size_t N>
struct VecTraits<Vec<T, N>> {
  using ValueType = std::decay_t<decltype(*Vec<T, N>::m)>;

  static_assert(sizeof(Vec<T, N>) == N * sizeof(T));
  static constexpr auto kSize = sizeof(Vec<T, N>) / sizeof(ValueType);

  static constexpr auto Get(const Vec<T, N>& v, const std::size_t i)
      -> ValueType {
    return v.m[i];
  }

  static constexpr void Set(Vec<T, N>* const v, const std::size_t i,
                            const ValueType val) {
    v->m[i] = val;
  }
};

namespace {

template <typename T>
constexpr auto squared(const T x) -> T {  // NOLINT
  return x * x;
}

template <typename VecTraitsT, typename VecT>
auto SquaredDistance(const VecT& u, const VecT& v) ->
    typename VecTraitsT::ValueType {
  static_assert(VecTraitsT::kSize >= 1, "vec dimensionality must be >= 1");

  auto d = squared(VecTraitsT::Get(u, 0) - VecTraitsT::Get(v, 0));
  for (std::size_t i = 1; i < VecTraitsT::kSize; ++i) {
    d += squared(VecTraitsT::Get(u, i) - VecTraitsT::Get(v, i));
  }
  return d;
}

// O(N^2) verification. Verifies that the distance between each possible
// sample pair meets the Poisson requirement, i.e. is greater than some radius.
// Returns true if Poisson criteria is met for all samples, otherwise false.
template <typename VecT, typename FloatT>
auto VerifyPoisson(const std::vector<VecT>& samples, const FloatT radius)
    -> bool {
  if (samples.empty()) {
    return false;
  }
  if (samples.size() == 1) {
    return true;
  }

  // Setup threading.
  const auto thread_count = std::thread::hardware_concurrency() > 0
                                ? std::thread::hardware_concurrency()
                                : 1;
  const auto sample_count = samples.size();
  const auto chunk_size = sample_count / thread_count + (sample_count % thread_count != 0);

  // Launch threads.
  const auto r_squared = radius * radius;
  std::vector<std::future<bool>> futures;
  for (std::size_t i = 0; i < thread_count; ++i) {
    futures.push_back(std::async(std::launch::async, [&]() {
      const std::size_t begin = i * chunk_size;
      const std::size_t end = std::min((i + 1) * chunk_size, sample_count);
      for (std::size_t j = begin; j < end; ++j) {
        for (const auto& s : samples) {
          if (std::addressof(samples[j]) != std::addressof(s)) {
            const auto sqr_dist = static_cast<FloatT>(
                SquaredDistance<thinks::VecTraits<VecT>>(samples[j], s));

            // Fail for NaN.
            if (!(sqr_dist >= r_squared)) {
              return false;
            }
          }
        }
      }
      return true;
    }));
  }

  // Check results.
  return std::all_of(std::begin(futures), std::end(futures),
                     [](std::future<bool>& f) { return f.get(); });  
}

template <typename VecT, typename FloatT, std::size_t N>
constexpr auto SampleInsideBounds(const VecT& sample,
                                  const std::array<FloatT, N>& x_min,
                                  const std::array<FloatT, N>& x_max) -> bool {
  using VecTraitsType = thinks::VecTraits<VecT>;

  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsType::kSize,
                "bounds/samples dimensionality mismatch");

  for (std::size_t i = 0; i < kDims; ++i) {
    const auto xi = static_cast<FloatT>(VecTraitsType::Get(sample, i));

    // Fail if x_min > x_max.
    if (!(x_min[i] <= xi && xi <= x_max[i])) {
      return false;
    }
  }
  return true;
}

// Returns true if all samples are within the bounds specified by x_min and
// x_max, otherwise false.
template <typename VecT, typename FloatT, std::size_t N>
auto VerifyBounds(const std::vector<VecT>& samples,
                  const std::array<FloatT, N>& x_min,
                  const std::array<FloatT, N>& x_max) -> bool {
  using VecTraitsType = thinks::VecTraits<VecT>;

  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsType::kSize,
                "bounds/samples dimensionality mismatch");

  if (samples.empty()) {
    return false;
  }

  // Setup threading.
  const auto thread_count = std::thread::hardware_concurrency() > 0
                                ? std::thread::hardware_concurrency()
                                : 1;
  const auto sample_count = samples.size();
  const auto chunk_size = sample_count / thread_count + (sample_count % thread_count != 0);

  // Launch threads.
  std::vector<std::future<bool>> futures;
  for (std::size_t i = 0; i < thread_count; ++i) {
    futures.push_back(std::async(std::launch::async, [&]() {
      const std::size_t begin = i * chunk_size;
      const std::size_t end = std::min((i + 1) * chunk_size, sample_count);
      for (std::size_t j = begin; j < end; ++j) {
        if (!SampleInsideBounds(samples[j], x_min, x_max)) {
          return false;
        }
      }
      return true;
    }));
  }

  // Check results.
  return std::all_of(std::begin(futures), std::end(futures),
                     [](std::future<bool>& f) { return f.get(); });
}

template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
auto VerifyPoissonDiskSampling(const FloatT radius,
                               const std::array<FloatT, N>& x_min,
                               const std::array<FloatT, N>& x_max,
                               const std::uint32_t max_sample_attempts = 30,
                               const std::uint32_t seed = 0) -> bool {
  const auto samples = thinks::PoissonDiskSampling<FloatT, N, VecT>(
      radius, x_min, x_max, max_sample_attempts, seed);
  return VerifyBounds(samples, x_min, x_max) && VerifyPoisson(samples, radius);
}

TEST_CASE("Test samples <std::array>", "[container]") {
  SECTION("N = 2") {
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 2>{{-10.F, -10.F}},
        /* x_max */ std::array<float, 2>{{10.F, 10.F}}));
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 2>{{-10.0, -10.0}},
        /* x_max */ std::array<double, 2>{{10.0, 10.0}}));
  }
  SECTION("N = 3") {
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 3>{{-10.F, -10.F, -10.F}},
        /* x_max */ std::array<float, 3>{{10.F, 10.F, 10.F}}));
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 3>{{-10.0, -10.0, -10.0}},
        /* x_max */ std::array<double, 3>{{10.0, 10.0, 10.0}}));
  }
  SECTION("N = 4") {
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 4>{{-10.F, -10.F, -10.F, -10.F}},
        /* x_max */ std::array<float, 4>{{10.F, 10.F, 10.F, 10.F}}));
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 4>{{-10.0, -10.0, -10.0, -10.0}},
        /* x_max */ std::array<double, 4>{{10.0, 10.0, 10.0, 10.0}}));
  }
}

TEST_CASE("Test samples <Vec>", "[container]") {
  SECTION("N = 2") {
    using VecType = Vec<float, 2>;
    REQUIRE(VerifyPoissonDiskSampling<float, 2, VecType>(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 2>{{-10.F, -10.F}},
        /* x_max */ std::array<float, 2>{{10.F, 10.F}}));
    REQUIRE(VerifyPoissonDiskSampling<double, 2, VecType>(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 2>{{-10.0, -10.0}},
        /* x_max */ std::array<double, 2>{{10.0, 10.0}}));
  }
  SECTION("N = 3") {
    using VecType = Vec<float, 3>;
    REQUIRE(VerifyPoissonDiskSampling<float, 3, VecType>(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 3>{{-10.F, -10.F, -10.F}},
        /* x_max */ std::array<float, 3>{{10.F, 10.F, 10.F}}));
    REQUIRE(VerifyPoissonDiskSampling<double, 3, VecType>(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 3>{{-10.0, -10.0, -10.0}},
        /* x_max */ std::array<double, 3>{{10.0, 10.0, 10.0}}));
  }
  SECTION("N = 4") {
    using VecType = Vec<float, 4>;
    REQUIRE(VerifyPoissonDiskSampling<float, 4, VecType>(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 4>{{-10.F, -10.F, -10.F, -10.F}},
        /* x_max */ std::array<float, 4>{{10.F, 10.F, 10.F, 10.F}}));
    REQUIRE(VerifyPoissonDiskSampling<double, 4, VecType>(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 4>{{-10.0, -10.0, -10.0, -10.0}},
        /* x_max */ std::array<double, 4>{{10.0, 10.0, 10.0, 10.0}}));
  }
}

#if 0
TEST_CASE("Invalid arguments", "[container]") {
  SECTION("Negative radius") {
    constexpr auto kRadius = -1.F;

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
        (TestPoissonDiskSampling<float, 2>(kRadius)), std::invalid_argument,
        thinks::ExceptionContentMatcher{"radius must be positive, was -1"});
  }

  SECTION("Min >= max") {
    // Not relevant here.
    constexpr auto kRadius = 1.F;

    constexpr auto kXMinValue = 10.F;
    constexpr auto kXMaxValue = -10.F;

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
        (TestPoissonDiskSampling<float, 2>(kRadius, kXMinValue, kXMaxValue)),
        std::invalid_argument,
        thinks::ExceptionContentMatcher{
            "invalid bounds - max must be greater than min, was min: [10, 10], "
            "max: [-10, -10]"});

    {
      constexpr std::array<float, 2> kXMin = {10.F, -10.F};
      constexpr std::array<float, 2> kXMax = {-10.F, 10.F};

      // Strange () work-around for catch framework.
      REQUIRE_THROWS_MATCHES((TestPoissonDiskSampling<float, 2>(kXMin, kXMax)),
                             std::invalid_argument,
                             thinks::ExceptionContentMatcher{
                                 "invalid bounds - max must be greater "
                                 "than min, was min: [10, -10], "
                                 "max: [-10, 10]"});
    }
    {
      constexpr std::array<float, 2> kXMin = {-10.F, 10.F};
      constexpr std::array<float, 2> kXMax = {10.F, -10.F};

      // Strange () work-around for catch framework.
      REQUIRE_THROWS_MATCHES((TestPoissonDiskSampling<float, 2>(kXMin, kXMax)),
                             std::invalid_argument,
                             thinks::ExceptionContentMatcher{
                                 "invalid bounds - max must be greater "
                                 "than min, was min: [-10, 10], "
                                 "max: [10, -10]"});
    }
  }

  SECTION("Zero sample attempts") {
    // Not relevant here.
    constexpr auto kRadius = 1.F;
    constexpr auto kXMinValue = -10.F;
    constexpr auto kXMaxValue = 10.F;

    constexpr auto kMaxSampleAttempts = std::uint32_t{0};

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
        (TestPoissonDiskSampling<float, 2>(kRadius, kXMinValue, kXMaxValue,
                                           kMaxSampleAttempts)),
        std::invalid_argument,
        thinks::ExceptionContentMatcher{
            "max sample attempts must be greater than zero, was 0"});
  }
}
#endif
}  // namespace
}  // namespace thinks
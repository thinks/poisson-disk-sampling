// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <future>
#include <limits>
#include <thread>
#include <vector>

#include "catch2/catch.hpp"

#if __cplusplus >= 201402L  // C++14 or later.
#define CONSTEXPR14 constexpr
#else
#define CONSTEXPR14 inline
#endif

namespace {

// Simple small vector type.
template <typename T, std::size_t N>
struct Vec {
  std::array<T, N> m;
};

}  // namespace

namespace thinks {

// Specializations of VecTraits in thinks:: namespace.
template <typename T, std::size_t N>
struct VecTraits<Vec<T, N>> {
  using ValueType = typename decltype(Vec<T, N>::m)::value_type;

  static_assert(sizeof(Vec<T, N>) == N * sizeof(T),
                "Vec type must be tightly packed");
  static constexpr auto kSize = sizeof(Vec<T, N>) / sizeof(ValueType);

  static constexpr auto Get(const Vec<T, N>& v, const std::size_t i) noexcept
      -> ValueType {
    return v.m[i];
  }

  static CONSTEXPR14 void Set(Vec<T, N>* const v, const std::size_t i,
                             const ValueType val) noexcept {
    v->m[i] = val;
  }
};

namespace {

auto ThreadCount(const std::size_t max_thread_count) noexcept -> std::size_t {
  return std::thread::hardware_concurrency() > 0
             ? std::min(static_cast<std::size_t>(
                            std::thread::hardware_concurrency()),
                        max_thread_count)
             : 1;
}

template <typename T>
constexpr auto squared(const T x) noexcept -> T {  // NOLINT
  return x * x;
}

// Returns the squared distance between u and v. Not checking for overflow.
template <typename VecTraitsT, typename VecT>
CONSTEXPR14 auto SquaredDistance(const VecT& u, const VecT& v) noexcept ->
    typename VecTraitsT::ValueType {
  static_assert(VecTraitsT::kSize >= 1, "vec dimensionality must be >= 1");

  auto d = squared(VecTraitsT::Get(u, 0) - VecTraitsT::Get(v, 0));
  for (std::size_t i = 1; i < VecTraitsT::kSize; ++i) {
    d += squared(VecTraitsT::Get(u, i) - VecTraitsT::Get(v, i));
  }
  return d;
}

// Brute-force (with some tricks) verification that the distance between each
// possible sample pair meets the Poisson requirement, i.e. is greater than some
// radius. Returns true if Poisson criteria is met for all samples, otherwise
// false.
template <typename VecT, typename FloatT>
auto VerifyPoisson(const std::vector<VecT>& samples,
                   const FloatT radius) noexcept -> bool {
  if (samples.empty()) {
    return false;
  }
  if (samples.size() == 1) {
    return true;
  }

  // Setup threading.
  // Avoid spawning more threads than there are samples (although very
  // unlikely).
  const auto sample_count = samples.size();
  const auto thread_count = ThreadCount(sample_count);

  // Launch threads.
  std::vector<std::future<bool>> futures;
  for (std::size_t i = 0; i < thread_count; ++i) {
    // Each thread works on local copy of samples.
    futures.emplace_back(std::async(std::launch::async, [=]() {
      // We know that distance is symmetrical, such that
      // dist(s[j], s[k]) == dist(s[k], s[j]). Therefore
      // we need only compute the upper half of the matrix (excluding diagonal).
      //
      // Load balance threads such that "short" (small j) and "long" (large j)
      // columns are divided evenly among threads.
      const auto r_sqr = squared(radius);
      for (std::size_t j = i; j < sample_count; j += thread_count) {
        const auto sj = samples[j];
        const auto k_max = j;
        for (std::size_t k = 0; k < k_max; ++k) {
          const auto sk = samples[k];
          const auto dist_sqr = static_cast<FloatT>(
              SquaredDistance<thinks::VecTraits<VecT>>(sj, sk));

          // Fail for NaN.
          if (!(dist_sqr > r_sqr)) {
            return false;
          }
        }
      }
      return true;
    }));
  }

  // clang-format off
  // Check results.
  for (auto&& f : futures) { f.wait(); }
  return std::all_of(std::begin(futures), std::end(futures),
                     [](std::future<bool>& f) { return f.get(); });
  // clang-format on
}

template <typename VecT, typename FloatT, std::size_t N>
CONSTEXPR14 auto SampleInsideBounds(const VecT& sample,
                                   const std::array<FloatT, N>& x_min,
                                   const std::array<FloatT, N>& x_max) noexcept
    -> bool {
  using VecTraitsType = thinks::VecTraits<VecT>;

  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsType::kSize,
                "sample/bounds dimensionality mismatch");

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
                  const std::array<FloatT, N>& x_max) noexcept -> bool {
  if (samples.empty()) {
    return false;
  }

  // Setup threading.
  // Avoid spawning more threads than there are samples (although very
  // unlikely).
  const auto sample_count = samples.size();
  const auto thread_count = ThreadCount(sample_count);
  const auto chunk_size =
      sample_count / thread_count + (sample_count % thread_count != 0);

  // Launch threads.
  std::vector<std::future<bool>> futures;
  for (std::size_t i = 0; i < thread_count; ++i) {
    // Each thread works on local copy of samples.
    futures.emplace_back(std::async(std::launch::async, [=]() {
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

  // clang-format off
  // Check results.
  for (auto&& f : futures) { f.wait(); }
  return std::all_of(std::begin(futures), std::end(futures),
                     [](std::future<bool>& f) { return f.get(); });
  // clang-format on
}

template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
auto VerifyPoissonDiskSampling(const FloatT radius,
                               const std::array<FloatT, N>& x_min,
                               const std::array<FloatT, N>& x_max,
                               const std::uint32_t max_sample_attempts = 30,
                               const std::uint32_t seed = 0) noexcept -> bool {
  FloatT config_radius = radius;

#ifndef NDEBUG
  // Reduce the number of samples for debug builds to decrease
  // verification times. Larger radius gives fewer samples.
  config_radius *= 2;
#endif

  const auto samples = thinks::PoissonDiskSampling<FloatT, N, VecT>(
      config_radius, x_min, x_max, max_sample_attempts, seed);
  return VerifyBounds(samples, x_min, x_max) && VerifyPoisson(samples, radius);
}

template <typename FloatT>
constexpr auto SampleTestRadius() noexcept -> FloatT {
  return FloatT{2};
}

template <typename FloatT, std::size_t N>
struct SampleTestBounds;

// clang-format off
template <typename FloatT>
struct SampleTestBounds<FloatT, 2> {
  static constexpr auto x_min() noexcept -> std::array<FloatT, 2> { return {{-100, -100}}; }  // NOLINT
  static constexpr auto x_max() noexcept -> std::array<FloatT, 2> { return {{100, 100}}; }    // NOLINT
};

template <typename FloatT>
struct SampleTestBounds<FloatT, 3> {
  static constexpr auto x_min() noexcept -> std::array<FloatT, 3> { return {{-20, -20, -20}}; }  // NOLINT
  static constexpr auto x_max() noexcept -> std::array<FloatT, 3> { return {{20, 20, 20}}; }     // NOLINT
};

template <typename FloatT>
struct SampleTestBounds<FloatT, 4> {
  static constexpr auto x_min() noexcept -> std::array<FloatT, 4> { return {{-10, -10, -10, -10}}; }  // NOLINT
  static constexpr auto x_max() noexcept -> std::array<FloatT, 4> { return {{10, 10, 10, 10}}; }      // NOLINT
};
// clang-format on

TEST_CASE("Verify samples <std::array>", "[container]") {
  SECTION("N = 2") {
    REQUIRE(VerifyPoissonDiskSampling(SampleTestRadius<float>(),
                                      SampleTestBounds<float, 2>::x_min(),
                                      SampleTestBounds<float, 2>::x_max()));
    REQUIRE(VerifyPoissonDiskSampling(SampleTestRadius<double>(),
                                      SampleTestBounds<double, 2>::x_min(),
                                      SampleTestBounds<double, 2>::x_max()));
  }
  SECTION("N = 3") {
    REQUIRE(VerifyPoissonDiskSampling(SampleTestRadius<float>(),
                                      SampleTestBounds<float, 3>::x_min(),
                                      SampleTestBounds<float, 3>::x_max()));
    REQUIRE(VerifyPoissonDiskSampling(SampleTestRadius<double>(),
                                      SampleTestBounds<double, 3>::x_min(),
                                      SampleTestBounds<double, 3>::x_max()));
  }
  SECTION("N = 4") {
    REQUIRE(VerifyPoissonDiskSampling(SampleTestRadius<float>(),
                                      SampleTestBounds<float, 4>::x_min(),
                                      SampleTestBounds<float, 4>::x_max()));
    REQUIRE(VerifyPoissonDiskSampling(SampleTestRadius<double>(),
                                      SampleTestBounds<double, 4>::x_min(),
                                      SampleTestBounds<double, 4>::x_max()));
  }
}

TEST_CASE("Verify samples <Vec>", "[container]") {
  SECTION("N = 2") {
    using VecType = Vec<float, 2>;
    REQUIRE(VerifyPoissonDiskSampling<float, 2, VecType>(
        SampleTestRadius<float>(), SampleTestBounds<float, 2>::x_min(),
        SampleTestBounds<float, 2>::x_max()));
    REQUIRE(VerifyPoissonDiskSampling<double, 2, VecType>(
        SampleTestRadius<double>(), SampleTestBounds<double, 2>::x_min(),
        SampleTestBounds<double, 2>::x_max()));
  }
  SECTION("N = 3") {
    using VecType = Vec<float, 3>;
    REQUIRE(VerifyPoissonDiskSampling<float, 3, VecType>(
        SampleTestRadius<float>(), SampleTestBounds<float, 3>::x_min(),
        SampleTestBounds<float, 3>::x_max()));
    REQUIRE(VerifyPoissonDiskSampling<double, 3, VecType>(
        SampleTestRadius<double>(), SampleTestBounds<double, 3>::x_min(),
        SampleTestBounds<double, 3>::x_max()));
  }
  SECTION("N = 4") {
    using VecType = Vec<float, 4>;
    REQUIRE(VerifyPoissonDiskSampling<float, 4, VecType>(
        SampleTestRadius<float>(), SampleTestBounds<float, 4>::x_min(),
        SampleTestBounds<float, 4>::x_max()));
    REQUIRE(VerifyPoissonDiskSampling<double, 4, VecType>(
        SampleTestRadius<double>(), SampleTestBounds<double, 4>::x_min(),
        SampleTestBounds<double, 4>::x_max()));
  }
}

TEST_CASE("Verify max sampling attempts") {
  // Verify that we get a denser sampling, i.e. more samples,
  // when we increase the max sample attempts parameter (with
  // all other parameters constant).

  constexpr auto kRadius = 0.5F;
  constexpr auto kXMin = std::array<float, 2>{{-10.F, -10.F}};
  constexpr auto kXMax = std::array<float, 2>{{10.F, 10.F}};

  const auto samples_10 = thinks::PoissonDiskSampling(
      kRadius, kXMin, kXMax, /* max_sample_attempts */ 10);
  const auto samples_40 = thinks::PoissonDiskSampling(
      kRadius, kXMin, kXMax, /* max_sample_attempts */ 40);

  REQUIRE(samples_10.size() < samples_40.size());
}

TEST_CASE("Verify seed") {
  // Verify that different seeds give different sample distributions.

  constexpr auto kRadius = 0.5F;
  constexpr auto kXMin = std::array<float, 2>{{-10.F, -10.F}};
  constexpr auto kXMax = std::array<float, 2>{{10.F, 10.F}};
  constexpr auto kMaxSampleAttempts = std::uint32_t{20};

  const auto samples_1981 = thinks::PoissonDiskSampling(
      kRadius, kXMin, kXMax, kMaxSampleAttempts, /* seed */ 1981);
  const auto samples_1337 = thinks::PoissonDiskSampling(
      kRadius, kXMin, kXMax, kMaxSampleAttempts, /* seed */ 1337);

  // For each sample in the first point set compute the smallest
  // distance checking every sample in the second point set.
  // Then, if the smallest distance is larger than some threshold
  // we say that the sample from the first point set is distinct
  // from every sample in the second point set. Thus the two
  // distributions must be different.
  const auto sample_count_1981 = samples_1981.size();
  const auto sample_count_1337 = samples_1337.size();
  auto distinct_sample_found = false;
  for (std::size_t i = 0; i < sample_count_1981; ++i) {
    const auto si = samples_1981[i];
    auto min_sqr_dist = std::numeric_limits<float>::max();
    for (std::size_t j = 0; j < sample_count_1337; ++j) {
      const auto sj = samples_1981[j];
      min_sqr_dist = std::min(
          min_sqr_dist,
          SquaredDistance<thinks::VecTraits<std::array<float, 2>>>(si, sj));
    }
    if (min_sqr_dist > 0.1F) {  // NOLINT
      distinct_sample_found = true;
      break;
    }
  }

  REQUIRE(distinct_sample_found);
}

struct ValidBounds {
  static constexpr auto XMin() noexcept -> std::array<float, 2> {
    return {{-1, -1}};
  }
  static constexpr auto XMax() noexcept -> std::array<float, 2> {
    return {{1, 1}};
  }
};

TEST_CASE("Invalid arguments", "[container]") {
  constexpr auto kValidRadius = 1.F;

  SECTION("radius == 0") {
    const auto samples = thinks::PoissonDiskSampling(
        /* radius */ 0.F,  // NOLINT
        ValidBounds::XMin(), ValidBounds::XMax());
    REQUIRE(samples.empty());
  }

  SECTION("radius < 0") {
    const auto samples = thinks::PoissonDiskSampling(
        /* radius */ -1.F,  // NOLINT
        ValidBounds::XMin(), ValidBounds::XMax());
    REQUIRE(samples.empty());
  }

  SECTION("x_min == x_max") {
    {
      const auto samples = thinks::PoissonDiskSampling(
          kValidRadius,
          /* x_min */ std::array<float, 2>{{1.F, 1.F}},   // NOLINT
          /* x_max */ std::array<float, 2>{{1.F, 1.F}});  // NOLINT
      REQUIRE(samples.empty());
    }
    {
      const auto samples = thinks::PoissonDiskSampling(
          kValidRadius,
          /* x_min */ std::array<float, 2>{{1.F, -1.F}},  // NOLINT
          /* x_max */ std::array<float, 2>{{1.F, 1.F}});  // NOLINT
      REQUIRE(samples.empty());
    }
    {
      const auto samples = thinks::PoissonDiskSampling(
          kValidRadius,
          /* x_min */ std::array<float, 2>{{-1.F, 1.F}},  // NOLINT
          /* x_max */ std::array<float, 2>{{1.F, 1.F}});  // NOLINT
      REQUIRE(samples.empty());
    }
  }

  SECTION("x_min > x_max") {
    {
      const auto samples = thinks::PoissonDiskSampling(
          kValidRadius,
          /* x_min */ std::array<float, 2>{{1.F, 1.F}},     // NOLINT
          /* x_max */ std::array<float, 2>{{-1.F, -1.F}});  // NOLINT
      REQUIRE(samples.empty());
    }
    {
      const auto samples = thinks::PoissonDiskSampling(
          kValidRadius,
          /* x_min */ std::array<float, 2>{{1.F, -1.F}},   // NOLINT
          /* x_max */ std::array<float, 2>{{-1.F, 1.F}});  // NOLINT
      REQUIRE(samples.empty());
    }
    {
      const auto samples = thinks::PoissonDiskSampling(
          kValidRadius,
          /* x_min */ std::array<float, 2>{{-1.F, 1.F}},   // NOLINT
          /* x_max */ std::array<float, 2>{{1.F, -1.F}});  // NOLINT
      REQUIRE(samples.empty());
    }
  }

  SECTION("max_sample_attempts == 0") {
    const auto samples = thinks::PoissonDiskSampling(
        kValidRadius, ValidBounds::XMin(), ValidBounds::XMax(),
        /* max_sample_attempts */ std::uint32_t{0});  // NOLINT
    REQUIRE(samples.empty());
  }
}

}  // namespace
}  // namespace thinks

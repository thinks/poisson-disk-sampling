// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <future>
//#include <iomanip>
//#include <iostream>
#include <thread>
#include <vector>

#include "catch2/catch.hpp"

namespace {

// Simple small vector type.
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

  static_assert(sizeof(Vec<T, N>) == N * sizeof(T),
                "Vec type must be tightly packed");
  static constexpr auto kSize = sizeof(Vec<T, N>) / sizeof(ValueType);

  static constexpr auto Get(const Vec<T, N>& v, const std::size_t i) noexcept
      -> ValueType {
    return v.m[i];
  }

  static constexpr void Set(Vec<T, N>* const v, const std::size_t i,
                            const ValueType val) noexcept {
    v->m[i] = val;
  }
};

namespace {

auto ThreadCount(const std::size_t max_thread_count) noexcept -> std::size_t {
  // return 1;
  return std::thread::hardware_concurrency() > 0
             ? std::min<std::size_t>(std::thread::hardware_concurrency(),
                                     max_thread_count)
             : 1;
}

template <typename T>
constexpr auto squared(const T x) noexcept -> T {  // NOLINT
  return x * x;
}

// Returns the squared distance between u and v. Not checking for overflow.
template <typename VecTraitsT, typename VecT>
constexpr auto SquaredDistance(const VecT& u, const VecT& v) noexcept ->
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
      // columns are divided equally among threads.
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
            /*std::ostringstream oss;
            oss << std::setprecision(17);
            oss << "s[" << j << "]: {" << sj[0] << ", " << sj[1] << ", " <<
            sj[2]
                << "}\n";
            oss << "s[" << k << "]: {" << sk[0] << ", " << sk[1] << ", " <<
            sk[2]
                << "}\n";
            oss << dist_sqr << " | " << r_sqr << "\n";
            oss << std::sqrt(dist_sqr) << " | " << radius << "\n";
            oss << "fail\n";
            std::cerr << oss.str();*/
            return false;
          }
        }
      }
      return true;
    }));
  }

  for (auto&& f : futures) {
    f.wait();
  }

  // Check results.
  return std::all_of(std::begin(futures), std::end(futures),
                     [](std::future<bool>& f) { return f.get(); });
}

template <typename VecT, typename FloatT, std::size_t N>
constexpr auto SampleInsideBounds(const VecT& sample,
                                  const std::array<FloatT, N>& x_min,
                                  const std::array<FloatT, N>& x_max) noexcept
    -> bool {
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

  // Check results.
  for (auto&& f : futures) {
    f.wait();
  }

  return std::all_of(std::begin(futures), std::end(futures),
                     [](std::future<bool>& f) { return f.get(); });
}

template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
auto VerifyPoissonDiskSampling(const FloatT radius,
                               const std::array<FloatT, N>& x_min,
                               const std::array<FloatT, N>& x_max,
                               const std::uint32_t max_sample_attempts = 30,
                               const std::uint32_t seed = 0) noexcept -> bool {
  FloatT scaled_radius = radius;

// clang-format off
  #ifndef NDEBUG
    // Reduce the number of samples for debug builds to decrease 
    // verification times.
    scaled_radius *= 2;
  #endif
  // clang-format on

  const auto samples = thinks::PoissonDiskSampling<FloatT, N, VecT>(
      scaled_radius, x_min, x_max, max_sample_attempts, seed);
  return VerifyBounds(samples, x_min, x_max) && VerifyPoisson(samples, radius);
}

TEST_CASE("Test samples <std::array>", "[container]") {
  SECTION("N = 2") {
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 2>{{-500.F, -500.F}},
        /* x_max */ std::array<float, 2>{{500.F, 500.F}}));
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 2>{{-500.0, -500.0}},
        /* x_max */ std::array<double, 2>{{500.0, 500.0}}));
  }
  SECTION("N = 3") {
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 3>{{-50.F, -50.F, -50.F}},
        /* x_max */ std::array<float, 3>{{50.F, 50.F, 50.F}}));
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 3>{{-50.0, -50.0, -50.0}},
        /* x_max */ std::array<double, 3>{{50.0, 50.0, 50.0}}));
  }
  SECTION("N = 4") {
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 4>{{-5.F, -5.F, -5.F, -5.F}},
        /* x_max */ std::array<float, 4>{{5.F, 5.F, 5.F, 5.F}}));
    REQUIRE(VerifyPoissonDiskSampling(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 4>{{-5.0, -5.0, -5.0, -5.0}},
        /* x_max */ std::array<double, 4>{{5.0, 5.0, 5.0, 5.0}}));
  }
}

TEST_CASE("Test samples <Vec>", "[container]") {
  SECTION("N = 2") {
    using VecType = Vec<float, 2>;
    REQUIRE(VerifyPoissonDiskSampling<float, 2, VecType>(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 2>{{-500.F, -500.F}},
        /* x_max */ std::array<float, 2>{{500.F, 500.F}}));
    REQUIRE(VerifyPoissonDiskSampling<double, 2, VecType>(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 2>{{-500.0, -500.0}},
        /* x_max */ std::array<double, 2>{{500.0, 500.0}}));
  }
  SECTION("N = 3") {
    using VecType = Vec<float, 3>;
    REQUIRE(VerifyPoissonDiskSampling<float, 3, VecType>(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 3>{{-50.F, -50.F, -50.F}},
        /* x_max */ std::array<float, 3>{{50.F, 50.F, 50.F}}));
    REQUIRE(VerifyPoissonDiskSampling<double, 3, VecType>(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 3>{{-50.0, -50.0, -50.0}},
        /* x_max */ std::array<double, 3>{{50.0, 50.0, 50.0}}));
  }
  SECTION("N = 4") {
    using VecType = Vec<float, 4>;
    REQUIRE(VerifyPoissonDiskSampling<float, 4, VecType>(
        /* radius */ 2.F,
        /* x_min */ std::array<float, 4>{{-5.F, -5.F, -5.F, -5.F}},
        /* x_max */ std::array<float, 4>{{5.F, 5.F, 5.F, 5.F}}));
    REQUIRE(VerifyPoissonDiskSampling<double, 4, VecType>(
        /* radius */ 2.0,
        /* x_min */ std::array<double, 4>{{-5.F, -5.F, -5.F, -5.F}},
        /* x_max */ std::array<double, 4>{{5.F, 5.F, 5.F, 5.F}}));
  }
}

TEST_CASE("Invalid arguments", "[container]") {
  SECTION("radius == 0") {
    const auto samples = thinks::PoissonDiskSampling(
        /* radius */ 0.F,
        /* x_min */ std::array<float, 2>{{-1.F, -1.F}},
        /* x_max */ std::array<float, 2>{{1.F, 1.F}});
    REQUIRE(samples.empty());
  }

  SECTION("radius < 0") {
    const auto samples = thinks::PoissonDiskSampling(
        /* radius */ -1.F,
        /* x_min */ std::array<float, 2>{{-1.F, -1.F}},
        /* x_max */ std::array<float, 2>{{1.F, 1.F}});
    REQUIRE(samples.empty());
  }

  SECTION("x_min == x_max") {
    const auto samples = thinks::PoissonDiskSampling(
        /* radius */ 1.F,
        /* x_min */ std::array<float, 2>{{1.F, 1.F}},
        /* x_max */ std::array<float, 2>{{1.F, 1.F}});
    REQUIRE(samples.empty());
  }

  SECTION("x_min > x_max") {
    {
      const auto samples = thinks::PoissonDiskSampling(
          /* radius */ 1.F,
          /* x_min */ std::array<float, 2>{{1.F, 1.F}},
          /* x_max */ std::array<float, 2>{{-1.F, -1.F}});
      REQUIRE(samples.empty());
    }
    {
      const auto samples = thinks::PoissonDiskSampling(
          /* radius */ 1.F,
          /* x_min */ std::array<float, 2>{{-1.F, 1.F}},
          /* x_max */ std::array<float, 2>{{1.F, -1.F}});
      REQUIRE(samples.empty());
    }
    {
      const auto samples = thinks::PoissonDiskSampling(
          /* radius */ 1.F,
          /* x_min */ std::array<float, 2>{{1.F, -1.F}},
          /* x_max */ std::array<float, 2>{{-1.F, 1.F}});
      REQUIRE(samples.empty());
    }
  }

  SECTION("max_sample_attempts == 0") {
    const auto samples = thinks::PoissonDiskSampling(
        /* radius */ 1.F,
        /* x_min */ std::array<float, 2>{{-1.F, -1.F}},
        /* x_max */ std::array<float, 2>{{1.F, 1.F}},
        /* max_sample_attempts */ std::uint32_t{0});
    REQUIRE(samples.empty());
  }
}

}  // namespace
}  // namespace thinks
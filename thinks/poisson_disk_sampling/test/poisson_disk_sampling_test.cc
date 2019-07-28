// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "catch2/catch.hpp"

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"
#include "thinks/poisson_disk_sampling/test/catch_utils.h"

namespace {

template <typename T>
struct Vec2 {
  using ValueType = T;
  T x;
  T y;
};

template <typename T>
struct Vec3 {
  using ValueType = T;
  T x;
  T y;
  T z;
};

template <typename T>
struct Vec4 {
  using ValueType = T;
  T x;
  T y;
  T z;
  T w;
};

template <typename T>
constexpr T squared(const T x) {
  return x * x;
}

template <std::size_t N, typename T>
std::array<T, N> FilledArray(const T value) {
  auto a = std::array<T, N>{};
  std::fill(std::begin(a), std::end(a), value);
  return a;
}

template <typename VecTraitsT, typename VecT>
typename VecTraitsT::ValueType SquaredDistance(const VecT& u, const VecT& v) {
  static_assert(VecTraitsT::kSize >= 1, "vec dimensionality must be >= 1");

  auto d = squared(VecTraitsT::Get(u, 0) - VecTraitsT::Get(v, 0));
  for (std::size_t i = 1; i < VecTraitsT::kSize; ++i) {
    d += squared(VecTraitsT::Get(u, i) - VecTraitsT::Get(v, i));
  }
  return d;
}

// O(N^2) verification. Verifies that the distance between each possible
// sample pair meets the Poisson requirement, i.e. is greater than some radius.
template <typename VecT, typename FloatT>
bool VerifyPoisson(const std::vector<VecT>& samples, const FloatT radius) {
  if (samples.empty()) {
    return false;
  }

  const auto iend = std::end(samples);
  const auto ibegin = std::begin(samples);
  const auto r_squared = radius * radius;

  for (auto u = ibegin; u != iend; ++u) {
    for (auto v = ibegin; v != iend; ++v) {
      const auto sqr_dist =
          static_cast<FloatT>(SquaredDistance<thinks::VecTraits<VecT>>(*u, *v));
      if (&(*u) != &(*v) && sqr_dist < r_squared) {
        return false;
      }
    }
  }
  return true;
}

// Returns true if all samples are within the bounds specified by x_min and x_max.
template <typename VecT, typename FloatT, std::size_t N>
bool VerifyBounds(const std::vector<VecT>& samples,
                  const std::array<FloatT, N>& x_min,
                  const std::array<FloatT, N>& x_max) {
  using VecTraitsType = thinks::VecTraits<VecT>;

  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsType::kSize, "dimensionality mismatch");

  for (auto v = std::begin(samples); v != std::end(samples); ++v) {
    for (std::size_t i = 0; i < kDims; ++i) {
      const auto xi = static_cast<FloatT>(VecTraitsType::Get(*v, i));
      if (x_min[i] > xi || xi > x_max[i]) {
        return false;
      }
    }
  }
  return true;
}

template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
void TestPoissonDiskSampling(const std::array<FloatT, N>& x_min,
                             const std::array<FloatT, N>& x_max,
                             const FloatT radius = FloatT{2},
                             const std::uint32_t max_sample_attempts = 30,
                             const std::uint32_t seed = 0) {
  const auto samples = thinks::PoissonDiskSampling<FloatT, N, VecT>(
      radius, x_min, x_max, max_sample_attempts, seed);

  REQUIRE(VerifyPoisson(samples, radius));
  REQUIRE(VerifyBounds(samples, x_min, x_max));
}

template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
void TestPoissonDiskSampling(const FloatT radius = FloatT{2},
                             const FloatT x_min_value = FloatT{-10},
                             const FloatT x_max_value = FloatT{10},
                             const std::uint32_t max_sample_attempts = 30,
                             const std::uint32_t seed = 0) {
  const auto x_min = FilledArray<N>(x_min_value);
  const auto x_max = FilledArray<N>(x_max_value);
  TestPoissonDiskSampling(x_min, x_max, radius, max_sample_attempts, seed);
}

}  // namespace

namespace thinks {

template <typename T>
struct VecTraits<Vec2<T>> {
  using ValueType = typename Vec2<T>::ValueType;

  static constexpr auto kSize = 2;

  static /*constexpr*/ ValueType Get(const Vec2<T>& v, const std::size_t i) {
    return *(&v.x + i);
  }

  static /*constexpr*/ void Set(Vec2<T>* const v, const std::size_t i,
                                const ValueType val) {
    *(&v->x + i) = val;
  }
};

template <typename T>
struct VecTraits<Vec3<T>> {
  using ValueType = typename Vec3<T>::ValueType;

  static constexpr auto kSize = 3;

  static /*constexpr*/ ValueType Get(const Vec3<T>& v, const std::size_t i) {
    return *(&v.x + i);
  }

  static /*constexpr*/ void Set(Vec3<T>* const v, const std::size_t i,
                                const ValueType val) {
    *(&v->x + i) = val;
  }
};

template <typename T>
struct VecTraits<Vec4<T>> {
  using ValueType = typename Vec4<T>::ValueType;

  static constexpr auto kSize = 4;

  static /*constexpr*/ ValueType Get(const Vec4<T>& v, const std::size_t i) {
    return *(&v.x + i);
  }

  static /*constexpr*/ void Set(Vec4<T>* const v, const std::size_t i,
                                const ValueType val) {
    *(&v->x + i) = val;
  }
};

}  // namespace thinks

TEST_CASE("Test samples <std::array>", "[container]") {
  SECTION("N = 2") {
    TestPoissonDiskSampling<float, 2>();
    TestPoissonDiskSampling<double, 2>();
  }
  SECTION("N = 3") {
    TestPoissonDiskSampling<float, 3>();
    TestPoissonDiskSampling<double, 3>();
  }
  SECTION("N = 4") {
    TestPoissonDiskSampling<float, 4>();
    TestPoissonDiskSampling<double, 4>();
  }
}

TEST_CASE("Test samples <Vec>", "[container]") {
  SECTION("N = 2") {
    using VecType = Vec2<float>;
    TestPoissonDiskSampling<float, 2, VecType>();
    TestPoissonDiskSampling<double, 2, VecType>();
  }
  SECTION("N = 3") {
    using VecType = Vec3<float>;
    TestPoissonDiskSampling<float, 3, VecType>();
    TestPoissonDiskSampling<double, 3, VecType>();
  }
  SECTION("N = 4") {
    using VecType = Vec4<float>;
    TestPoissonDiskSampling<float, 4, VecType>();
    TestPoissonDiskSampling<double, 4, VecType>();
  }
}

TEST_CASE("Invalid arguments", "[container]") {
  SECTION("Negative radius") {
    constexpr auto radius = -1.f;

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
        (TestPoissonDiskSampling<float, 2>(radius)), std::invalid_argument,
        thinks::ExceptionContentMatcher{"radius must be positive, was -1"});
  }

  SECTION("Min >= max") {
    // Not relevant here.
    constexpr auto radius = 1.f;

    constexpr auto x_min_value = 10.f;
    constexpr auto x_max_value = -10.f;

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
        (TestPoissonDiskSampling<float, 2>(radius, x_min_value, x_max_value)),
        std::invalid_argument,
        thinks::ExceptionContentMatcher{
            "invalid bounds - max must be greater than min, was min: [10, 10], "
            "max: [-10, -10]"});

    {
      constexpr std::array<float, 2> x_min = {10.f, -10.f};
      constexpr std::array<float, 2> x_max = {-10.f, 10.f};

      // Strange () work-around for catch framework.
      REQUIRE_THROWS_MATCHES(
          (TestPoissonDiskSampling<float, 2>(x_min, x_max)),
          std::invalid_argument,
          thinks::ExceptionContentMatcher{"invalid bounds - max must be greater "
                                         "than min, was min: [10, -10], "
                                         "max: [-10, 10]"});
    }
    {
      constexpr std::array<float, 2> x_min = {-10.f, 10.f};
      constexpr std::array<float, 2> x_max = {10.f, -10.f};

      // Strange () work-around for catch framework.
      REQUIRE_THROWS_MATCHES(
          (TestPoissonDiskSampling<float, 2>(x_min, x_max)),
          std::invalid_argument,
          thinks::ExceptionContentMatcher{"invalid bounds - max must be greater "
                                         "than min, was min: [-10, 10], "
                                         "max: [10, -10]"});
    }
  }

  SECTION("Zero sample attempts") {
    // Not relevant here.
    constexpr auto radius = 1.f;
    constexpr auto x_min_value = -10.f;
    constexpr auto x_max_value = 10.f;

    constexpr auto max_sample_attempts = std::uint32_t{0};

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
        (TestPoissonDiskSampling<float, 2>(radius, x_min_value, x_max_value,
                                           max_sample_attempts)),
        std::invalid_argument,
        thinks::ExceptionContentMatcher{
            "max sample attempts must be greater than zero, was 0"});
  }
}

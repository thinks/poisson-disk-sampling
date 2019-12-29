// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "catch2/catch.hpp"
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
constexpr auto squared(const T x) -> T {  // NOLINT
  return x * x;
}

template <std::size_t N, typename T>
auto FilledArray(const T value) -> std::array<T, N> {
  auto a = std::array<T, N>{};
  std::fill(std::begin(a), std::end(a), value);
  return a;
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
// sample pair meets the Poisson requirement, i.e. is greater than some kRadius.
template <typename VecT, typename FloatT>
auto VerifyPoisson(const std::vector<VecT>& samples, const FloatT kRadius)
    -> bool {
  if (samples.empty()) {
    return false;
  }

  const auto iend = std::end(samples);
  const auto ibegin = std::begin(samples);
  const auto r_squared = kRadius * kRadius;

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

// Returns true if all samples are within the bounds specified by kXMin and
// kXMax.
template <typename VecT, typename FloatT, std::size_t N>
auto VerifyBounds(const std::vector<VecT>& samples,
                  const std::array<FloatT, N>& kXMin,
                  const std::array<FloatT, N>& kXMax) -> bool {
  using VecTraitsType = thinks::VecTraits<VecT>;

  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsType::kSize, "dimensionality mismatch");

  for (auto v = std::begin(samples); v != std::end(samples); ++v) {
    for (std::size_t i = 0; i < kDims; ++i) {
      const auto xi = static_cast<FloatT>(VecTraitsType::Get(*v, i));
      if (kXMin[i] > xi || xi > kXMax[i]) {
        return false;
      }
    }
  }
  return true;
}

template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
void TestPoissonDiskSampling(const std::array<FloatT, N>& kXMin,
                             const std::array<FloatT, N>& kXMax,
                             const FloatT kRadius = FloatT{2},
                             const std::uint32_t kMaxSampleAttempts = 30,
                             const std::uint32_t seed = 0) {
  const auto samples = thinks::PoissonDiskSampling<FloatT, N, VecT>(
      kRadius, kXMin, kXMax, kMaxSampleAttempts, seed);

  REQUIRE(VerifyPoisson(samples, kRadius));
  REQUIRE(VerifyBounds(samples, kXMin, kXMax));
}

template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
void TestPoissonDiskSampling(const FloatT kRadius = FloatT{2},
                             const FloatT kXMinValue = FloatT{-10},
                             const FloatT kXMaxValue = FloatT{10},
                             const std::uint32_t kMaxSampleAttempts = 30,
                             const std::uint32_t seed = 0) {
  const auto kXMin = FilledArray<N>(kXMinValue);
  const auto kXMax = FilledArray<N>(kXMaxValue);
  TestPoissonDiskSampling(kXMin, kXMax, kRadius, kMaxSampleAttempts, seed);
}

}  // namespace

namespace thinks {
// Specialization of VecTraits in namespace.

template <typename T>
struct VecTraits<Vec2<T>> {
  using ValueType = typename Vec2<T>::ValueType;

  static constexpr auto kSize = 2;

  static constexpr auto Get(const Vec2<T>& v, const std::size_t i)
      -> ValueType {
    return *(&v.x + i);
  }

  static constexpr void Set(Vec2<T>* const v, const std::size_t i,
                            const ValueType val) {
    *(&v->x + i) = val;
  }
};

template <typename T>
struct VecTraits<Vec3<T>> {
  using ValueType = typename Vec3<T>::ValueType;

  static constexpr auto kSize = 3;

  static /*constexpr*/ auto Get(const Vec3<T>& v, const std::size_t i)
      -> ValueType {
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

  static /*constexpr*/ auto Get(const Vec4<T>& v, const std::size_t i)
      -> ValueType {
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
  SECTION("Negative kRadius") {
    constexpr auto kRadius = -1.F;

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
        (TestPoissonDiskSampling<float, 2>(kRadius)), std::invalid_argument,
        thinks::ExceptionContentMatcher{"kRadius must be positive, was -1"});
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

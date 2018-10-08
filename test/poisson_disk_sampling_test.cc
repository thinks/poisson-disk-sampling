// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream> // tmp?!?!
#include <vector>

#include <catch2/catch.hpp>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>
#include <utils/catch_utils.h>

namespace pds = thinks::poisson_disk_sampling;


namespace {

template <typename T>
struct Vec2
{
  typedef T ValueType;
  T x;
  T y;
};


template <typename T>
struct Vec3
{
  typedef T ValueType;
  T x;
  T y;
  T z;
};


template <typename T>
struct Vec4
{
  typedef T ValueType;
  T x;
  T y;
  T z;
  T w;
};


template <typename T>
T squared(const T x)
{
  return x * x;
}


template <std::size_t N, typename T>
std::array<T, N> FilledArray(const T value)
{
  auto a = std::array<T, N>{};
  std::fill(std::begin(a), std::end(a), value);
  return a;
}


template <typename VecTraitsT, typename VecT>
typename VecTraitsT::ValueType SquaredDistance(
  const VecT& u, 
  const VecT& v)
{
  static_assert(VecTraitsT::kSize >= 1, 
    "vec dimensionality must be >= 1");

  auto d = squared(VecTraitsT::Get(u, 0) - VecTraitsT::Get(v, 0));
  for (auto i = std::size_t{ 1 }; i < VecTraitsT::kSize; ++i) {
    d += squared(VecTraitsT::Get(u, i) - VecTraitsT::Get(v, i));
  }
  return d;
}


/*!
O(N^2) verification. Verifies that the distance between each possible
sample pair meets the Poisson requirement, i.e. is greater than some radius.
*/
template <typename VecT, typename FloatT>
bool VerifyPoisson(
  const std::vector<VecT>& samples,
  const FloatT radius)
{
  const auto iend = std::end(samples);
  const auto ibegin = std::begin(samples);
  const auto r_squared = radius * radius;

  for (auto u = ibegin; u != iend; ++u) {
    for (auto v = ibegin; v != iend; ++v) {
      if (&(*u) != &(*v) && 
          SquaredDistance<pds::VecTraits<VecT>>(*u, *v) < r_squared) {
        return false;
      }
    }
  }
  return true;
}


/*!
Returns true if all samples are within the bounds specified by x_min and x_max.
*/
template <typename VecT, typename FloatT, std::size_t N>
bool VerifyBounds(
  const std::vector<VecT>& samples, 
  const std::array<FloatT, N>& x_min, 
  const std::array<FloatT, N>& x_max)
{
  typedef pds::VecTraits<VecT> VecTraitsType;

  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsType::kSize, "dimensionality mismatch");

  for (auto v = std::begin(samples); v != std::end(samples); ++v) {
    for (auto i = std::size_t{ 0 }; i < kDims; ++i) {
      const auto xi = VecTraitsType::Get(*v, i);
      if (x_min[i] > xi || xi > x_max[i]) {
        return false;
      }
    }
  }
  return true;
}


template <typename FloatT, std::size_t N, typename VecT = std::array<FloatT, N>>
void TestPoissonDiskSampling(
  const FloatT radius = FloatT{ 2 },
  const FloatT x_min_value = FloatT{ -10 },
  const FloatT x_max_value = FloatT{ 10 },
  const std::uint32_t max_sample_attempts = 30,
  const std::uint32_t seed = 0)
{
  const auto x_min = FilledArray<N>(x_min_value);
  const auto x_max = FilledArray<N>(x_max_value);
  const auto samples = pds::PoissonDiskSampling<FloatT, N, VecT>(
    radius, 
    x_min, 
    x_max, 
    max_sample_attempts, 
    seed);

  REQUIRE(VerifyPoisson(samples, radius));
  REQUIRE(VerifyBounds(samples, x_min, x_max));
}

} // namespace


namespace thinks {
namespace poisson_disk_sampling {

template <typename T>
struct VecTraits<Vec2<T>>
{
  typedef typename Vec2<T>::ValueType ValueType;

  static constexpr auto kSize = 2;

  static 
  ValueType Get(const Vec2<T>& v, const std::size_t i)
  {
    return *(&v.x + i);
  }

  static 
  void Set(Vec2<T>* const v, const std::size_t i, const ValueType val)
  {
    *(&v->x + i) = val;
  }
};

template <typename T>
struct VecTraits<Vec3<T>>
{
  typedef typename Vec3<T>::ValueType ValueType;

  static constexpr auto kSize = 3;

  static 
  ValueType Get(const Vec3<T>& v, const std::size_t i)
  {
    return *(&v.x + i);
  }

  static 
  void Set(Vec3<T>* const v, const std::size_t i, const ValueType val)
  {
    *(&v->x + i) = val;
  }
};

template <typename T>
struct VecTraits<Vec4<T>>
{
  typedef typename Vec4<T>::ValueType ValueType;

  static constexpr auto kSize = 4;

  static 
  ValueType Get(const Vec4<T>& v, const std::size_t i)
  {
    return *(&v.x + i);
  }

  static 
  void Set(Vec4<T>* const v, const std::size_t i, const ValueType val)
  {
    *(&v->x + i) = val;
  }
};

} // namespace poisson_disk_sampling
} // namespace thinks


TEST_CASE("Test samples <std::array>", "[container]")
{
  SECTION("N = 2")
  {
    TestPoissonDiskSampling<float, 2>();
    TestPoissonDiskSampling<double, 2>();
  }
  SECTION("N = 3")
  {
    TestPoissonDiskSampling<float, 3>();
    TestPoissonDiskSampling<double, 3>();
  }
  SECTION("N = 4")
  {
    TestPoissonDiskSampling<float, 4>();
    TestPoissonDiskSampling<double, 4>();
  }
}


TEST_CASE("Test samples <Vec>", "[container]")
{
  SECTION("N = 2")
  {
    typedef Vec2<float> VecType;
    TestPoissonDiskSampling<float, 2, VecType>();
    TestPoissonDiskSampling<double, 2, VecType>();
  }
  SECTION("N = 3")
  {
    typedef Vec3<float> VecType;
    TestPoissonDiskSampling<float, 3, VecType>();
    TestPoissonDiskSampling<double, 3, VecType>();
  }
  SECTION("N = 4")
  {
    typedef Vec4<float> VecType;
    TestPoissonDiskSampling<float, 4, VecType>();
    TestPoissonDiskSampling<double, 4, VecType>();
  }
}


TEST_CASE("Invalid arguments", "[container]")
{
  SECTION("Negative radius")
  {
    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
      (TestPoissonDiskSampling<float, 2>(-1.f)),
      std::invalid_argument,
      utils::ExceptionContentMatcher{ "radius must be positive, was -1" });
  }

  SECTION("Min >= max")
  {
    // Not relevant here.
    constexpr auto radius = 1.f;

    constexpr auto x_min_value = 10.f;
    constexpr auto x_max_value = -10.f;

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
      (TestPoissonDiskSampling<float, 2>(radius, x_min_value, x_max_value)),
      std::invalid_argument,
      utils::ExceptionContentMatcher{ 
        "invalid bounds - max must be greater than min, was min: [10, 10], max: [-10, -10]" });
  }

  SECTION("Zero sample attempts")
  {
    // Not relevant here.
    constexpr auto radius = 1.f;
    constexpr auto x_min_value = -10.f;
    constexpr auto x_max_value = 10.f;

    constexpr auto max_sample_attempts = std::uint32_t{ 0 };

    // Strange () work-around for catch framework.
    REQUIRE_THROWS_MATCHES(
      (TestPoissonDiskSampling<float, 2>(radius, x_min_value, x_max_value,
        max_sample_attempts)),
      std::invalid_argument,
      utils::ExceptionContentMatcher{ 
        "max sample attempts must be greater than zero, was 0" });
  }
}

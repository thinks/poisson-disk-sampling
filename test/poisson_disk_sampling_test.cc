// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include <catch.hpp>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>

namespace pds = thinks::poisson_disk_sampling;


namespace {

/// Minimum implementation of the vector interface required by the 
/// poisson disk sampling method.
template <typename T, std::size_t N>
class Vec
{
public:
  typedef T ValueType;
  static const std::size_t kSize = N;
  Vec() {}
  T& operator[](const std::size_t i) { return data_[i]; }
  const T& operator[](const std::size_t i) const { return data_[i]; }
private:
  T data_[N];
};


/// Helper struct for keeping track of inputs to the algorithm.
template <typename FloatT, std::size_t N>
struct Input
{
  Input(
    const FloatT radius, 
    const std::array<FloatT, N>& x_min, 
    const std::array<FloatT, N>& x_max,
    const std::uint32_t max_sample_attempts, 
    const std::uint32_t seed)
    : radius(radius)
    , x_min(x_min)
    , x_max(x_max)
    , max_sample_attempts(max_sample_attempts)
    , seed(seed) 
  {
  }

  FloatT radius;
  std::array<FloatT, N> x_min;
  std::array<FloatT, N> x_max;
  std::uint32_t max_sample_attempts;
  std::uint32_t seed;
};


template <typename T>
T squared(const T x)
{
  return x * x;
}


/// Returns the squared distance between the vectors @a u and @a v.
template <typename VecTraitsT, typename VecT>
typename VecTraitsT::ValueType SquaredDistance(const VecT& u, const VecT& v)
{
  static_assert(VecTraitsT::kSize > 0, "todo");

  auto d = squared(u[0] - v[0]);
  for (auto i = std::size_t{ 1 }; i < VecTraitsT::kSize; ++i) {
    d += squared(u[i] - v[i]);
  }
  return d;
}



/// O(N^2) verification. Verifies that the distance between each possible
/// sample pair meets the Poisson requirement, i.e. is greater than some radius.
template < typename VecTraitsT, typename VecT, typename FloatT>
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
          SquaredDistance<VecTraitsT>(*u, *v) < r_squared) {
        return false;
      }
    }
  }
  return true;
}


/// Returns true if all points in @a samples are within the bounds specified
/// by @a x_min and @a x_max.
template <typename VecTraitsT, typename VecT, typename FloatT, std::size_t N>
bool VerifyBounds(
  const std::vector<VecT>& samples, 
  const std::array<FloatT, N>& x_min, 
  const std::array<FloatT, N>& x_max)
{
  constexpr auto kDims = std::tuple_size<std::array<FloatT, N>>::value;
  static_assert(kDims == VecTraitsT::kSize, "todo");

  for (auto v = std::begin(samples); v != std::end(samples); ++v) {
    for (auto i = std::size_t{ 0 }; i < kDims; ++i) {
      const auto xi = VecTraitsT::Get(*v, i);
      if (x_min[i] > xi || xi > x_max[i]) {
        return false;
      }
    }
  }
  return true;
}


/// Convenience method for actually running the function under test.
template <typename VecT, typename VecTraitsT, typename InputT>
void SampleAndVerify(const std::vector<InputT>& inputs)
{
  for (auto input = std::begin(inputs); input != std::end(inputs); ++input) {
    const auto samples = pds::PoissonDiskSampling<VecT>(
      input->radius, 
      input->x_min, 
      input->x_max,
      input->max_sample_attempts, 
      input->seed);

    REQUIRE(VerifyPoisson<VecTraitsT>(samples, input->radius));
    REQUIRE(VerifyBounds<VecTraitsT>(samples, input->x_min, input->x_max));
  }
}


/// asdf
template <typename VecT, typename VecTraitsT>
void TestPoissonDiskSampling()
{
  typedef typename VecTraitsT::ValueType ValueType;
  constexpr auto kDims = VecTraitsT::kSize;

  constexpr auto kRadius = ValueType{ 2 };
  constexpr auto kSeed = std::uint32_t{ 0 };
  constexpr auto kMaxSampleAttempts = std::uint32_t{ 30 };

  auto inputs = std::vector<Input<ValueType, kDims>>{};
  auto x_min = std::array<ValueType, kDims>{};
  auto x_max = std::array<ValueType, kDims>{};
  std::fill(std::begin(x_min), std::end(x_min), -10.f);
  std::fill(std::begin(x_max), std::end(x_max), 10.f);
  inputs.push_back(Input<ValueType, kDims>(
    kRadius, 
    x_min, 
    x_max, 
    kMaxSampleAttempts, 
    kSeed));
  SampleAndVerify<VecT, VecTraitsT>(inputs);
}

} // namespace


TEST_CASE("Poisson Disk Sampling <Vec>", "[Container]")
{
  /*
  SECTION("N = 2")
  {
    TestPoissonDiskSampling<Vec<float, 2>>();
  }
  SECTION("N = 3")
  {
    TestPoissonDiskSampling<Vec<float, 3>>();
  }
  */
}


TEST_CASE("Poisson Disk Sampling <std::array>", "[Container]")
{
  SECTION("N = 2")
  {
    typedef std::array<float, 2> Vec2Type;
    typedef pds::VecTraits<Vec2Type> Vec2TraitsType;
    TestPoissonDiskSampling<Vec2Type, Vec2TraitsType>();
  }
  SECTION("N = 3")
  {
    typedef std::array<float, 3> Vec3Type;
    typedef pds::VecTraits<Vec3Type> Vec3TraitsType;
    TestPoissonDiskSampling<Vec3Type, Vec3TraitsType>();
  }
}

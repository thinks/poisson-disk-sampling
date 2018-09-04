// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <vec_example.h>

#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>


namespace {

struct Vec3
{
  float x;
  float y;
  float z;
};


struct MyVecTraits
{
  typedef float ValueType;

  static constexpr auto kSize = 3;

  static 
    ValueType Get(const Vec3& v, const std::size_t i)
  {
    return *(&v.x + i);
  }

  static 
    void Set(Vec3* const v, const std::size_t i, const ValueType val)
  {
    *(&v->x + i) = val;
  }
};


void ExplicitTraits(const std::string& filename_base)
{
  namespace pds = thinks::poisson_disk_sampling;

  constexpr auto radius = 2.f;
  const auto x_min = std::array<float, 3>{{ -10.f, -10.f, -10.f }};
  const auto x_max = std::array<float, 3>{{ 10.f, 10.f, 10.f }};
  const auto samples = pds::PoissonDiskSampling<float, 3, Vec3, MyVecTraits>(
    radius, x_min, x_max);

  auto oss = std::ostringstream(filename_base);
  oss << "_explicit_traits.txt";
  auto ofs = std::ofstream(oss.str());
  assert(ofs);
  for (const auto& sample : samples) {
    ofs << sample.x << ", " << sample.y << ", " << sample.z << std::endl;
  }
  ofs.close();
}


void TraitsInNamespace(const std::string& filename_base)
{
  namespace pds = thinks::poisson_disk_sampling;

  constexpr auto radius = 2.f;
  const auto x_min = std::array<float, 3>{{ -10.f, -10.f, -10.f }};
  const auto x_max = std::array<float, 3>{{ 10.f, 10.f, 10.f }};
  const auto samples = pds::PoissonDiskSampling<float, 3, Vec3>(
    radius, x_min, x_max);

  auto oss = std::ostringstream(filename_base);
  oss << "_traits_in_namespace.txt";
  auto ofs = std::ofstream(oss.str());
  assert(ofs);
  for (const auto& sample : samples) {
    ofs << sample.x << ", " << sample.y << ", " << sample.z << std::endl;
  }
  ofs.close();
}

} // namespace


namespace thinks {
namespace poisson_disk_sampling {

template<>
struct VecTraits<Vec3>
{
  typedef float ValueType;

  static constexpr auto kSize = 3;

  static 
  ValueType Get(const Vec3& v, const std::size_t i)
  {
    return *(&v.x + i);
  }

  static 
  void Set(Vec3* const v, const std::size_t i, const ValueType val)
  {
    *(&v->x + i) = val;
  }
};

} // namespace poisson_disk_sampling
} // namespace thinks


namespace examples {

void VecExample(const std::string& filename_base)
{
  ExplicitTraits(filename_base);
  TraitsInNamespace(filename_base);
}

} // namespace examples

// Copyright(C) 2018 Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <json_example.h>

#include <nlohmann/json.hpp>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>


namespace {

template <typename T, std::size_t N>
class Vec
{
public:
  typedef T value_type;
  static const std::size_t size = N;
  Vec() {}
  T& operator[](std::size_t i) { return _data[i]; }
  const T& operator[](std::size_t i) const { return _data[i]; }
private:
  T _data[N];
};

} // namespace


namespace examples {

void JsonExample(const std::string& filename)
{
  namespace pds = thinks::poisson_disk_sampling;

  typedef Vec<float, 2> Vec2f;

  float radius = 2.f;
  Vec2f x_min;
  x_min[0] = -10.f;
  x_min[1] = -10.f;
  Vec2f x_max;
  x_max[0] = 10.f;
  x_max[1] = 10.f;
  uint32_t max_sample_attempts = 30;
  uint32_t seed = 1981;

  //auto samples = pds::PoissonDiskSampling(radius, x_min, x_max, max_sample_attempts, seed);
}

} // namespace examples

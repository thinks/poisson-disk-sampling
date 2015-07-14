#ifndef THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED
#define THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED

#include <cmath>
#include <cstdint>
#include <vector>

#include "../../../include/thinks/poissonDiskSampling.hpp"

namespace thinks {
namespace test {
namespace detail {

//! Minimum implementation of the vector interface required by the poisson
//! disk sampling method.
template <typename T, std::size_t N>
class Vec
{
public:
  typedef T value_type;
  static const std::size_t size = N;
  Vec() {}
  Vec(T x) { std::fill_n(_data, N, x); }
  T& operator[](const std::size_t i) { return _data[i]; }
  const T& operator[](const std::size_t i) const { return _data[i]; }
private:
  T _data[N];
};

typedef Vec<float, 2> Vec2f;

//! Helper struct for keeping track of inputs to the algorithm.
template <std::size_t N>
struct Input
{
  Input(const float radius, const Vec<float, N>& x_min,
        const Vec<float, N>& x_max, const std::uint32_t max_sample_attempts,
        const std::uint32_t seed)
    : radius(radius), x_min(x_min), x_max(x_max)
    , max_sample_attempts(max_sample_attempts), seed(seed) {}

  float radius;
  Vec<float, N> x_min;
  Vec<float, N> x_max;
  std::uint32_t max_sample_attempts;
  std::uint32_t seed;
};

//! Returns the sqaured distance between the vectors @a u and @a v.
template <typename V> inline
typename V::value_type squaredDistance(const V& u, const V& v)
{
  using namespace std;
  auto d = (u[0] - v[0]) * (u[0] - v[0]);
  for (size_t i = 1; i < V::size; ++i) {
    d += ((u[i] - v[i]) * (u[i] - v[i]));
  }
  return d;
}

//! O(N^2) verification. Verifies that the distance between each possible
//! sample pair meets the Poisson requirement, i.e. is less than some radius.
template <typename V> inline
bool verifyPoisson(const std::vector<V>& samples,
                   const typename V::value_type radius)
{
  using namespace std;
  for (auto u = begin(samples); u != end(samples); ++u) {
    for (auto v = begin(samples); v != end(samples); ++v) {
      if (&(*u) != &(*v) && squaredDistance(*u, *v) < (radius * radius)) {
        return false;
      }
    }
  }
  return true;
}

//! Returns true if all points in @a samples are within the bounds specified
//! by @a x_min and @a x_max.
template <typename V> inline
bool verifyBounds(const std::vector<V>& samples, const V& x_min, const V& x_max)
{
  using namespace std;
  for (auto v = begin(samples); v != end(samples); ++v) {
    for (size_t i = 0; i < V::size; ++i) {
      if (x_min[i] > v[i] || v[i] > x_max[i]) {
        return false;
      }
    }
  }
  return true;
}

} // namespace detail

template <std::size_t N>
inline bool testPoissonDiskSampling()
{
  using namespace std;

  vector<detail::Input<N>> inputs;
  inputs.push_back(detail::Input(2.f, detail::Vec<float, N>(-10.f),
                                 detail::Vec<float, N>(10.f), 30, 0));
  inputs.push_back(detail::Input(5.f, detail::Vec2f(-10.f),
                                 detail::Vec2f(10.f), 30, 1981));
  inputs.push_back(detail::Input(3.f, detail::Vec2f(-5.f),
                                 detail::Vec2f(20.f), 30, 2015));

  for (auto input = begin(inputs); input != end(inputs); ++input) {
    auto samples = poissonDiskSampling(input->radius, input->x_min,
                                       input->x_max, input->max_sample_attempts,
                                       input->seed);
    if (!verifyPoisson(samples, input->radius)) {
      return false;
    }

    if (!verifyBounds(samples, input->x_min, input->x_max)) {
      return false;
    }
  }

  return true;
}

} // namespace test
} // namespace thinks

#endif // THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED

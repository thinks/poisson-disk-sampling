#ifndef THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED
#define THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED

#include <cmath>
#include <cstdint>
#include <vector>

#include "poissonDiskSampling.hpp"

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

struct Input
{
  Input(const float radius, const Vec2f& x_min, const Vec2f& x_max,
        const std::uint32_t max_sample_attempts, const std::uint32_t seed)
    : radius(radius), x_min(x_min), x_max(x_max)
    , max_sample_attempts(max_sample_attempts), seed(seed) {}

  float radius;
  Vec2f x_min;
  Vec2f x_max;
  std::uint32_t max_sample_attempts;
  std::uint32_t seed;
};

//!
template <typename T, std::size_t N> inline
typename Vec<T, N>::value_type
squaredDistance(const Vec<T, N>& u, const Vec<T, N>& v)
{
  using namespace std;
  auto d = (u[0] - v[0]) * (u[0] - v[0]);
  for (size_t i = 1; i < Vec<T, N>::size; ++i) {
    d += ((u[i] - v[i]) * (u[i] - v[i]));
  }
  return d;
}

//! O(N^2) verification. Verifies that the distance between each possible
//! sample pair meets the Poisson requirement, i.e. is less than some radius.
template <typename V>
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

} // namespace detail

inline bool testPoissonDiskSampling()
{
  using namespace std;

  vector<detail::Input> inputs;
  inputs.push_back(detail::Input(2.f, detail::Vec2f(-10.f),
                                 detail::Vec2f(10.f), 30, 0));
  /*inputs.push_back(detail::Input(2.f, detail::Vec2f(-10.f),
                                 detail::Vec2f(10.f), 30, 1981));
  inputs.push_back(detail::Input(3.f, detail::Vec2f(-5.f),
                                 detail::Vec2f(20.f), 30, 2015));*/

  for (auto input = begin(inputs); input != end(inputs); ++input) {
    cerr << "Input: " << &*input << endl;
    auto samples = poissonDiskSampling(input->radius, input->x_min,
                                       input->x_max, input->max_sample_attempts,
                                       input->seed);
    for (auto iter = begin(samples); iter != end(samples); ++iter) {
      cerr << (*iter)[0] << ", " << (*iter)[1] << endl;
    }
    std::cerr << "Verifying..." << std::endl;
    if (!verifyPoisson(samples, input->radius)) {
      return false;
    }
  }

  return true;
}

} // namespace test
} // namespace thinks

#endif // THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED

#ifndef THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED
#define THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include <thinks/poissonDiskSampling.hpp>

namespace thinks {
namespace test {
namespace detail {

//! Generic, looks for static constant size in type A.
template<typename A>
struct array_size
{
  static const size_t size = A::size;
};

//! Specialized for std::array, which does not provide compile time
//! access to array size (at least not without full support for constexpr).
template<typename T, std::size_t N>
struct array_size<std::array<T, N>>
{
  static const size_t size = N;
};

//! Minimum implementation of the vector interface required by the poisson
//! disk sampling method.
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

//! Helper struct for keeping track of inputs to the algorithm.
template <typename V>
struct Input
{
  Input(const float radius, const V& x_min, const V& x_max,
        const std::uint32_t max_sample_attempts, const std::uint32_t seed)
    : radius(radius), x_min(x_min), x_max(x_max)
    , max_sample_attempts(max_sample_attempts), seed(seed) {}

  float radius;
  V x_min;
  V x_max;
  std::uint32_t max_sample_attempts;
  std::uint32_t seed;
};

//! Returns the sqaured distance between the vectors @a u and @a v.
template <typename V> inline
typename V::value_type squaredDistance(const V& u, const V& v)
{
  using namespace std;
  auto d = (u[0] - v[0]) * (u[0] - v[0]);
  for (size_t i = 1; i < array_size<V>::size; ++i) {
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
    for (size_t i = 0; i < array_size<V>::size; ++i) {
      if (x_min[i] > (*v)[i] || (*v)[i] > x_max[i]) {
        return false;
      }
    }
  }
  return true;
}

//! Convenience method for actually running the function under test.
template <typename V>
bool testSamplings(const std::vector<Input<V>>& inputs)
{
  for (auto input = begin(inputs); input != end(inputs); ++input) {
    auto samples =
      poissonDiskSampling(input->radius, input->x_min, input->x_max,
                          input->max_sample_attempts, input->seed);
    if (!verifyPoisson(samples, input->radius)) {
      return false;
    }

    if (!verifyBounds(samples, input->x_min, input->x_max)) {
      return false;
    }
  }
  return true;
}

} // namespace detail

//! Returns true if tests are OK, otherwise false. Performs tests both
//! with example vector class above and with built-in std::array.
template <std::size_t N>
inline bool testPoissonDiskSampling()
{
  using namespace std;

  bool vec_success = false;
  {
    typedef detail::Vec<float, N> Vec;
    vector<detail::Input<Vec>> vec_inputs;
    Vec x_min;
    fill_n(&x_min[0], N, -10.f);
    Vec x_max;
    fill_n(&x_max[0], N, 10.f);
    vec_inputs.push_back(detail::Input<Vec>(2.f, x_min, x_max, 30, 0));
    vec_success = detail::testSamplings(vec_inputs);
  }

  bool array_success = false;
  {
    typedef array<float, N> Array;
    vector<detail::Input<Array>> array_inputs;
    Array x_min;
    fill_n(begin(x_min), N, -10.f);
    Array x_max;
    fill_n(begin(x_max), N, 10.f);
    array_inputs.push_back(detail::Input<Array>(2.f, x_min, x_max, 30, 0));
    array_success = detail::testSamplings(array_inputs);
  }

  return vec_success && array_success;
}

} // namespace test
} // namespace thinks

#endif // THINKS_TESTPOISSONDISKSAMPLING_HPP_INCLUDED

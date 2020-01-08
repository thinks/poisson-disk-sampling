[![Build Status](https://travis-ci.org/thinks/poisson-disk-sampling.svg?branch=master)](https://travis-ci.org/thinks/poisson-disk-sampling) 
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# Poisson Disk Sampling

[ ] - Update CMake, add cppbestpractices build flags.
[ ] - Build with MSVC, clang, gcc on travis.
[ ] - AppVeyor
[ ] - CodeCov or Coveralls
[ ] - Coverity scan

This repository contains a [single file](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poisson_disk_sampling/poisson_disk_sampling.h), header-only, no-dependencies, C++ library for generating Poisson disk samplings in an arbitrary number of dimensions. The implementation uses the techniques reported in the paper [Fast Poisson Disk Sampling in Arbitrary Dimensions](http://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf) published by [Robert Bridson](http://www.cs.ubc.ca/~rbridson/) in 2007. In fact, the implementation in this library is based on the public domain [example code](http://www.cs.ubc.ca/~rbridson/download/curlnoise.tar.gz) provided by the author.  

All code in this repository is released under the [MIT license](https://en.wikipedia.org/wiki/MIT_License).


## Contributions

This repository contributes the following improvements compared to the public domain code released by the original author:
* The code is in a [single header file](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poisson_disk_sampling/poisson_disk_sampling.h) and has no dependencies other than the standard library.
* The code is flexible in that results can be retrieved as a user-defined vector type (see examples below).
* The code is tested (see [test folder](https://github.com/thinks/poisson-disk-sampling/blob/master/test/) and section below).
* The code is in a namespace (```thinks::poisson_disk_sampling```)

## Cloning

This repository uses `git submodules`, which means that it needs to be cloned with the `--recursive` flag in order to initialize the submodules.

```
git clone --recursive https://github.com/thinks/poisson-disk-sampling.git
```

Note that the submodules are only used for building tests and examples.

## Usage

Poisson disk sampling aims to generate a set of samples within a bounded region such that no two samples are closer than some user-specified radius. Let us first show a simple example.
```C++
#include <array>
#include <vector>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>

std::vector<std::array<float, 2>> Foo()
{
  namespace pds = thinks::poisson_disk_sampling;

  // Input parameters.
  constexpr auto radius = 3.f;
  const auto x_min = std::array<float, 2>{{ -10.f, -10.f }};
  const auto x_max = std::array<float, 2>{{ 10.f, 10.f }};

  // Samples returned as std::vector<std::array<float, 2>>.
  // Default seed and max attempts.
  const auto samples = pds::PoissonDiskSampling(radius, x_min, x_max);
  return samples;
}
```
The code snippet above generates a set of points in the 2D range [-10, 10] separated by a distance (`radius`) of 3 units. The image below visualizes the results (generated using a simple [python script](https://github.com/thinks/poisson-disk-sampling/blob/master/examples/python/poisson_plot.py)). On the right-hand side the radius has been plotted to illustrate the distance separating the points. Here it is "clear" that each circle contains only a single point.

![Simple example](https://github.com/thinks/poisson-disk-sampling/blob/master/examples/images/simple_example.png "Simple example")

There are two additional parameters of the `PoissonDiskSampling` function: `seed` and `max_sample_attempts`. The `seed` parameter is used to generate pseudo-random numbers in a deterministic way. Changing the seed gives slightly different patterns. The `max_sample_attempts` controls the number of attempts that are made at finding neighboring points for each sample. Increasing this number could lead to a more tightly packed sampling in some cases, at the cost of computation time. Both `seed` and `max_sample_attempts` have reasonable default values so they need not always be specified. The images below illustrate the effect of varying `seed` and `max_sample_attempts`. 

![Seed and attempts](https://github.com/thinks/poisson-disk-sampling/blob/master/examples/images/seed_and_attempts.png "Seed and attempts")

By default the samples are returned as a `std::vector<std::array<F, N>>`, where the inner type `std::array<F, N>` has the same type as that used to specify the region bounds (see example above). In some cases it is useful to have the samples returned as a different type. There are two ways of doing this. First, we can explicitly provide our vector type together with a traits type, as in function `Foo` in the snippet below. The second way of doing it is to specialize the `thinks::poisson_disk_sampling::VecTraits` template for our vector type, as in function `Bar` below.
```C++
struct Vec3
{
  float x;
  float y;
  float z;
};

struct Vec3Traits
{
  typedef float ValueType;

  static constexpr auto kSize = 3;

  static ValueType Get(const Vec3& v, const std::size_t i)
  {
    return *(&v.x + i);
  }

  static void Set(Vec3* const v, const std::size_t i, const ValueType val)
  {
    *(&v->x + i) = val;
  }
};

namespace thinks {
namespace poisson_disk_sampling {

template<>
struct VecTraits<Vec3>
{
  typedef float ValueType;

  static constexpr auto kSize = 3;

  static ValueType Get(const Vec3& v, const std::size_t i)
  {
    return *(&v.x + i);
  }

  static void Set(Vec3* const v, const std::size_t i, const ValueType val)
  {
    *(&v->x + i) = val;
  }
};

} // namespace poisson_disk_sampling
} // namespace thinks

std::vector<Vec3> Foo()
{
  namespace pds = thinks::poisson_disk_sampling;

  constexpr auto radius = 3.f;
  const auto x_min = std::array<float, 3>{{ -10.f, -10.f, -10.f }};
  const auto x_max = std::array<float, 3>{{ 10.f, 10.f, 10.f }};
  
  // Explicitly passing in our own traits class.
  const auto samples = 
    pds::PoissonDiskSampling<float, 3, Vec3, Vec3Traits>(
      radius, x_min, x_max);
  return samples;
}

std::vector<Vec3> Bar()
{
  namespace pds = thinks::poisson_disk_sampling;

  constexpr auto radius = 3.f;
  const auto x_min = std::array<float, 3>{{ -10.f, -10.f, -10.f }};
  const auto x_max = std::array<float, 3>{{ 10.f, 10.f, 10.f }};

  // No need to explicitly specify traits here.
  const auto samples = pds::PoissonDiskSampling<float, 3, Vec3>(
    radius, x_min, x_max);
  return samples;
}
```

## Tests

The tests for this distribution are written in the [Catch2](https://github.com/catchorg/Catch2) framework. The Catch2 framework is included as a [submodule](https://github.com/thinks/poisson-disk-sampling/blob/master/test/external/) in this repository. 

Running the tests using [CTest](https://cmake.org/home/) is simple. In a terminal do the following (and similar for `Debug`):
```bash
$ cd d:
$ git clone git@github.com:/thinks/poisson-disk-sampling.git D:/pds
$ mkdir build-pds
$ cd build-pds
$ cmake ../pds
$ cmake --build . --config Release
$ ctest . -C Release
```
For more detailed test output locate the test executable (_thinks_poisson_disk_sampling_test.exe_) in the build tree and run it directly.

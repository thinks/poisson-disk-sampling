[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Standard](https://img.shields.io/badge/c%2B%2B-11/14/17/20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization)
![CI](https://github.com/thinks/poisson-disk-sampling/workflows/CI/badge.svg)
![Version](https://img.shields.io/badge/version-0.3.0-blue)

# Poisson Disk Sampling
This repository contains a [single file](https://github.com/thinks/poisson-disk-sampling/blob/master/thinks/poisson_disk_sampling/poisson_disk_sampling.h), header-only, no-dependencies, C++ library for generating Poisson disk samplings in an arbitrary number of dimensions. The implementation uses the techniques reported in the paper [Fast Poisson Disk Sampling in Arbitrary Dimensions](http://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf) published by [Robert Bridson](http://www.cs.ubc.ca/~rbridson/) in 2007. In fact, the implementation in this library is based on the public domain [example code](http://www.cs.ubc.ca/~rbridson/download/curlnoise.tar.gz) provided by the author.  

All code in this repository is released under the [MIT license](https://en.wikipedia.org/wiki/MIT_License).


## Contributions
This repository contributes the following improvements compared to the public domain code released by the original author:
* The code is in a [single header file](https://github.com/thinks/poisson-disk-sampling/blob/master/thinks/poisson_disk_sampling/poisson_disk_sampling.h) and has no dependencies other than the standard library.
* The code is flexible in that results can be retrieved as a user-defined vector type (see examples below).
* The code is tested (see [test folder](https://github.com/thinks/poisson-disk-sampling/tree/master/thinks/poisson_disk_sampling/test) and [test section](#tests) below).
* The code is bundled with a modern CMake build system that enables easy inclusion in existing projects.

## Cloning
In order to use the [single header file](https://github.com/thinks/poisson-disk-sampling/blob/master/thinks/poisson_disk_sampling/poisson_disk_sampling.h) in your project a simple clone is sufficient.
```
git clone https://github.com/thinks/poisson-disk-sampling.git
```
However, in order to build the tests and examples you need to initialize the submodules of this repository by cloning it recursively.
```
git clone --recursive https://github.com/thinks/poisson-disk-sampling.git
```
The easiest way to access the code is to use the provided CMake target. Assuming you cloned this repository (preferably as a submodule) in your `external` folder, just add the following lines of code to your CMakeLists.txt:

```CMake
// CMakeLists.txt
add_subdirectory(external/poisson-disk-sampling)
add_library(my_lib my_lib.cpp)
target_link_libraries(my_lib PUBLIC thinks::poisson_disk_sampling)
```

Giving you easy access to the functionality in your code.

```C++17
// my_lib.cpp
#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h

// Use the thinks::PoissonDiskSampling(...) function in your code.
```

## Usage
Poisson disk sampling aims to generate a set of samples within a bounded region such that no two samples are closer than some user-specified radius. Let us first show a simple example.
```C++
// C++17

#include <array>
#include <vector>

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

auto Foo() -> std::vector<std::array<float, 2>> {
  // Input parameters.
  constexpr auto kRadius = 3.F;
  constexpr auto kXMin = std::array<float, 2>{{-10.F, -10.F}};
  constexpr auto kXMax = std::array<float, 2>{{10.F, 10.F}};

  // Samples returned as std::vector<std::array<float, 2>>.
  // Default seed and max sample attempts.
  return thinks::PoissonDiskSampling(kRadius, kXMin, kXMax);
}
```
The code snippet above generates a set of points in the 2D range [-10, 10] separated by a distance (`radius`) of 3 units. The image below visualizes the results (generated using a simple [python script](https://github.com/thinks/poisson-disk-sampling/blob/master/python/poisson_plot.py)). On the right-hand side the radius has been plotted to illustrate the distance separating the points. Here it is "clear" that each circle contains only a single point.

![Simple example](https://github.com/thinks/poisson-disk-sampling/blob/master/images/simple_example.png "Simple example")

There are two additional parameters of the `PoissonDiskSampling` function: `seed` and `max_sample_attempts`. The `seed` parameter is used to generate pseudo-random numbers in a deterministic way. Changing the seed gives slightly different patterns. The `max_sample_attempts` controls the number of attempts that are made at finding neighboring points for each sample. Increasing this number could lead to a more tightly packed sampling in some cases, at the cost of additional computation time. Both `seed` and `max_sample_attempts` have reasonable default values so they need not always be specified. The images below illustrate the effect of varying `seed` and `max_sample_attempts`. 

![Seed and attempts](https://github.com/thinks/poisson-disk-sampling/blob/master/images/seed_and_attempts.png "Seed and attempts")

By default the samples are returned as a `std::vector<std::array<F, N>>`, where the inner type `std::array<F, N>` has the same type as that used to specify the region bounds (see example above). In some cases it is useful to have the samples returned as a different type. There are two ways of doing this. First, we can explicitly provide our vector type together with a traits type, as in the function `Foo` in the snippet below. The second way of doing it is to specialize the `thinks::poisson_disk_sampling::VecTraits` template for our vector type, as in the function `Bar` below.
```C++
// C++17

#include <array>
#include <vector>

#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"

struct Vec3 {
  float v[3];
};

// Traits outside thinks namespace.
struct Vec3Traits {
  using ValueType = float;

  static constexpr auto kSize = 3;

  static constexpr auto Get(const Vec3& v, const std::size_t i) -> ValueType {
    return v.v[i];
  }

  static constexpr void Set(Vec3* const v, const std::size_t i, const ValueType val) {
    v->v[i] = val;
  }
};

namespace thinks {

// Traits in thinks namespace.
template<>
struct VecTraits<Vec3> {
  using ValueType = float;

  static constexpr auto kSize = 3;

  static constexpr auto Get(const Vec3& v, const std::size_t i) -> ValueType {
    return v.v[i];
  }

  static constexpr void Set(Vec3* const v, const std::size_t i, const ValueType val) {
    v->v[i] = val;
  }
};

} // namespace thinks

auto Foo() -> std::vector<Vec3> {
  constexpr auto kRadius = 3.F;
  constexpr auto kXMin = std::array<float, 3>{{ -10.F, -10.F, -10.F }};
  constexpr auto kXMax = std::array<float, 3>{{ 10.F, 10.F, 10.F }};
  
  // Explicitly passing in our own traits class.
  return thinks::PoissonDiskSampling<float, 3, Vec3, Vec3Traits>(
      kRadius, kXMin, kXMax);
}

auto Bar() -> std::vector<Vec3> {
  constexpr auto kRadius = 3.F;
  constexpr auto kXMin = std::array<float, 3>{{ -10.F, -10.F, -10.F }};
  constexpr auto kXMax = std::array<float, 3>{{ 10.F, 10.F, 10.F }};

  // No need to explicitly specify traits here since there exists
  // a suitable candidate for Vec3 in the thinks namespace.
  return thinks::PoissonDiskSampling<float, 3, Vec3>(
      kRadius, kXMin, kXMax);
}
```

## Periodogram
Poisson disk sampling generates samples from a blue noise distribution. We can verify this by plotting the corresponding periodogram, noticing that there are minimal low frequency components and no concentrated spikes in energy.

The image below was generated using the code in [one of the provided examples](https://github.com/thinks/poisson-disk-sampling/blob/master/thinks/poisson_disk_sampling/examples/periodogram_example.cc) and is an average over 100 sampling patterns (original pixel resolution 2048x2048).

![Average periodogram](https://github.com/thinks/poisson-disk-sampling/blob/master/images/avg_periodogram_512.png "Average periodogram")

## Tests

The tests for this distribution are written in the [Catch2](https://github.com/catchorg/Catch2) framework, which is included as a [submodule](https://github.com/thinks/poisson-disk-sampling/blob/master/external/) in this repository. 

Running the tests using [CTest](https://cmake.org/home/) is simple. In a terminal do the following (and similar for `Debug`):
```bash
$ git clone --recursive https://github.com/thinks/poisson-disk-sampling.git
$ cd poisson-disk-sampling
$ cmake -E remove_directory build
$ cmake -B build -S . -G Ninja -DTHINKS_RUN_TESTS=ON -DCMAKE_BUILD_TYPE=Release
$ cmake --build build
$ cd build
$ ctest -j4 --output-on-failure --verbose
```

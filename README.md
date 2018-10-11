# Poisson Disk Sampling

This repository contains a [single file](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poisson_disk_sampling/poisson_disk_sampling.h), header-only, no dependencies, C++ library for generating Poisson disk samplings in an arbitrary number of dimensions. The implementation uses the techniques reported in the paper [Fast Poisson Disk Sampling in Arbitrary Dimensions](http://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf) published by [Robert Bridson](http://www.cs.ubc.ca/~rbridson/) in 2007. In fact, the implementation in this library is based on the public domain [example code](http://www.cs.ubc.ca/~rbridson/download/curlnoise.tar.gz) provided by the author.  

All code in this repository is released under the [MIT license](https://en.wikipedia.org/wiki/MIT_License).

## Contributions

This repository contributes the following improvements compared to the public domain code released by the original author:
* The code is in a [single file](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poisson_disk_sampling/poisson_disk_sampling.h) and has no dependencies other than the standard library.
* The code is flexible in that results can be retrieved as a user-defined vector type (see examples below).
* The code is tested (see [test folder](https://github.com/thinks/poisson-disk-sampling/blob/master/test/) and section below).
* The code is in a namespace (```thinks::poisson_disk_sampling```)

## Usage

Poisson disk sampling aims to generate a set of samples within a bounded region such that no two samples are closer than some user-specified radius. Let us first show a simple example.
```C++
#include <array>

#include <thinks/poisson_disk_sampling/poisson_disk_sampling.h>

namespace pds = thinks::poisson_disk_sampling;

// Input parameters.
constexpr auto radius = 3.f;
const auto x_min = std::array<float, 2>{{ -10.f, -10.f }};
const auto x_max = std::array<float, 2>{{ 10.f, 10.f }};

// Samples returned as std::vector<std::array<float, 2>>.
const auto samples = pds::PoissonDiskSampling(radius, x_min, x_max);
```
The code snippet above generates a set of points in the range [-10, 10] separated by a distance (`radius`) of 3 units. The image below visualizes the results. On the right-hand side the radius has been plotted to illustrate the distance separating the points.
![Simple example](https://github.com/thinks/poisson-disk-sampling/blob/master/examples/images/simple_example.png "Simple example")



Calling the Poisson disk sampling function is fairly straight-forward. Input is taken in the form of min/max coordinates in the relevant number of dimensions. The resulting sampling is returned as a set of points in that same dimensionality. The vector class used to specify the min/max bounds also determines the type of the returned points. Care has been taken to make sure that the required interface of this vector class is as minimal as possible. The following extremely simple vector class is sufficient:
```C++
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
```

For instance, generating a Poisson disk sampling in 2D in the region [(-10, -10), (10, 10)] could be done with the following lines of code:
```C++
#include <cstdint>
#include <vector>
#include <thinks/poissonDiskSampling.hpp>

using namespace std;
typedef Vec<float, 2> Vec2f;

// Setup input parameters.
float radius = 2.f;
Vec2f x_min;
x_min[0] = -10.f;
x_min[1] = -10.f;
Vec2f x_max;
x_max[0] = 10.f;
x_max[1] = 10.f;
uint32_t max_sample_attempts = 30;
uint32_t seed = 1981;

vector<Vec2f> samples = thinks::poissonDiskSampling(radius, x_min, x_max, max_sample_attempts, seed);
```
For simplicity, reasonable default values for ```max_sample_attempts``` and ```seed``` are set if not provided, making these last two parameters optional. Also, worth noting is that the built-in type ```std::array``` can be used as a vector type for those not interested in rolling their own (or don't have a suitable one lying around).
```C++
#include <array>
#include <thinks/poissonDiskSampling.hpp>

using namespace std;

// Setup input parameters.
float radius = 2.f;
array<float, 2> x_min = { -10.f, -10.f };
array<float, 2> x_max = { 10.f, 10.f };

vector<Vec2f> samples = thinks::poissonDiskSampling(radius, x_min, x_max);
```

## Tests

The tests for this distribution are written in the [Catch2](https://github.com/catchorg/Catch2) framework. A snapshot of the [single header](https://github.com/thinks/poisson-disk-sampling/blob/master/test/catch2/catch.hpp) version of Catch2 is included in this repository. 

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

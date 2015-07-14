# Poisson Disk Sampling

This repository contains a [single header](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poissonDiskSampling.hpp) library for generating Poisson disk samplings in for an arbitrary number of dimensions. The implementation uses the techniques reported in the paper [Fast Poisson Disk Sampling in Arbitrary Dimensions](http://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf) published by [Robert Bridson](http://www.cs.ubc.ca/~rbridson/) in 2007. In fact, the implementation in this library is itself heavily based on the [public domain example code](http://www.cs.ubc.ca/~rbridson/download/curlnoise.tar.gz) provided by the author.  

## Contributions

This repository contributes the following improvements compared to the public domain code released by the original author:
* The code is self-contained in a [single header](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poissonDiskSampling.hpp) file.
* The code is placed in a namespace (```thinks::```)
* The code relies almost only on built-in types and enables the user to easily inject the necessary types (see examples below).
* A framework agnostic set of [tests](https://github.com/thinks/poisson-disk-sampling/blob/master/test/include/thinks/testPoissonDiskSampling.hpp) have been added so that stability can be measured in the future.

## Example Usage

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
Vec2f x_min(-10.f);
Vec2f x_max(10.f);
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

## Running Tests

Running the tests is even easier than using the actual function. Simply call the test method specifying the number of dimensions as a template parameter:
```C++
#include <thinks/testPoissonDiskSampling.hpp>

bool success1 = thinks::testPoissonDiskSampling<1>();
bool success2 = thinks::testPoissonDiskSampling<2>();
bool success3 = thinks::testPoissonDiskSampling<3>();
bool success4 = thinks::testPoissonDiskSampling<4>();
```

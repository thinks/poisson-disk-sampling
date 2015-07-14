# Poisson Disk Sampling

This repository contains a single [header](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poissonDiskSampling.hpp) library for generating Poisson disk samplings in any dimension. The implementation uses the techniques reported in the paper [Fast Poisson Disk Sampling in Arbitrary Dimensions](http://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf) published by [Robert Bridson](http://www.cs.ubc.ca/~rbridson/) in 2007. In fact, the implementation in this library is itself heavily based on the [public domain example code](http://www.cs.ubc.ca/~rbridson/download/curlnoise.tar.gz) provided by the author.  

## Contributions

This repository contributes the following improvements compared to the public domain code released byt the original author:
* The code is self-contained in a single [header](https://github.com/thinks/poisson-disk-sampling/blob/master/include/thinks/poissonDiskSampling.hpp) file.
* The code is placed in a namespace (```thinks::```)
* The code relies almost only on built-in types and enables the user to easily inject the necessary types (see examples below).
* A framework agnostic set of [tests](https://github.com/thinks/poisson-disk-sampling/blob/master/test/include/thinks/testPoissonDiskSampling.hpp) have been added so that stability can be measured in the future.

## Example Usage

Calling the Poisson disk sampling function is fairly straight-forward. Input is taken in the form of min/max coordinates in the relevant number of dimensions. The resulting sampling is returned as a set of points in that same dimensionality. The vector class used to specify the min/max bounds also determines 

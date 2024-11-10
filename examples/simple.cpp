#include <array>// std::array
#include <cassert>// assert
#include <cstdint>// UINT64_C, etc
#include <cstdio>// std::printf
#include <functional>// std::function
#include <memory>// std::unique_ptr
#include <type_traits>// std::is_same_v

#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

static_assert(std::is_same_v<tph_poisson_real, float>);

int main(int /*argc*/, char * /*argv*/[])
{
  // clang-format off
  constexpr std::array<tph_poisson_real, 2> bounds_min{ 
    static_cast<tph_poisson_real>(-10), static_cast<tph_poisson_real>(-10) };
  constexpr std::array<tph_poisson_real, 2> bounds_max{ 
    static_cast<tph_poisson_real>(10),  static_cast<tph_poisson_real>(10) };
  // clang-format on

  // Configure arguments.
  tph_poisson_args args = {};
  args.radius = static_cast<tph_poisson_real>(3);
  args.ndims = INT32_C(2);
  args.bounds_min = bounds_min.data();
  args.bounds_max = bounds_max.data();
  args.max_sample_attempts = UINT32_C(30);
  args.seed = UINT64_C(1981);

  // Using default allocator (libc malloc).
  const tph_poisson_allocator *alloc = NULL;

  // Initialize empty sampling.
  using unique_poisson_ptr =
    std::unique_ptr<tph_poisson_sampling, std::function<void(tph_poisson_sampling *)>>;
  auto sampling = unique_poisson_ptr{ new tph_poisson_sampling{}, [](tph_poisson_sampling *s) {
                                       tph_poisson_destroy(s);
                                       delete s;
                                     } };

  // Populate sampling with points.
  if (const int ret = tph_poisson_create(&args, alloc, sampling.get());
      ret != TPH_POISSON_SUCCESS) {
    std::printf("Failed creating Poisson sampling! Error code: %d\n", ret);
    return EXIT_FAILURE;
  };

  // Retrieve sampling points.
  const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
  if (samples == nullptr) {
    /* Shouldn't happen since we check the return value from tph_poisson_create! */
    std::printf("Bad samples!\n");
    return EXIT_FAILURE;
  }

  // Print first and last sample positions.
  // clang-format off
  assert(sampling->nsamples >= 2);
  std::printf("\nsimple (Cpp):\n");
  std::printf("sample[%td] = ( %.3f, %.3f )\n", 
    static_cast<ptrdiff_t>(0), 
    static_cast<double>(samples[0]), 
    static_cast<double>(samples[1]));
  std::printf("...\n");
  std::printf("sample[%td] = ( %.3f, %.3f )\n\n",
    static_cast<ptrdiff_t>(sampling->nsamples - 1),
    static_cast<double>(samples[(sampling->nsamples - 1) * sampling->ndims]),
    static_cast<double>(samples[(sampling->nsamples - 1) * sampling->ndims + 1]));
  // clang-format on

  // tph_poisson_destroy is called by unique_poisson_ptr destructor.

  return EXIT_SUCCESS;
}

#include <array>//std::array
#include <cstdio>// std::printf
#include <functional>// std::function
#include <memory>// std::unique_ptr

#define TPH_POISSON_IMPLEMENTATION
#include <thinks/tph_poisson.h>

int main(int /*argc*/, char * /*argv*/[])
{
  // clang-format off
  constexpr std::array<tph_poisson_real, 2> bounds_min{ 
    static_cast<tph_poisson_real>(-100),
    static_cast<tph_poisson_real>(-100) };
  constexpr std::array<tph_poisson_real, 2> bounds_max{ 
    static_cast<tph_poisson_real>(100),
    static_cast<tph_poisson_real>(100) };
  // clang-format on

  tph_poisson_args args = {};
  args.radius = 10.F;
  args.ndims = 2;
  args.bounds_min = bounds_min.data();
  args.bounds_max = bounds_max.data();
  args.max_sample_attempts = 30;
  args.seed = 1981;

  using unique_poisson_ptr =
    std::unique_ptr<tph_poisson_sampling, std::function<void(tph_poisson_sampling *)>>;
  auto sampling = unique_poisson_ptr{ new tph_poisson_sampling{}, [](tph_poisson_sampling *s) {
                                       tph_poisson_destroy(s);
                                       delete s;
                                     } };

  if (const int ret = tph_poisson_create(&args, /*alloc=*/nullptr, sampling.get());
      ret != TPH_POISSON_SUCCESS) {
    printf("Failed creating Poisson sampling! Error code: %d\n", ret);
    return EXIT_FAILURE;
  };

  const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
  if (samples == nullptr) {
    // This cannot really happen since we check the return code from tph_poisson_create.
    printf("Failed getting Poisson samples!\n");
    return EXIT_FAILURE;
  }

  // Print sample positions.
  // clang-format off
  printf("\nsimple_cpp:\n");
  printf("sample[%td] = ( %.3f, %.3f )\n", 
    (ptrdiff_t)0, 
    (double)samples[0], 
    (double)samples[1]);
  printf("...\n");
  printf("sample[%td] = ( %.3f, %.3f )\n\n",
    sampling->nsamples - 1,
    (double)samples[(sampling->nsamples - 1) * sampling->ndims],
    (double)samples[(sampling->nsamples - 1) * sampling->ndims + 1]);
  // clang-format on

  for (ptrdiff_t i = 0; i < sampling->nsamples; ++i) {
    std::printf("samples[%td] = ( %.3f, %.3f )\n",
      i,
      static_cast<double>(samples[i * sampling->ndims]),
      static_cast<double>(samples[i * sampling->ndims + 1]));
  }

  return EXIT_SUCCESS;
}

#include <array>//std::array
#include <cinttypes>// PRIu32, etc
#include <cstdio>// std::printf
#include <cstring>// std::memset
#include <memory>// std::unique_ptr

#define TPH_POISSON_IMPLEMENTATION
// #define TPH_POISSON_REAL_TYPE float /*double*/
#include <thinks/tph_poisson.h>

int main(int argc, char *argv[])
{
  constexpr std::array<tph_poisson_real, 2> bounds_min{ -100.F, -100.F };
  constexpr std::array<tph_poisson_real, 2> bounds_max{ 100.F, 100.F };

  tph_poisson_args args = {};
  args.radius = 10.F;
  args.ndims = 2;
  args.bounds_min = bounds_min.data();
  args.bounds_max = bounds_max.data();
  args.max_sample_attempts = 30;
  args.seed = 0;
  auto sampling = std::unique_ptr<tph_poisson_sampling, decltype(&tph_poisson_destroy)>{
    new tph_poisson_sampling{},
    tph_poisson_destroy
  };

  if (const int ret = tph_poisson_create(sampling.get(), &args, nullptr);
      ret != TPH_POISSON_SUCCESS) {
    std::printf("Create error...");
    return EXIT_FAILURE;
  };

  const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
  if (samples == nullptr) {
    tph_poisson_destroy(sampling.get());
    return EXIT_FAILURE;
  }


  for (ptrdiff_t i = 0; i < sampling->nsamples; ++i) {
    std::printf("p[%td] = ( %.3f, %.3f )\n",
      i,
      samples[i * sampling->ndims],
      samples[i * sampling->ndims + 1]);
  }

  return EXIT_SUCCESS;
}

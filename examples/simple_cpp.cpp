#include <array>//std::array
#include <cassert>
#include <cstdio>// std::printf
#include <functional>// std::function
#include <memory>// std::unique_ptr

#define TPH_POISSON_IMPLEMENTATION
#include <thinks/tph_poisson.h>

int main(int /*argc*/, char * /*argv*/[])
{
  constexpr std::array<tph_poisson_real, 2> bounds_min{ -100.F, -100.F };
  constexpr std::array<tph_poisson_real, 2> bounds_max{ 100.F, 100.F };

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
    printf("Failed creating Poisson sampling! Error code: %d", ret);
    return EXIT_FAILURE;
  };

  const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
  assert(samples);

  for (ptrdiff_t i = 0; i < sampling->nsamples; ++i) {
    std::printf("samples[%td] = ( %.3f, %.3f )\n",
      i,
      static_cast<double>(samples[i * sampling->ndims]),
      static_cast<double>(samples[i * sampling->ndims + 1]));
  }

  return EXIT_SUCCESS;
}

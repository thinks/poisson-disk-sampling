#include <inttypes.h>// PRIu32, etc
#include <stdio.h>// printf
#include <string.h>// memset

#define TPH_POISSON_IMPLEMENTATION
/* #define TPH_POISSON_REAL_TYPE float */ /*double*/
#include <thinks/tph_poisson.h>

int main(int argc, char *argv[])
{
  const tph_poisson_real bounds_min[] = { -100.F, -100.F };
  const tph_poisson_real bounds_max[] = { 100.F, 100.F };
  tph_poisson_args args = {};
  args.radius = 10.F;
  args.ndims = 2;
  args.bounds_min = bounds_min;
  args.bounds_max = bounds_max;
  args.max_sample_attempts = 30;
  args.seed = 0;

  tph_poisson_sampling sampling = {};
  int ret = tph_poisson_create(&sampling, &args, NULL);
  if (ret != TPH_POISSON_SUCCESS) {
    printf("Error!");
    return EXIT_FAILURE;
  }

  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  if (samples == NULL) {
    tph_poisson_destroy(&sampling);
    return EXIT_FAILURE;
  }

  for (ptrdiff_t i = 0; i < sampling.nsamples; ++i) {
    printf("p[%td] = ( %.3f, %.3f )\n",
      i,
      samples[i * sampling.ndims],
      samples[i * sampling.ndims + 1]);
  }

  tph_poisson_destroy(&sampling);

  return EXIT_SUCCESS;
}

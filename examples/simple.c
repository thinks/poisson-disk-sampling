#include <assert.h>
#include <stddef.h>/* ptrdiff_t */
#include <stdio.h>/* printf */
#include <stdlib.h>/* EXIT_FAILURE, etc */

#define TPH_POISSON_IMPLEMENTATION
#if 0
#define TPH_POISSON_REAL_TYPE double
#include <math.h>
#define TPH_POISSON_SQRT sqrt
#define TPH_POISSON_CEIL ceil
#define TPH_POISSON_FLOOR floor
#endif
#include <thinks/tph_poisson.h>

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-100, (tph_poisson_real)-100 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)100, (tph_poisson_real)100 };
  tph_poisson_args args = { NULL };
  args.radius = (tph_poisson_real)10;
  args.ndims = 2;
  args.bounds_min = bounds_min;
  args.bounds_max = bounds_max;
  args.max_sample_attempts = 30;
  args.seed = 1981;

  tph_poisson_sampling sampling = { NULL };
  tph_poisson_allocator *alloc = NULL;
  int ret = tph_poisson_create(&sampling, &args, alloc);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy sampling here! */
    printf("Failed creating Poisson sampling! Error code: %d", ret);
    return EXIT_FAILURE;
  }

  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  assert(samples);

  /* Print sample positions. */
  for (ptrdiff_t i = 0; i < sampling.nsamples; ++i) {
    printf("sample[%td] = ( %.3f, %.3f )\n",
      i,
      samples[i * sampling.ndims],
      samples[i * sampling.ndims + 1]);
  }

  /* Free memory. */
  tph_poisson_destroy(&sampling);

  return EXIT_SUCCESS;
}

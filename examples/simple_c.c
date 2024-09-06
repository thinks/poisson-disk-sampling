#include <stddef.h>/* ptrdiff_t */
#include <stdint.h>/* UINT64_C, etc */
#include <stdio.h>/* printf */
#include <stdlib.h>/* EXIT_FAILURE, etc */

#define TPH_POISSON_IMPLEMENTATION
/*
#define TPH_POISSON_REAL_TYPE double
#include <math.h>
#define TPH_POISSON_SQRT sqrt
#define TPH_POISSON_CEIL ceil
#define TPH_POISSON_FLOOR floor
*/
#include <thinks/tph_poisson.h>

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-100, (tph_poisson_real)-100 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)100, (tph_poisson_real)100 };

  const tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)10,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(30),
    .seed = UINT64_C(1981) };

  tph_poisson_sampling sampling = { .internal = NULL, .ndims = INT32_C(0), .nsamples = 0 };
  tph_poisson_allocator *alloc = NULL;

  const int ret = tph_poisson_create(&args, alloc, &sampling);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy sampling here! */
    printf("Failed creating Poisson sampling! Error code: %d\n", ret);
    return EXIT_FAILURE;
  }

  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  if (samples == NULL) {
    /* This cannot really happen since we check the return code from tph_poisson_create. */
    printf("Failed getting Poisson samples!\n");
    tph_poisson_destroy(&sampling);
    return EXIT_FAILURE;
  }

  /* Print sample positions. */
  /* clang-format off */
  printf("\nsimple_c:\n");
  printf("sample[%td] = ( %.3f, %.3f )\n", 
    (ptrdiff_t)0, 
    (double)samples[0], 
    (double)samples[1]);
  printf("...\n");
  printf("sample[%td] = ( %.3f, %.3f )\n\n",
    sampling.nsamples - 1,
    (double)samples[(sampling.nsamples - 1) * sampling.ndims],
    (double)samples[(sampling.nsamples - 1) * sampling.ndims + 1]);
  /* clang-format on */

  /* Free memory. */
  tph_poisson_destroy(&sampling);

  return EXIT_SUCCESS;
}

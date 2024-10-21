#include <assert.h>/* assert */
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
#include "thinks/tph_poisson.h"

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  /* Configure arguments. */
  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-10, (tph_poisson_real)-10 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)10, (tph_poisson_real)10 };
  const tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)3,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(30),
    .seed = UINT64_C(1981) };
  /* Using default allocator (libc malloc). */
  const tph_poisson_allocator *alloc = NULL;

  /* Create samples. */
  tph_poisson_sampling sampling = { .internal = NULL, .ndims = INT32_C(0), .nsamples = 0 };
  const int ret = tph_poisson_create(&args, alloc, &sampling);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy sampling here! */
    printf("Failed creating Poisson sampling! Error code: %d\n", ret);
    return EXIT_FAILURE;
  }

  /* Retrieve samples. */
  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  if (samples == NULL) {
    /* Cannot happen since we check the return value from tph_poisson_create! */
    printf("Bad samples!\n");
    tph_poisson_destroy(&sampling);
    return EXIT_FAILURE;
  }

  /* Print first and last sample positions. */
  /* clang-format off */
  printf("\nsimple_c:\n");
  printf("samples[%td] = ( %.3f, %.3f )\n", 
    (ptrdiff_t)0, 
    (double)samples[0], 
    (double)samples[1]);
  printf("...\n");
  printf("samples[%td] = ( %.3f, %.3f )\n\n",
    sampling.nsamples - 1,
    (double)samples[(sampling.nsamples - 1) * sampling.ndims],
    (double)samples[(sampling.nsamples - 1) * sampling.ndims + 1]);
  /* clang-format on */

  /* Free memory. */
  tph_poisson_destroy(&sampling);

  return EXIT_SUCCESS;
}

#include <assert.h> /* assert */
#include <stddef.h> /* ptrdiff_t */
#include <stdint.h> /* UINT64_C, etc */
#include <stdio.h> /* printf */
#include <stdlib.h> /* EXIT_FAILURE, etc */
#include <string.h> /* memset */

void *my_malloc(size_t size);
void my_free(void *ptr);

/* Provide custom functions for both malloc and free. Note that this
 * is quite different from using a custom allocator, since it is not
 * really possible to use an allocation context with this approach. */
#define TPH_POISSON_MALLOC my_malloc
#define TPH_POISSON_FREE my_free
#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

void *my_malloc(size_t size)
{
  static int call_count = 0;
  void *ptr = malloc(size);
  printf("%d - my_malloc(%zu) -> %p\n", call_count++, size, ptr);
  return ptr;
}

void my_free(void *ptr)
{
  static int call_count = 0;
  printf("%d - my_free(%p)\n", call_count++, ptr);
  free(ptr);
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  /* Configure arguments. */
  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-10, (tph_poisson_real)-10 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)10, (tph_poisson_real)10 };
  const tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)1.47,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(40),
    .seed = UINT64_C(2017) };

  /* Using default allocator (libc malloc). */
  const tph_poisson_allocator *alloc = NULL;

  /* Initialize empty sampling. */
  tph_poisson_sampling sampling;
  memset(&sampling, 0, sizeof(tph_poisson_sampling));

  /* Populate sampling with points. */
  const int ret = tph_poisson_create(&args, alloc, &sampling);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy sampling here! */
    printf("tph_poisson_create error, code: %d\n", ret);
    return EXIT_FAILURE;
  }

  assert(sampling.nsamples >= 2);

  /* Retrieve samples. */
  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  if (samples == NULL) {
    /* Shouldn't happen since we check the return value from tph_poisson_create! */
    printf("Bad samples!\n");
    tph_poisson_destroy(&sampling);
    return EXIT_FAILURE;
  }

  /* Print first and last sample positions. */
  /* clang-format off */
  printf("\ncustom_libc:\n");
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

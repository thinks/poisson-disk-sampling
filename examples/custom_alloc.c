#include <assert.h> /* assert */
#include <stddef.h> /* ptrdiff_t */
#include <stdint.h> /* UINT64_C, etc */
#include <stdio.h> /* printf */
#include <stdlib.h> /* EXIT_FAILURE, etc */
#include <string.h> /* memset */

#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

static_assert(sizeof(tph_poisson_real) == 4, "");

typedef struct my_alloc_ctx_
{
  ptrdiff_t total_malloc;
  ptrdiff_t total_free;
} my_alloc_ctx;

static void *my_alloc_malloc(ptrdiff_t size, void *ctx)
{
  my_alloc_ctx *a_ctx = (my_alloc_ctx *)ctx;
  if (size == 0) { return NULL; }
  void *ptr = malloc((size_t)(size));
  a_ctx->total_malloc += size;
  return ptr;
}

static void my_alloc_free(void *ptr, ptrdiff_t size, void *ctx)
{
  my_alloc_ctx *a_ctx = (my_alloc_ctx *)ctx;
  if (ptr == NULL) { return; }
  a_ctx->total_free += size;
  free(ptr);
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  /* clang-format off */
  const tph_poisson_real bounds_min[2] = { 
    (tph_poisson_real)-10, (tph_poisson_real)-10 };
  const tph_poisson_real bounds_max[2] = { 
    (tph_poisson_real)10, (tph_poisson_real)10 };
  /* clang-format on */

  /* Configure arguments. */
  const tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)3,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(30),
    .seed = UINT64_C(1981) };

  /* Using custom allocator. */
  my_alloc_ctx alloc_ctx = { .total_malloc = 0, .total_free = 0 };
  tph_poisson_allocator alloc = {
    .malloc = my_alloc_malloc, .free = my_alloc_free, .ctx = &alloc_ctx
  };

  /* Initialize empty sampling. */
  tph_poisson_sampling sampling;
  memset(&sampling, 0, sizeof(tph_poisson_sampling));

  /* Populate sampling with points. */
  const int ret = tph_poisson_create(&args, &alloc, &sampling);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy sampling here! */
    printf("Failed creating Poisson sampling! Error code: %d\n", ret);
    return EXIT_FAILURE;
  }

  /* Retrieve sampling points. */
  const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
  if (samples == NULL) {
    /* Shouldn't happen since we check the return value from tph_poisson_create! */
    printf("Bad samples!\n");
    tph_poisson_destroy(&sampling);
    return EXIT_FAILURE;
  }

  /* Print first and last sample positions. */
  /* clang-format off */
  assert(sampling.nsamples >= 2);
  printf("\n%s:\n"
         "samples[%td] = ( %.3f, %.3f )\n"
         "...\n"
         "samples[%td] = ( %.3f, %.3f )\n\n", 
    "custom_alloc",
    (ptrdiff_t)0, 
    (double)samples[0], 
    (double)samples[1],
    (ptrdiff_t)(sampling.nsamples - 1),
    (double)samples[(sampling.nsamples - 1) * sampling.ndims],
    (double)samples[(sampling.nsamples - 1) * sampling.ndims + 1]);
  /* clang-format on */

  /* Free memory. */
  tph_poisson_destroy(&sampling);

  printf(
    "total malloc = %td\n"
    "total free = %td\n",
    alloc_ctx.total_malloc,
    alloc_ctx.total_free);

  return EXIT_SUCCESS;
}

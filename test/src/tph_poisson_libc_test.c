#include <stdint.h> /* UINT64_C, etc */
#include <stdio.h> /* printf */
#include <stdlib.h> /* malloc, free, EXIT_SUCCESS */
#include <string.h> /* memset, memcpy */

static void *my_malloc(size_t size);
static void my_free(void *ptr);
static void *my_memcpy(void *dest, const void *src, size_t count);
static void *my_memset(void *dest, int ch, size_t count);

/* Provide custom functions for both malloc, free, memcpy, and memset. */
#define TPH_POISSON_MALLOC my_malloc
#define TPH_POISSON_FREE my_free
#define TPH_POISSON_MEMCPY my_memcpy
#define TPH_POISSON_MEMSET my_memset
#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

#include "require.h"

/* Global variables to count the number of calls to our custom libc functions. */
static int num_malloc = 0;
static int num_free = 0;
static int num_memcpy = 0;
static int num_memset = 0;

static void *my_malloc(size_t size)
{
  ++num_malloc;
  return malloc(size);
}

static void my_free(void *ptr)
{
  ++num_free;
  free(ptr);
}

static void *my_memcpy(void *dest, const void *src, size_t count)
{
  ++num_memcpy;
  return memcpy(dest, src, count);
}

static void *my_memset(void *dest, int ch, size_t count)
{
  ++num_memset;
  return memset(dest, ch, count);
}

typedef struct my_tph_alloc_ctx_
{
  int num_malloc;
  int num_free;
} my_tph_alloc_ctx;

static void *my_tph_malloc(ptrdiff_t size, void *ctx)
{
  my_tph_alloc_ctx *a_ctx = (my_tph_alloc_ctx *)ctx;
  ++a_ctx->num_malloc;
  return malloc((size_t)size);
}

static void my_tph_free(void *ptr, ptrdiff_t size, void *ctx)
{
  (void)size;
  my_tph_alloc_ctx *a_ctx = (my_tph_alloc_ctx *)ctx;
  ++a_ctx->num_free;
  free(ptr);
}

/* ---------- */

static void test_custom_libc(void)
{
  /* Configure arguments. */
  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-5, (tph_poisson_real)-5 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)5, (tph_poisson_real)5 };
  const tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)0.37,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(23),
    .seed = UINT64_C(2015) };

  /* Initialize empty sampling. Use libc memset here. */
  tph_poisson_sampling sampling;
  memset(&sampling, 0, sizeof(tph_poisson_sampling));

  /* Populate sampling with points using default allocator. This should call
   * our custom libc functions. */
  num_malloc = 0;
  num_free = 0;
  num_memcpy = 0;
  num_memset = 0;
  int ret = tph_poisson_create(&args, /*alloc=*/NULL, &sampling);
  REQUIRE(ret == TPH_POISSON_SUCCESS);
  REQUIRE(tph_poisson_get_samples(&sampling) != NULL);
  tph_poisson_destroy(&sampling);
  REQUIRE(num_malloc > 0);
  REQUIRE(num_free > 0);
  REQUIRE(num_memcpy > 0);
  REQUIRE(num_memset > 0);
  const int num_default_malloc = num_malloc;
  const int num_default_free = num_free;
  const int num_default_memcpy = num_memcpy;
  const int num_default_memset = num_memset;

  /* Populate sampling again with points using a custom allocator. This should NOT
   * call our custom libc functions. */
  my_tph_alloc_ctx my_alloc_ctx = { .num_malloc = 0, .num_free = 0 };
  tph_poisson_allocator my_alloc = {
    .malloc = my_tph_malloc, .free = my_tph_free, .ctx = &my_alloc_ctx
  };
  ret = tph_poisson_create(&args, &my_alloc, &sampling);
  REQUIRE(ret == TPH_POISSON_SUCCESS);
  REQUIRE(tph_poisson_get_samples(&sampling) != NULL);
  tph_poisson_destroy(&sampling);

  /* Unchanged. */
  REQUIRE(num_malloc == num_default_malloc);
  REQUIRE(num_free == num_default_free);

  /* Same number of calls. */
  REQUIRE(num_default_malloc == my_alloc_ctx.num_malloc);
  REQUIRE(num_default_free == my_alloc_ctx.num_free);
  REQUIRE(num_memcpy == 2 * num_default_memcpy);
  REQUIRE(num_memset == 2 * num_default_memset);
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  printf("test_custom_libc...\n");
  test_custom_libc();

  return EXIT_SUCCESS;
}

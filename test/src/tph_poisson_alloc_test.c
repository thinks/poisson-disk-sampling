#include <stdint.h> /* UINT64_C, etc */
#include <stdio.h> /* printf */
#include <stdlib.h> /* malloc, free, abort, EXIT_SUCCESS */
#include <string.h> /* memset */

#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

#include "require.h"

typedef struct bad_alloc_ctx_
{
  int num_mallocs;
  int max_mallocs;
} bad_alloc_ctx;

static void *bad_alloc_malloc(ptrdiff_t size, void *ctx)
{
  bad_alloc_ctx *a_ctx = (bad_alloc_ctx *)ctx;
  if ((size == 0) | (a_ctx->num_mallocs >= a_ctx->max_mallocs)) { return NULL; }
  void *ptr = malloc((size_t)(size));
  ++a_ctx->num_mallocs;
  return ptr;
}

static void bad_alloc_free(void *ptr, ptrdiff_t size, void *ctx)
{
  (void)size;
  (void)ctx;
  if (ptr == NULL) { return; }
  free(ptr);
}

static void test_bad_alloc(void)
{
  /* The idea here is to use a custom allocator that fails (i.e. malloc returns null)
   * after a controllable number of allocations. This way it becomes possible
   * to verify that bad allocations are gracefully handled internally. However,
   * we don't know exactly how many allocations will be made (nor would we want
   * to rely on such knowledge for testing, since it may change). Therefore, our
   * custom allocator is configured to fail after 1, 2, 3, ... allocations, until
   * tph_poisson_create succeeds. This way all possible memory allocation failures
   * will be hit.
   *
   * Note that without this kind of exhaustive test it would be very difficult to
   * achieve full test code coverage. */

  /* Configure arguments. */
  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-10, (tph_poisson_real)-10 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)10, (tph_poisson_real)10 };
  const tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)1,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(30),
    .seed = UINT64_C(1981) };

  /* Initialize empty sampling. */
  tph_poisson_sampling sampling;
  memset(&sampling, 0, sizeof(tph_poisson_sampling));

  /* Verify arguments using default allocator. Reset sampling. */
  REQUIRE(tph_poisson_create(&args, /*alloc=*/NULL, &sampling) == TPH_POISSON_SUCCESS);
  tph_poisson_destroy(&sampling);

  int ret = TPH_POISSON_BAD_ALLOC;
  int i = 0;
  while (ret != TPH_POISSON_SUCCESS) {
    /* Use a custom allocator that will fail after 'i' allocations. Note that
     * if 'i' == 0 the first allocation fails. */
    bad_alloc_ctx alloc_ctx = { .num_mallocs = 0, .max_mallocs = i };
    tph_poisson_allocator alloc = {
      .malloc = bad_alloc_malloc, .free = bad_alloc_free, .ctx = &alloc_ctx
    };

    /* Try to populate sampling with points. */
    ret = tph_poisson_create(&args, &alloc, &sampling);
    REQUIRE(ret == TPH_POISSON_BAD_ALLOC || ret == TPH_POISSON_SUCCESS);
    ++i;
  }

  /* Free memory associated with sampling. */
  tph_poisson_destroy(&sampling);
}

typedef struct destroyed_alloc_ctx_
{
  int num_mallocs;
  int num_frees;
} destroyed_alloc_ctx;

static void *destroyed_alloc_malloc(ptrdiff_t size, void *ctx)
{
  if (size == 0) { return NULL; }
  void *ptr = malloc((size_t)(size));
  destroyed_alloc_ctx *a_ctx = (destroyed_alloc_ctx *)ctx;
  ++a_ctx->num_mallocs;
  return ptr;
}

static void destroyed_alloc_free(void *ptr, ptrdiff_t size, void *ctx)
{
  (void)size;
  if (ptr == NULL) { return; }
  destroyed_alloc_ctx *a_ctx = (destroyed_alloc_ctx *)ctx;
  ++a_ctx->num_frees;
  free(ptr);
}

static void test_destroyed_alloc(void)
{
  /* Configure arguments. */
  const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-10, (tph_poisson_real)-10 };
  const tph_poisson_real bounds_max[2] = { (tph_poisson_real)10, (tph_poisson_real)10 };
  const tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)1,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(30),
    .seed = UINT64_C(1981) };

  /* Initialize empty sampling. */
  tph_poisson_sampling sampling;
  memset(&sampling, 0, sizeof(tph_poisson_sampling));

  /* Set up a simple allocator that count number of allocations/deallocations. */
  destroyed_alloc_ctx alloc_ctx = { .num_mallocs = 0, .num_frees = 0 };
  tph_poisson_allocator *alloc = (tph_poisson_allocator*)malloc(sizeof(tph_poisson_default_alloc));
  alloc->malloc = destroyed_alloc_malloc;
  alloc->free = destroyed_alloc_free;
  alloc->ctx = &alloc_ctx;

  REQUIRE(tph_poisson_create(&args, alloc, &sampling) == TPH_POISSON_SUCCESS);

  /* Destroy the allocator before deallocating the sampling memory. The sampling
   * should have stored the malloc/free function pointers and context internally,
   * so there should be no errors. Note that the allocator context is not destroyed,
   * only the allocator.
   *
   * This prevent the implementation from storing a pointer to the allocator
   * internally. Instead the implementation should copy the malloc/free function
   * pointers (and context) so that it does not depend on the lifetime of the
   * allocator instance. */
  /* free(alloc); */
  REQUIRE(alloc_ctx.num_mallocs > 0);
  REQUIRE(alloc_ctx.num_frees > 0);

  /* Free memory associated with sampling. */
  tph_poisson_destroy(&sampling);
}

static void test_arena_alloc(void) {}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  // printf("TestUserAlloc...\n");
  // TestUserAlloc();

  printf("test_bad_alloc...\n");
  test_bad_alloc();

#if 0
  printf("test_destroyed_alloc...\n");
  test_destroyed_alloc();
#endif

  printf("test_arena_alloc...\n");
  test_arena_alloc();

  return EXIT_SUCCESS;
}

#include <inttypes.h> /* PRIXPTR, etc */
#include <stdbool.h> /* bool, true, false */
#include <stdio.h> /* printf */
#include <stdlib.h> /* malloc, free, abort, EXIT_SUCCESS */

#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

#include "require.h"

typedef struct vec_test_alloc_ctx_
{
  ptrdiff_t align_offset; /** [bytes] */
  int fail; /** When non-zero vec_test_malloc returns NULL. */
} vec_test_alloc_ctx;

static void *vec_test_malloc(ptrdiff_t size, void *ctx)
{
  vec_test_alloc_ctx *a_ctx = (vec_test_alloc_ctx *)ctx;
  if ((size == 0) | (a_ctx->fail != 0)) { return NULL; }
  void *ptr = malloc((size_t)(size + a_ctx->align_offset));

  return (void *)((intptr_t)ptr + a_ctx->align_offset);
}

static void vec_test_free(void *ptr, ptrdiff_t size, void *ctx)
{
  (void)size;
  if (ptr == NULL) { return; }
  vec_test_alloc_ctx *a_ctx = (vec_test_alloc_ctx *)ctx;
  free((void *)((intptr_t)ptr - a_ctx->align_offset));
}

#define VEC_TEST_SIZEOF(_X_) (ptrdiff_t)sizeof(_X_)
#define VEC_TEST_ALIGNOF(_X_) (ptrdiff_t)alignof(_X_)

static bool flt_eq(const float *a, const float *b)
{
  return memcmp((const void *)a, (const void *)b, sizeof(float)) == 0 ? true : false;
}

static bool is_zeros(const void *const mem, const ptrdiff_t n)
{
  static const uint8_t test_block[128] = { 0 };
  assert(n > 0);
  assert(n <= 128);
  return memcmp((const void *)test_block, mem, (size_t)n) == 0;
}

/* ------------------------- */

/* This function is reasonable for a vector, but is not required by the implementation.
 * It is provided here since it is useful for testing purposes. */
static inline ptrdiff_t tph_poisson_vec_capacity(const tph_poisson_vec *vec)
{
  assert(vec != NULL);
  assert(vec->mem_size >= 0);
  assert((intptr_t)vec->begin >= (intptr_t)vec->mem);

  /* NOTE: Account for possible alignment correction when computing capacity! */
  return (ptrdiff_t)(vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem));
}

#if 0
static void print_my_vec_f(const tph_poisson_vec *vec)
{
  /* clang-format off */
  printf("mem      = 0x%" PRIXPTR "\n"
         "mem_size = %td [bytes]\n"
         "begin    = 0x%" PRIXPTR "\n"
         "end      = 0x%" PRIXPTR "\n"
         "size     = %td, %td [bytes]\n"
         "cap      = %td, %td [bytes]\n",
    (uintptr_t)vec->mem,
    vec->mem_size,
    (uintptr_t)vec->begin,
    (uintptr_t)vec->end,
    tph_poisson_vec_size(vec) / VEC_TEST_SIZEOF(float), tph_poisson_vec_size(vec), 
    tph_poisson_vec_capacity(vec) / VEC_TEST_SIZEOF(float), tph_poisson_vec_capacity(vec));
  /* clang-format on */
  printf("data     = [ ");
  const float *iter = (const float *)vec->begin;
  const float *const iend = (const float *)vec->end;
  for (; iter != iend; ++iter) { printf("%.3f%s", (double)*iter, iter + 1 == iend ? "" : ", "); }
  printf(" ]\n\n");
}
#endif

static bool valid_invariants(tph_poisson_vec *vec, const ptrdiff_t alignment)
{
  if (vec == NULL) { return false; }

  /* Zero-initialized. */
  if (is_zeros(vec, VEC_TEST_SIZEOF(tph_poisson_vec))) { return true; }

  /* clang-format off */
  if (!(vec->mem != NULL && 
        vec->mem_size > 0 && 
        (intptr_t)vec->begin >= (intptr_t)vec->mem && 
        (intptr_t)vec->end >= (intptr_t)vec->begin && 
        (intptr_t)vec->begin % alignment == 0)) {
    return false;
  }
  /* clang-format on */

  if (!(tph_poisson_vec_capacity(vec) >= tph_poisson_vec_size(vec))) { return false; }

  return true;
}

static void test_reserve(void)
{
  for (size_t i = 0; i < sizeof(float); ++i) {
    /* Create an allocator that returns misaligned memory (except when i == 0). */
    vec_test_alloc_ctx alloc_ctx = { .align_offset = (ptrdiff_t)i };
    tph_poisson_allocator alloc = {
      .malloc = vec_test_malloc, .free = vec_test_free, .ctx = &alloc_ctx
    };
    const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F };
    const ptrdiff_t extra_cap =
      alloc_ctx.align_offset == 0 ? VEC_TEST_ALIGNOF(float) : alloc_ctx.align_offset;

    {
      /* Reserve on zero-initialized vector. No existing capacity. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_reserve(
                &vec, &alloc, /*new_cap=*/VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == 0);
      REQUIRE(tph_poisson_vec_capacity(&vec) == VEC_TEST_SIZEOF(values) + extra_cap);

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Reserve less than existing capacity is a no-op. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_reserve(
                &vec, &alloc, /*new_cap=*/VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      uint8_t vec0[sizeof(tph_poisson_vec)];
      memcpy(vec0, &vec, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/VEC_TEST_SIZEOF(float) / 2,
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(memcmp(vec0, &vec, sizeof(tph_poisson_vec)) == 0);
      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Append some values and then reserve a larger buffer. Check that existing
         values are copied to new buffer. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(tph_poisson_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/VEC_TEST_SIZEOF(values) + 3 * VEC_TEST_SIZEOF(values),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_capacity(&vec)
              == VEC_TEST_SIZEOF(values) + 3 * VEC_TEST_SIZEOF(values) + extra_cap);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, VEC_TEST_SIZEOF(values)) == 0);

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Reserve additional capacity for a vector with existing capacity but no values. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      tph_poisson_vec_append(
        &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float));
      tph_poisson_vec_erase_swap(&vec, 0, VEC_TEST_SIZEOF(values));
      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == 0);
      REQUIRE(tph_poisson_vec_capacity(&vec) == VEC_TEST_SIZEOF(values) + extra_cap);

      REQUIRE(tph_poisson_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/2 * VEC_TEST_SIZEOF(values),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Check that bad allocation propagates to caller. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      alloc_ctx.fail = 1;
      REQUIRE(tph_poisson_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/VEC_TEST_SIZEOF(float),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_BAD_ALLOC);
      alloc_ctx.fail = 0;

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros((const void *)&vec, sizeof(tph_poisson_vec)));
      /* No need to free the vector! */
    }
  }
}

static void test_append(void)
{
  for (size_t i = 0; i < sizeof(float); ++i) {
    /* Create an allocator that returns misaligned memory (except when i == 0). */
    vec_test_alloc_ctx alloc_ctx = { .align_offset = (ptrdiff_t)i };
    tph_poisson_allocator alloc = {
      .malloc = vec_test_malloc, .free = vec_test_free, .ctx = &alloc_ctx
    };
    const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F };

    {
      /* Append to zero-initialized vector. No existing capacity. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == VEC_TEST_SIZEOF(values));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, sizeof(values)) == 0);

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Append to empty vector with existing capacity. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(
        tph_poisson_vec_reserve(&vec, &alloc, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 2 * sizeof(float)) == 0);

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Append to vector with elements. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, &values[2], 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == 4 * VEC_TEST_SIZEOF(float));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 4 * sizeof(float)) == 0);

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Append to vector with existing capacity but no values. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      tph_poisson_vec_append(
        &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float));
      tph_poisson_vec_erase_swap(&vec, 0, VEC_TEST_SIZEOF(values));
      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == 0);

      /* clang-format off */
      const float values2[] = { 
        0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F,
        0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F,
      };
      /* clang-format on */

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values2, VEC_TEST_SIZEOF(values2), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Append that causes reallocation and element copy. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float) + VEC_TEST_SIZEOF(values));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 2 * sizeof(float)) == 0);
      REQUIRE(memcmp((const void *)((intptr_t)vec.begin + 2 * VEC_TEST_SIZEOF(float)),
                (const void *)values,
                VEC_TEST_SIZEOF(values))
              == 0);

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Check that bad allocation propagates to caller. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      alloc_ctx.fail = 1;
      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_BAD_ALLOC);
      alloc_ctx.fail = 0;

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros((const void *)&vec, sizeof(tph_poisson_vec)));
      /* No need to free the vector! */
    }
  }
}

static void test_erase_swap(void)
{
  for (size_t i = 0; i < sizeof(float); ++i) {
    /* Create an allocator that returns misaligned memory (except when i == 0). */
    vec_test_alloc_ctx alloc_ctx = { .align_offset = (ptrdiff_t)i };
    tph_poisson_allocator alloc = {
      .malloc = vec_test_malloc, .free = vec_test_free, .ctx = &alloc_ctx
    };
    const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F };

    /* Append some values. */
    tph_poisson_vec vec;
    memset(&vec, 0, sizeof(tph_poisson_vec));
    REQUIRE(
      tph_poisson_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
      == TPH_POISSON_SUCCESS);

    const ptrdiff_t mem_size0 = vec.mem_size;
    void *const mem0 = vec.mem;
    void *const begin0 = vec.begin;

    /* Remove first of several elements. */
    tph_poisson_vec_erase_swap(
      &vec, /*pos=*/0 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(tph_poisson_vec_size(&vec) == 6 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
    REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));
    REQUIRE(flt_eq((const float *)vec.begin + 4, &values[4]));
    REQUIRE(flt_eq((const float *)vec.begin + 5, &values[5]));

    /* Remove last of several elements. */
    tph_poisson_vec_erase_swap(
      &vec, /*pos=*/5 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(tph_poisson_vec_size(&vec) == 5 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
    REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));
    REQUIRE(flt_eq((const float *)vec.begin + 4, &values[4]));

    /* Remove third and fourth element. */
    tph_poisson_vec_erase_swap(
      &vec, /*pos=*/2 * VEC_TEST_SIZEOF(float), /*n=*/2 * VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(tph_poisson_vec_size(&vec) == 3 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[4]));

    /* Remove second element. */
    tph_poisson_vec_erase_swap(
      &vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(tph_poisson_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[4]));

    /* Remove second element. */
    tph_poisson_vec_erase_swap(
      &vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(tph_poisson_vec_size(&vec) == 1 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));

    /* Remove only remaining element. */
    tph_poisson_vec_erase_swap(
      &vec, /*pos=*/0 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(tph_poisson_vec_size(&vec) == 0);

    /* Capacity unchanged, no reallocation, vector is empty. */
    REQUIRE(vec.mem_size == mem_size0);
    REQUIRE((intptr_t)vec.mem == (intptr_t)mem0);
    REQUIRE((intptr_t)vec.begin == (intptr_t)begin0);
    REQUIRE((intptr_t)vec.begin == (intptr_t)vec.end);

    tph_poisson_vec_free(&vec, &alloc);
  }
}

static void test_shrink_to_fit(void)
{
  for (size_t i = 0; i < sizeof(float); ++i) {
    /* Create an allocator that returns misaligned memory (except when i == 0). */
    vec_test_alloc_ctx alloc_ctx = { .align_offset = (ptrdiff_t)i };
    tph_poisson_allocator alloc = {
      .malloc = vec_test_malloc, .free = vec_test_free, .ctx = &alloc_ctx
    };
    const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F };
    const ptrdiff_t extra_cap =
      alloc_ctx.align_offset == 0 ? VEC_TEST_ALIGNOF(float) : alloc_ctx.align_offset;

    {
      /* No-op on zero-initialized vector. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros(&vec, VEC_TEST_SIZEOF(tph_poisson_vec)));
      /* No need to free the vector! */
    }

    {
      /* Capacity but no elements. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      REQUIRE(
        tph_poisson_vec_reserve(&vec, &alloc, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(tph_poisson_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros(&vec, VEC_TEST_SIZEOF(tph_poisson_vec)));
      /* No need to free the vector! */
    }

    {
      /* size == capacity means that shrink_to_fit is a no-op. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      /* Append some values to an empty vector. */
      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      if (extra_cap == VEC_TEST_SIZEOF(float)) {
        REQUIRE(tph_poisson_vec_append(
                  &vec, &alloc, values, VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
                == TPH_POISSON_SUCCESS);
      }

      /* Shrink and verify that vector is bit-wise intact. */
      tph_poisson_vec vec0;
      memcpy(&vec0, &vec, sizeof(tph_poisson_vec));

      REQUIRE(tph_poisson_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(memcmp(&vec, &vec0, sizeof(tph_poisson_vec)) == 0);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, VEC_TEST_SIZEOF(values)) == 0);

      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Verify that shrinking removes extraneous capacity. This requires a reallocation and copying
       * elements to the smaller buffer. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      /* Reserve a large capacity and append only a few values so that shrinking removes
       * extraneous capacity (which triggers reallocation). */
      REQUIRE(
        tph_poisson_vec_reserve(&vec, &alloc, 20 * VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);
      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == VEC_TEST_SIZEOF(values));
      REQUIRE(tph_poisson_vec_capacity(&vec) == 20 * VEC_TEST_SIZEOF(values) + extra_cap);

      /* Shrink capacity. */
      REQUIRE(tph_poisson_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(tph_poisson_vec_size(&vec) == VEC_TEST_SIZEOF(values));
      REQUIRE(tph_poisson_vec_capacity(&vec) == VEC_TEST_SIZEOF(values) + extra_cap);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, sizeof(values)) == 0);
      tph_poisson_vec_free(&vec, &alloc);
    }

    {
      /* Check that bad allocation propagates to caller. */
      tph_poisson_vec vec;
      memset(&vec, 0, sizeof(tph_poisson_vec));

      /* Reserve a large capacity and append only a few values so that shrinking removes
       * extraneous capacity (which triggers reallocation). */
      REQUIRE(
        tph_poisson_vec_reserve(&vec, &alloc, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);
      REQUIRE(tph_poisson_vec_append(
                &vec, &alloc, values, VEC_TEST_SIZEOF(values) / 2, VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      /* Check that caller is notified when reallocation fails. */
      alloc_ctx.fail = 1;
      REQUIRE(tph_poisson_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_BAD_ALLOC);
      alloc_ctx.fail = 0;

      tph_poisson_vec_free(&vec, &alloc);
    }
  }
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  printf("test_reserve...\n");
  test_reserve();

  printf("test_append...\n");
  test_append();

  printf("test_erase_swap...\n");
  test_erase_swap();

  printf("test_shrink_to_fit...\n");
  test_shrink_to_fit();

  return EXIT_SUCCESS;
}

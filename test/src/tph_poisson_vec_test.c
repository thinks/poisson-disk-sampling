#include <inttypes.h>// PRIXPTR, etc
#include <stdbool.h>// bool, true, false
#include <stdio.h>// printf
#include <stdlib.h>// malloc, free, EXIT_SUCCESS

#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

static void
  require_fail(const char *expr, const char *file, unsigned int line, const char *function)
{
  printf(
    "Requirement failed: '%s' on line %u in file %s in function %s\n", expr, line, file, function);
  abort();
}

/* clang-format off */
#ifdef _MSC_VER
  #define TPH_PRETTY_FUNCTION __FUNCSIG__
#else 
  #define TPH_PRETTY_FUNCTION __func__
#endif
#define REQUIRE(expr) \
  ((bool)(expr) ? (void)0 : require_fail(#expr, __FILE__, __LINE__, TPH_PRETTY_FUNCTION))
/* clang-format on */

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

/*-------------------------*/

typedef struct my_vec_
{
  ptrdiff_t mem_size; /** [bytes] */
  void *mem;
  void *begin;
  void *end;
} my_vec;

static inline void my_vec_free(my_vec *vec, const tph_poisson_allocator *alloc)
{
  assert(vec != NULL);
  assert(vec->mem != NULL);
  assert(vec->mem_size > 0);
  assert(alloc != NULL);
  alloc->free(vec->mem, vec->mem_size, alloc->ctx);
}

static inline ptrdiff_t my_vec_size(const my_vec *vec)
{
  assert((intptr_t)vec->end >= (intptr_t)vec->begin);
  return (ptrdiff_t)((intptr_t)vec->end - (intptr_t)vec->begin);
}

static inline ptrdiff_t my_vec_capacity(const my_vec *vec)
{
  assert(vec != NULL);
  assert(vec->mem_size >= 0);
  assert((intptr_t)vec->begin >= (intptr_t)vec->mem);

  /* NOTE: Account for possible alignment correction when computing capacity! */
  return (ptrdiff_t)(vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem));
}

static int my_vec_reserve(my_vec *vec,
  const tph_poisson_allocator *alloc,
  const ptrdiff_t new_cap,
  const ptrdiff_t alignment)
{
  assert(vec != NULL);
  assert(alloc != NULL);
  assert(new_cap > 0);
  assert(alignment > 0);

  /* NOTE: For zero-initialized vector cap is 0. */
  const ptrdiff_t cap = vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem);
  if (new_cap <= cap) { return TPH_POISSON_SUCCESS; }

  /* Allocate and align a new buffer with sufficient capacity. Take into account that
   * the memory returned by the allocator may not match the requested alignment. */
  const ptrdiff_t new_mem_size = new_cap + alignment;
  void *const new_mem = alloc->malloc(new_mem_size, alloc->ctx);
  if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
  void *const new_begin = tph_poisson_align(new_mem, (size_t)alignment);

  const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;

  /* Copy existing data (if any) to the new buffer. */
  if (size > 0) {
    assert(vec->begin != NULL);
    memcpy(new_begin, vec->begin, (size_t)size);
  }

  /* Destroy the old buffer (if any). */
  if (vec->mem_size > 0) {
    assert(vec->mem != NULL);
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
  }

  /* Configure vector to use the new buffer. */
  vec->mem = new_mem;
  vec->mem_size = new_mem_size;
  vec->begin = new_begin;
  vec->end = (void *)((intptr_t)new_begin + size);

  assert(vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem) >= new_cap);

  return TPH_POISSON_SUCCESS;
}

static int my_vec_append(my_vec *vec,
  const tph_poisson_allocator *alloc,
  const void *buf,
  const ptrdiff_t n,
  const ptrdiff_t alignment)
{
  assert(vec != NULL);
  assert(alloc != NULL);
  assert(buf != NULL);
  assert(n > 0);
  assert(alignment > 0);

  const ptrdiff_t cap = vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem);
  const ptrdiff_t req_cap = (intptr_t)vec->end - (intptr_t)vec->begin + n;
  if (req_cap > cap) {
    /* Current buffer does not have enough capacity. Try doubling the vector
     * capacity and check if it's enough to hold the new elements. If not,
     * set the new capacity to hold exactly the new elements. */
    ptrdiff_t new_cap = cap <= (PTRDIFF_MAX >> 1) ? (cap << 1) : cap;
    if (new_cap < req_cap) { new_cap = req_cap; }

    /* Allocate and align a new buffer with sufficient capacity. Take into
     * account that the memory returned by the allocator may not be aligned to
     * the type of element that will be stored. */
    new_cap += alignment;
    void *new_mem = alloc->malloc(new_cap, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *new_begin = tph_poisson_align(new_mem, (size_t)alignment);

    const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;

    /* Copy existing data (if any) to the new buffer. */
    if (size > 0) {
      assert(vec->begin != NULL);
      memcpy(new_begin, vec->begin, (size_t)size);
    }

    /* Destroy the old buffer (if any). */
    if (vec->mem_size > 0) {
      assert(vec->mem != NULL);
      alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    }

    /* Configure vector to use the new buffer. */
    vec->mem = new_mem;
    vec->mem_size = new_cap;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)new_begin + size);
  }
  assert(vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem) >= req_cap);
  assert((intptr_t)vec->end % alignment == 0);

  memcpy(vec->end, buf, (size_t)n);
  vec->end = (void *)((intptr_t)vec->end + n);
  return TPH_POISSON_SUCCESS;
}

static void my_vec_erase_swap(my_vec *vec, const ptrdiff_t pos, const ptrdiff_t n)
{
  assert(vec != NULL);
  assert(pos >= 0);
  assert((intptr_t)vec->begin + pos < (intptr_t)vec->end);
  assert(n >= 1);
  assert((intptr_t)vec->begin + pos + n <= (intptr_t)vec->end);

  const intptr_t dst_begin = (intptr_t)vec->begin + pos;
  const intptr_t dst_end = dst_begin + n;
  intptr_t src_begin = (intptr_t)vec->end - n;
  if (src_begin < dst_end) { src_begin = dst_end; }
  const intptr_t m = (intptr_t)vec->end - src_begin;
  if (m > 0) { memcpy((void *)dst_begin, (const void *)src_begin, (size_t)m); }
  /* else: when erasing up to the last element there is no need to copy anything,
   *       just "pop" the end of the vector. */

  /* "pop" the end of the buffer, decreasing the vector size by n. */
  vec->end = (void *)((intptr_t)vec->end - n);
}

static int
  my_vec_shrink_to_fit(my_vec *vec, const tph_poisson_allocator *alloc, const ptrdiff_t alignment)
{
  if (vec->end == vec->begin) {
    /* Empty vector, no elements. Wipe capacity!
     * This includes the case of a zero-initialized vector. */
    if (vec->mem != NULL) {
      /* Existing vector is empty but has capacity. */
      assert(vec->mem_size > 0);
      alloc->free(vec->mem, vec->mem_size, alloc->ctx);
      memset((void *)vec, 0, sizeof(my_vec));
    }
    assert(vec->mem == NULL);
    assert(vec->mem_size == 0);
    return TPH_POISSON_SUCCESS;
  }

  /* TODO: HANDLE ADDITIONAL CAPACITY FROM ALIGNMENT CORRECTION!?!? */

  const ptrdiff_t cap = vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem);
  const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;
  if (cap > size + alignment) {
    /* Allocate and align a new buffer with sufficient capacity. Take into
     * account that the memory returned by the allocator may not be aligned to
     * the type of element that will be stored. */
    const ptrdiff_t new_mem_size = size + alignment;
    void *const new_mem = alloc->malloc(new_mem_size, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *const new_begin = tph_poisson_align(new_mem, (size_t)alignment);

    /* Copy existing data (if any) to the new buffer. */
    if (size > 0) {
      assert(vec->begin != NULL);
      memcpy(new_begin, vec->begin, (size_t)size);
    }

    /* Destroy the old buffer (if any). */
    if (vec->mem_size > 0) {
      assert(vec->mem != NULL);
      alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    }

    /* Configure vector to use the new buffer. */
    vec->mem = new_mem;
    vec->mem_size = new_mem_size;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)new_begin + size);
  }
  assert(my_vec_capacity(vec) - my_vec_size(vec) <= alignment);
  return TPH_POISSON_SUCCESS;
}

#if 0
static void print_my_vec_f(const my_vec *vec)
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
    my_vec_size(vec) / VEC_TEST_SIZEOF(float), my_vec_size(vec), 
    my_vec_capacity(vec) / VEC_TEST_SIZEOF(float), my_vec_capacity(vec));
  /* clang-format on */
  printf("data     = [ ");
  const float *iter = (const float *)vec->begin;
  const float *const iend = (const float *)vec->end;
  for (; iter != iend; ++iter) { printf("%.3f%s", (double)*iter, iter + 1 == iend ? "" : ", "); }
  printf(" ]\n\n");
}
#endif

static bool valid_invariants(my_vec *vec, const ptrdiff_t alignment)
{
  if (vec == NULL) { return false; }

  /* Zero-initialized. */
  if (is_zeros(vec, VEC_TEST_SIZEOF(my_vec))) { return true; }

  /* clang-format off */
  if (!(vec->mem != NULL && 
        vec->mem_size > 0 && 
        (intptr_t)vec->begin >= (intptr_t)vec->mem && 
        (intptr_t)vec->end >= (intptr_t)vec->begin && 
        (intptr_t)vec->begin % alignment == 0)) {
    return false;
  }
  /* clang-format on */

  if (!(my_vec_capacity(vec) >= my_vec_size(vec))) { return false; }

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
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(
        my_vec_reserve(&vec, &alloc, /*new_cap=*/VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(my_vec_size(&vec) == 0);
      REQUIRE(my_vec_capacity(&vec) == VEC_TEST_SIZEOF(values) + extra_cap);

      my_vec_free(&vec, &alloc);
    }

    {
      /* Reserve less than existing capacity is a no-op. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(
        my_vec_reserve(&vec, &alloc, /*new_cap=*/VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);
      uint8_t vec0[sizeof(my_vec)];
      memcpy(vec0, &vec, sizeof(my_vec));

      REQUIRE(my_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/VEC_TEST_SIZEOF(float) / 2,
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(memcmp(vec0, &vec, sizeof(my_vec)) == 0);
      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));

      my_vec_free(&vec, &alloc);
    }

    {
      /* Append some values and then reserve a larger buffer. Check that existing
         values are copied to new buffer. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(my_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/VEC_TEST_SIZEOF(values) + 3 * VEC_TEST_SIZEOF(values),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(
        my_vec_capacity(&vec) == VEC_TEST_SIZEOF(values) + 3 * VEC_TEST_SIZEOF(values) + extra_cap);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, VEC_TEST_SIZEOF(values)) == 0);

      my_vec_free(&vec, &alloc);
    }

    {
      /* Reserve additional capacity for a vector with existing capacity but no values. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float));
      my_vec_erase_swap(&vec, 0, VEC_TEST_SIZEOF(values));
      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(my_vec_size(&vec) == 0);
      REQUIRE(my_vec_capacity(&vec) == VEC_TEST_SIZEOF(values) + extra_cap);

      REQUIRE(my_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/2 * VEC_TEST_SIZEOF(values),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      my_vec_free(&vec, &alloc);
    }

    {
      /* Check that bad allocation propagates to caller. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      alloc_ctx.fail = 1;
      REQUIRE(my_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/VEC_TEST_SIZEOF(float),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_BAD_ALLOC);
      alloc_ctx.fail = 0;

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros((const void *)&vec, sizeof(my_vec)));
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
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(my_vec_size(&vec) == VEC_TEST_SIZEOF(values));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, sizeof(values)) == 0);

      my_vec_free(&vec, &alloc);
    }

    {
      /* Append to empty vector with existing capacity. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(my_vec_reserve(&vec, &alloc, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(
        my_vec_append(&vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(my_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 2 * sizeof(float)) == 0);

      my_vec_free(&vec, &alloc);
    }

    {
      /* Append to vector with elements. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(
        my_vec_append(&vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(
        my_vec_append(&vec, &alloc, &values[2], 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(my_vec_size(&vec) == 4 * VEC_TEST_SIZEOF(float));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 4 * sizeof(float)) == 0);

      my_vec_free(&vec, &alloc);
    }

    {
      /* Append to vector with existing capacity but no values. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float));
      my_vec_erase_swap(&vec, 0, VEC_TEST_SIZEOF(values));
      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(my_vec_size(&vec) == 0);

      /* clang-format off */
      const float values2[] = { 
        0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F,
        0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F,
      };
      /* clang-format on */

      REQUIRE(
        my_vec_append(&vec, &alloc, values2, VEC_TEST_SIZEOF(values2), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      my_vec_free(&vec, &alloc);
    }

    {
      /* Append that causes reallocation and element copy. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(
        my_vec_append(&vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(my_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float) + VEC_TEST_SIZEOF(values));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 2 * sizeof(float)) == 0);
      REQUIRE(memcmp((const void *)((intptr_t)vec.begin + 2 * VEC_TEST_SIZEOF(float)),
                (const void *)values,
                VEC_TEST_SIZEOF(values))
              == 0);

      my_vec_free(&vec, &alloc);
    }

    {
      /* Check that bad allocation propagates to caller. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      alloc_ctx.fail = 1;
      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_BAD_ALLOC);
      alloc_ctx.fail = 0;

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros((const void *)&vec, sizeof(my_vec)));
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
    my_vec vec;
    memset(&vec, 0, sizeof(my_vec));
    REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
            == TPH_POISSON_SUCCESS);

    const ptrdiff_t mem_size0 = vec.mem_size;
    void *const mem0 = vec.mem;
    void *const begin0 = vec.begin;

    /* Remove first of several elements. */
    my_vec_erase_swap(&vec, /*pos=*/0 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(my_vec_size(&vec) == 6 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
    REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));
    REQUIRE(flt_eq((const float *)vec.begin + 4, &values[4]));
    REQUIRE(flt_eq((const float *)vec.begin + 5, &values[5]));

    /* Remove last of several elements. */
    my_vec_erase_swap(&vec, /*pos=*/5 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(my_vec_size(&vec) == 5 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
    REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));
    REQUIRE(flt_eq((const float *)vec.begin + 4, &values[4]));

    /* Remove third and fourth element. */
    my_vec_erase_swap(&vec, /*pos=*/2 * VEC_TEST_SIZEOF(float), /*n=*/2 * VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(my_vec_size(&vec) == 3 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[4]));

    /* Remove second element. */
    my_vec_erase_swap(&vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(my_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[4]));

    /* Remove second element. */
    my_vec_erase_swap(&vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(my_vec_size(&vec) == 1 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));

    /* Remove only remaining element. */
    my_vec_erase_swap(&vec, /*pos=*/0 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
    REQUIRE(my_vec_size(&vec) == 0);

    /* Capacity unchanged, no reallocation, vector is empty. */
    REQUIRE(vec.mem_size == mem_size0);
    REQUIRE((intptr_t)vec.mem == (intptr_t)mem0);
    REQUIRE((intptr_t)vec.begin == (intptr_t)begin0);
    REQUIRE((intptr_t)vec.begin == (intptr_t)vec.end);

    my_vec_free(&vec, &alloc);
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
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(my_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float)) == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros(&vec, VEC_TEST_SIZEOF(my_vec)));
      /* No need to free the vector! */
    }

    {
      /* Capacity but no elements. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      REQUIRE(my_vec_reserve(&vec, &alloc, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      REQUIRE(my_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float)) == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(is_zeros(&vec, VEC_TEST_SIZEOF(my_vec)));
      /* No need to free the vector! */
    }

    {
      /* size == capacity means that shrink_to_fit is a no-op. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      /* Append some values to an empty vector. */
      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      if (extra_cap == VEC_TEST_SIZEOF(float)) {
        REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
                == TPH_POISSON_SUCCESS);
      }

      /* Shrink and verify that vector is bit-wise intact. */
      my_vec vec0;
      memcpy(&vec0, &vec, sizeof(my_vec));

      REQUIRE(my_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float)) == TPH_POISSON_SUCCESS);

      REQUIRE(valid_invariants(&vec, VEC_TEST_ALIGNOF(float)));
      REQUIRE(memcmp(&vec, &vec0, sizeof(my_vec)) == 0);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, VEC_TEST_SIZEOF(values)) == 0);

      my_vec_free(&vec, &alloc);
    }

#if 0
    { 
      /* Verify that shrinking removes extraneous capacity. This requires a reallocation and copying
       * elements to the smaller buffer. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      /* Reserve a large capacity and append only a few values so that shrinking removes
       * extraneous capacity (which triggers reallocation). */
      REQUIRE(my_vec_reserve(&vec, &alloc, 20 * VEC_TEST_SIZEOF(float), alignof(float))
              == TPH_POISSON_SUCCESS);
      const float values[] = { 0.F, -1.F, 13.F, 42.F };
      const ptrdiff_t n = (ptrdiff_t)(sizeof(values) / sizeof(values[0]));
      REQUIRE(my_vec_append(&vec, &alloc, values, n * (ptrdiff_t)sizeof(float), alignof(float))
              == TPH_POISSON_SUCCESS);
lkj

      print_my_vec_f(&vec);
      REQUIRE(my_vec_size(&vec, VEC_TEST_SIZEOF(float)) == n);
      REQUIRE(my_vec_capacity(float, &vec) == (alloc_ctx.align_offset == 0 ? n + 1 : n));

      /* Append two more values, causing capacity to double. */
      REQUIRE(my_vec_append(&vec, &alloc, values, 2 * (ptrdiff_t)(sizeof(float)), alignof(float))
              == TPH_POISSON_SUCCESS);
      print_my_vec_f(&vec);
      REQUIRE(my_vec_size(&vec, VEC_TEST_SIZEOF(float)) == n + 2);
      REQUIRE(my_vec_capacity(float, &vec) == (alloc_ctx.align_offset == 0 ? 2 * n + 3 : 2 * n));

      /* Shrink capacity. */
      REQUIRE(my_vec_shrink_to_fit(&vec, &alloc, alignof(float)) == TPH_POISSON_SUCCESS);
      print_my_vec_f(&vec);
      REQUIRE(my_vec_size(&vec, VEC_TEST_SIZEOF(float)) == n + 2);
      REQUIRE(my_vec_capacity(float, &vec) == (alloc_ctx.align_offset == 0 ? n + 1 : n));
      REQUIRE(flt_eq((const float *)vec.begin + 0, &values[0]));
      REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
      REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
      REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));
      REQUIRE(flt_eq((const float *)vec.begin + 4, &values[0]));
      REQUIRE(flt_eq((const float *)vec.begin + 5, &values[1]));

      my_vec_free(&vec, &alloc);
    }
#endif
    {
      /* Check that bad allocation propagates to caller. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      /* Reserve a large capacity and append only a few values so that shrinking removes
       * extraneous capacity (which triggers reallocation). */
      REQUIRE(my_vec_reserve(&vec, &alloc, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(
        my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values) / 2, VEC_TEST_ALIGNOF(float))
        == TPH_POISSON_SUCCESS);

      /* Check that caller is notified when reallocation fails. */
      alloc_ctx.fail = 1;
      REQUIRE(my_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float)) == TPH_POISSON_BAD_ALLOC);
      alloc_ctx.fail = 0;

      my_vec_free(&vec, &alloc);
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

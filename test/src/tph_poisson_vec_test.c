#include <inttypes.h>// PRIXPTR, etc
#include <stdbool.h>// bool, true, false
#include <stdio.h>// printf
#include <stdlib.h>// malloc, free, EXIT_SUCCESS

#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

typedef struct vec_test_alloc_ctx_
{
  ptrdiff_t align_offset; /** [bytes] */
  int fail; /** When non-zero vec_test_malloc returns NULL. */
} vec_test_alloc_ctx;

static void *vec_test_malloc(ptrdiff_t size, void *ctx)
{
  vec_test_alloc_ctx *a_ctx = (vec_test_alloc_ctx *)ctx;
  if ((size == 0) | (a_ctx->fail != 0)) { return NULL; }
  void *ptr = malloc((size_t)size);
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
  alloc->free(vec->mem, vec->mem_size, alloc->ctx);
  /*memset((void *)vec, 0, sizeof(my_vec));*/
}

static inline ptrdiff_t my_vec_size(const my_vec *vec)
{
  assert((intptr_t)vec->end >= (intptr_t)vec->begin);
  return (ptrdiff_t)((intptr_t)vec->end - (intptr_t)vec->begin);
  // return (ptrdiff_t)(((intptr_t)vec->end - (intptr_t)vec->begin) / sizeof_elem);
}

static inline ptrdiff_t my_vec_capacity(const my_vec *vec)
{
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
  assert(alignment > 0);

  if (new_cap > my_vec_capacity(vec)) {
    /* Allocate and align a new buffer with sufficient capacity. Take into
     * account that the memory returned by the allocator may not be aligned to
     * the type of element that will be stored. */
    const ptrdiff_t new_mem_size = new_cap + alignment;
    void *new_mem = alloc->malloc(new_mem_size, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *new_begin = tph_poisson_align(new_mem, (size_t)alignment);

    /* Copy existing elements (if any) to the new buffer and destroy the old buffer. */
    const ptrdiff_t size = my_vec_size(vec);
    if (size > 0) {
      assert(((int)(vec->mem != NULL) & (int)(vec->begin != NULL)) == 1);
      memcpy(new_begin, vec->begin, (size_t)size);
      alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    }

    /* Configure vector to use the new buffer. */
    vec->mem = new_mem;
    vec->mem_size = new_mem_size;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)new_begin + size);
  }
  return TPH_POISSON_SUCCESS;
}

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static int my_vec_append(my_vec *vec,
  const tph_poisson_allocator *alloc,
  const void *buf,
  const ptrdiff_t n,
  const ptrdiff_t alignment)
{
  assert(buf != NULL);
  assert(n > 0);
  assert(alignment > 0); /* ??? */

  const ptrdiff_t cap = my_vec_capacity(vec);
  const ptrdiff_t req_cap = my_vec_size(vec) + n;
  if (req_cap > cap) {
    /* Current buffer does not have enough capacity. Try doubling the vector
     * capacity and check if it's enough to hold the new elements. If not,
     * set the new capacity to hold exactly the new elements. */
    ptrdiff_t new_cap = cap <= (PTRDIFF_MAX >> 1) ? (cap << 1) : cap;
    if (new_cap < req_cap) { new_cap = req_cap; }
    const int ret = my_vec_reserve(vec, alloc, new_cap, alignment);
    if (ret != TPH_POISSON_SUCCESS) { return ret; }
  }
  assert(my_vec_capacity(vec) >= req_cap);
  memcpy(vec->end, buf, (size_t)n);
  vec->end = (void *)((intptr_t)vec->end + n);
  return TPH_POISSON_SUCCESS;

#if 0
  /* NOTE: Account for possible alignment correction when computing current capacity! */
  const ptrdiff_t cap_bytes =
    vec->mem_size - (ptrdiff_t)((intptr_t)vec->begin - (intptr_t)vec->mem);
  const ptrdiff_t size_bytes = (intptr_t)vec->end - (intptr_t)vec->begin;
  const ptrdiff_t req_cap_bytes = size_bytes + buf_size_bytes;
  if (cap_bytes < req_cap_bytes) {
    /* Current buffer does not have enough capacity. Try doubling the vector
     * capacity and check if it's enough to hold the new elements. If not,
     * set the new capacity to hold exactly the new elements. */
    ptrdiff_t new_cap_bytes = cap_bytes <= (PTRDIFF_MAX >> 1) ? (cap_bytes << 1) : cap_bytes;
    if (new_cap_bytes < req_cap_bytes) { new_cap_bytes = req_cap_bytes; }
    const int ret = my_vec_reserve(vec, alloc, new_cap_bytes, buf_align);
    if (ret != TPH_POISSON_SUCCESS) { return ret; }
  } 
  assert((intptr_t)vec->end + buf_size_bytes <= (intptr_t)vec->mem + vec->mem_size);
  memcpy(vec->end, buf, (size_t)buf_size_bytes);
  vec->end = (void *)((intptr_t)vec->end + buf_size_bytes);
  return TPH_POISSON_SUCCESS;
#endif
}

static void my_vec_erase_swap(my_vec *vec, const ptrdiff_t pos, const ptrdiff_t n)
{
  assert(pos >= 0);
  assert((intptr_t)vec->begin + pos < (intptr_t)vec->end);
  assert(n >= 1);
  assert((intptr_t)vec->begin + pos + n <= (intptr_t)vec->end);

  const intptr_t dst_begin = (intptr_t)vec->begin + pos;
  const intptr_t dst_end = dst_begin + n;
  intptr_t src_begin = (intptr_t)vec->end - n;
  if (src_begin < dst_end) {
    src_begin = dst_end;
  }
  const intptr_t m = (intptr_t)vec->end - src_begin;
  if (m > 0) {
    memcpy((void *)dst_begin, (const void *)src_begin, (size_t)m);
  }
  /* else: when erasing the last element there is no need to copy anything,
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
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    memset((void *)vec, 0, sizeof(my_vec));
    return TPH_POISSON_SUCCESS;
  }

  /* TODO: HANDLE ADDITIONAL CAPACITY FROM ALIGNMENT CORRECTION!?!? */

  const ptrdiff_t size = my_vec_size(vec);
  if (my_vec_capacity(vec) > size + alignment) {
    /* Create new buffer that holds exactly the amount of data present in the existing vector. */
    my_vec new_vec;
    memset((void *)&new_vec, 0, sizeof(my_vec));
    const int ret = my_vec_reserve(&new_vec, alloc, size, alignment);
    if (ret != TPH_POISSON_SUCCESS) { return ret; }

    /* Copy contents to the new buffer and free the old buffer. */
    assert(new_vec.begin != NULL);
    memcpy(new_vec.begin, vec->begin, (size_t)size);
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);

    /* Configure existing vector to use the new buffer. */
    vec->mem = new_vec.mem;
    vec->mem_size = new_vec.mem_size;
    vec->begin = new_vec.begin;
    vec->end = (void *)((intptr_t)new_vec.begin + size);
  }
  return TPH_POISSON_SUCCESS;
}


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

static void test_reserve(void)
{
  for (size_t i = 0; i < sizeof(float); ++i) {
    /* Create an allocator that returns misaligned memory (except when i == 0). */
    vec_test_alloc_ctx alloc_ctx = { .align_offset = (ptrdiff_t)i };
    tph_poisson_allocator alloc = {
      .malloc = vec_test_malloc, .free = vec_test_free, .ctx = &alloc_ctx
    };
    const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, -34.F };

    {
      /* Reserve on zero-initialized vector. No existing capacity. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      REQUIRE(my_vec_reserve(
                &vec, &alloc, /*new_cap=*/VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == 0);
      REQUIRE(my_vec_capacity(&vec) >= VEC_TEST_SIZEOF(values));
      REQUIRE((intptr_t)vec.begin % VEC_TEST_ALIGNOF(float) == 0);
      my_vec_free(&vec, &alloc);
    }

    {
      /* Append some values and then reserve a larger buffer. Check that existing
         values are copied to new buffer. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == VEC_TEST_SIZEOF(values));
      REQUIRE(my_vec_capacity(&vec) >= VEC_TEST_SIZEOF(values));
      REQUIRE((intptr_t)vec.begin % VEC_TEST_ALIGNOF(float) == 0);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, VEC_TEST_SIZEOF(values)) == 0);

      void* mem0 = vec.mem;
      REQUIRE(my_vec_reserve(
                &vec, &alloc, /*new_cap=*/VEC_TEST_SIZEOF(values) + 7, VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE((intptr_t)vec.mem != (intptr_t)mem0);
      REQUIRE(my_vec_size(&vec) == VEC_TEST_SIZEOF(values));
      REQUIRE(my_vec_capacity(&vec) >= VEC_TEST_SIZEOF(values) + 3);
      REQUIRE((intptr_t)vec.begin % VEC_TEST_ALIGNOF(float) == 0);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, VEC_TEST_SIZEOF(values)) == 0);
      my_vec_free(&vec, &alloc);
    }

    {
      /* Reserve less than existing capacity is a no-op. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      REQUIRE(my_vec_reserve(
                &vec, &alloc, /*new_cap=*/10 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      uint8_t vec0[sizeof(my_vec)];
      memcpy(vec0, &vec, sizeof(my_vec));
      REQUIRE(my_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/5 * VEC_TEST_SIZEOF(float),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(memcmp(vec0, &vec, sizeof(my_vec)) == 0);
      my_vec_free(&vec, &alloc);
    }

    {
      /* Check that bad allocation propagates to caller. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      alloc_ctx.fail = 1;
      REQUIRE(my_vec_reserve(&vec,
                &alloc,
                /*new_cap=*/10 * VEC_TEST_SIZEOF(float),
                VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_BAD_ALLOC);
      alloc_ctx.fail = 0;
      my_vec_free(&vec, &alloc);
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
      REQUIRE(my_vec_capacity(&vec) == 0);
      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == VEC_TEST_SIZEOF(values));
      REQUIRE(my_vec_capacity(&vec) >= VEC_TEST_SIZEOF(values));
      REQUIRE((intptr_t)vec.begin % VEC_TEST_ALIGNOF(float) == 0);
      REQUIRE((intptr_t)vec.end % VEC_TEST_ALIGNOF(float) == 0);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, sizeof(values)) == 0);
      my_vec_free(&vec, &alloc);
    }

    {
      /* Append to empty vector with existing capacity. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      REQUIRE(my_vec_reserve(&vec, &alloc, 5 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == 0);
      REQUIRE(my_vec_capacity(&vec) >= 5 * VEC_TEST_SIZEOF(float));
      const ptrdiff_t cap0 = my_vec_capacity(&vec);
      void * const mem0 = vec.mem;
      REQUIRE(my_vec_append(&vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
      REQUIRE(my_vec_capacity(&vec) == cap0);
      REQUIRE((intptr_t)vec.mem == (intptr_t)mem0);
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 2 * sizeof(float)) == 0);
      my_vec_free(&vec, &alloc);
    }

    {
      /* Append to vector with elements. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      REQUIRE(my_vec_append(&vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 2 * sizeof(float)) == 0);
      REQUIRE(my_vec_append(&vec, &alloc, &values[2], 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == 4 * VEC_TEST_SIZEOF(float));
      REQUIRE(memcmp((const void *)vec.begin, (const void *)values, 4 * sizeof(float)) == 0);
      my_vec_free(&vec, &alloc);
    }

    {
      /* Append that causes reallocation and element copy. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      REQUIRE(my_vec_append(&vec, &alloc, values, 2 * VEC_TEST_SIZEOF(float), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
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
      my_vec_free(&vec, &alloc);
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

    void *const mem0 = vec.mem;
    const ptrdiff_t mem_size0 = vec.mem_size;
    void *const begin0 = vec.begin;

    /* Remove first of several elements. */
    my_vec_erase_swap(&vec, /*pos=*/0, /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(my_vec_size(&vec) == 6 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
    REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));
    REQUIRE(flt_eq((const float *)vec.begin + 4, &values[4]));
    REQUIRE(flt_eq((const float *)vec.begin + 5, &values[5]));

    /* Remove last of several elements. */
    my_vec_erase_swap(&vec, /*pos=*/5 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(my_vec_size(&vec) == 5 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[1]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
    REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));
    REQUIRE(flt_eq((const float *)vec.begin + 4, &values[4]));

    /* Remove second element. */
    my_vec_erase_swap(&vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(my_vec_size(&vec) == 4 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[4]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));
    REQUIRE(flt_eq((const float *)vec.begin + 3, &values[3]));

    /* Remove second element. */
    my_vec_erase_swap(&vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(my_vec_size(&vec) == 3 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[3]));
    REQUIRE(flt_eq((const float *)vec.begin + 2, &values[2]));

    /* Remove second element. */
    my_vec_erase_swap(&vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(my_vec_size(&vec) == 2 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));
    REQUIRE(flt_eq((const float *)vec.begin + 1, &values[2]));

    /* Remove second element. */
    my_vec_erase_swap(&vec, /*pos=*/1 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(my_vec_size(&vec) == 1 * VEC_TEST_SIZEOF(float));
    REQUIRE(flt_eq((const float *)vec.begin + 0, &values[6]));

    /* Remove only remaining element. */
    my_vec_erase_swap(&vec, /*pos=*/0 * VEC_TEST_SIZEOF(float), /*n=*/VEC_TEST_SIZEOF(float));
    REQUIRE(my_vec_size(&vec) == 0);

    /* Capacity unchanged, no reallocation, vector is empty. */
    REQUIRE((intptr_t)vec.mem == (intptr_t)mem0);
    REQUIRE(vec.mem_size == mem_size0);
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

    { 
      /* No-op on zero-initialized vector. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));
      my_vec vec0;
      memcpy(&vec0, &vec, sizeof(my_vec));
      REQUIRE(my_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float)) == TPH_POISSON_SUCCESS);
      REQUIRE(memcmp(&vec, &vec0, sizeof(my_vec)) == 0);
      my_vec_free(&vec, &alloc);
    }

    { 
      /* size == capacity means that shrink_to_fit should be a no-op. */
      my_vec vec;
      memset(&vec, 0, sizeof(my_vec));

      /* Append some values to an empty vector. */
      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);

      /* Shrink and verify that vector is bit-wise intact. */
      my_vec vec0;
      memcpy(&vec0, &vec, sizeof(my_vec));
      REQUIRE(my_vec_shrink_to_fit(&vec, &alloc, VEC_TEST_ALIGNOF(float)) == TPH_POISSON_SUCCESS);
      REQUIRE(memcmp(&vec, &vec0, sizeof(my_vec)) == 0);
      assert(vec.begin != NULL); /* TMP!?!?!???????????????????????!!! */
      REQUIRE(
        memcmp((const void *)vec.begin, (const void *)values, VEC_TEST_SIZEOF(values)) == 0);
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
      REQUIRE(my_vec_reserve(&vec, &alloc, 3 * VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
              == TPH_POISSON_SUCCESS);
      REQUIRE(my_vec_append(&vec, &alloc, values, VEC_TEST_SIZEOF(values), VEC_TEST_ALIGNOF(float))
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

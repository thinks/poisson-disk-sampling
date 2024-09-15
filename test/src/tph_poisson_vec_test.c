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

static bool flt_eq(const float *a, const float *b)
{
  return memcmp((const void *)a, (const void *)b, sizeof(float)) == 0 ? true : false;
}

typedef struct my_vec_
{
  void *mem;
  ptrdiff_t mem_size; /** [bytes] */
  void *begin;
  void *end;
} my_vec;

static inline void my_vec_free(my_vec *vec, const tph_poisson_allocator *alloc)
{
  alloc->free(vec->mem, vec->mem_size, alloc->ctx);
  memset((void *)vec, 0, sizeof(my_vec));
}

#define my_vec_size(ElemT, vec) (my_vec_size_impl((vec), (ptrdiff_t)sizeof(ElemT)))
static inline ptrdiff_t my_vec_size_impl(const my_vec *vec, const ptrdiff_t sizeof_elem)
{
  return (ptrdiff_t)(((intptr_t)vec->end - (intptr_t)vec->begin) / sizeof_elem);
}

#define my_vec_capacity(ElemT, vec) (my_vec_capacity_impl((vec), (ptrdiff_t)sizeof(ElemT)))
static inline ptrdiff_t my_vec_capacity_impl(const my_vec *vec, const ptrdiff_t sizeof_elem)
{
  /* NOTE: Account for possible alignment loss when computing current capacity! */
  return (ptrdiff_t)((vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem)) / sizeof_elem);
}

#define my_vec_reserve(ElemT, vec, alloc, new_cap) \
  (my_vec_reserve_impl(                            \
    (vec), (alloc), (new_cap), (ptrdiff_t)sizeof(ElemT), (ptrdiff_t)alignof(ElemT)))
static int my_vec_reserve_impl(my_vec *vec,
  const tph_poisson_allocator *alloc,
  const ptrdiff_t new_cap,
  const ptrdiff_t sizeof_elem,
  const ptrdiff_t alignof_elem)
{
  if (my_vec_capacity_impl(vec, sizeof_elem) < new_cap) {
    /* Allocate and align a new buffer with sufficient capacity.
     * Take into account that the memory may not be aligned to the
     * type of element that will be stored. */
    const ptrdiff_t new_mem_size = new_cap * sizeof_elem + (alignof_elem - 1);
    void *new_mem = alloc->malloc(new_mem_size, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *new_begin = tph_poisson_align(new_mem, (size_t)alignof_elem);

    /* Copy existing elements (if any) to the new buffer and
     * destroy the old buffer. */
    const ptrdiff_t size = my_vec_size_impl(vec, sizeof_elem);
    if (size > 0) {
      memcpy(new_begin, vec->begin, (size_t)(size * sizeof_elem));
      alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    }

    /* Configure vector to use the new buffer. */
    vec->mem = new_mem;
    vec->mem_size = new_mem_size;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)vec->begin + size * sizeof_elem);
  }
  return TPH_POISSON_SUCCESS;
}

#define my_vec_append(ElemT, vec, alloc, values, count) \
  (my_vec_append_impl(                                  \
    (vec), (alloc), (values), (count), (ptrdiff_t)sizeof(ElemT), (ptrdiff_t)alignof(ElemT)))
static int my_vec_append_impl(my_vec *vec,
  const tph_poisson_allocator *alloc,
  const void *values,
  const ptrdiff_t count,
  const ptrdiff_t sizeof_elem,
  const ptrdiff_t alignof_elem)
{
  assert(((int)(values != NULL) & (int)(count > 0)) == 1);
  assert(((int)(sizeof_elem > 0) & (int)(alignof_elem > 0)) == 1);

  const ptrdiff_t size = my_vec_size_impl(vec, sizeof_elem);
  ptrdiff_t cap = my_vec_capacity_impl(vec, sizeof_elem);
  const ptrdiff_t req_cap = size + count;
  if (cap < req_cap) {
    /* Current buffer does not have enough capacity. Try doubling the vector
     * capacity and check if it's enough to hold the new elements. If not,
     * set the new capacity to hold exactly the new elements. */
    if ((cap = cap <= (PTRDIFF_MAX >> 1) ? cap << 1 : cap) < req_cap) { cap = req_cap; }
    const int ret = my_vec_reserve_impl(vec, alloc, cap, sizeof_elem, alignof_elem);
    if (ret != TPH_POISSON_SUCCESS) { return ret; }
  }
  const ptrdiff_t n = count * sizeof_elem;
  assert((intptr_t)vec->end + n <= (intptr_t)vec->mem + vec->mem_size);
  memcpy(vec->end, values, (size_t)n);
  vec->end = (void *)((intptr_t)vec->end + n);
  assert(my_vec_size_impl(vec, sizeof_elem) == size + count);
  assert(my_vec_capacity_impl(vec, sizeof_elem) >= size + count);
  return TPH_POISSON_SUCCESS;
}

#define my_vec_erase_unordered(ElemT, vec, pos) \
  (my_vec_erase_unordered_impl((vec), (pos), (ptrdiff_t)sizeof(ElemT)))
static void
  my_vec_erase_unordered_impl(my_vec *vec, const ptrdiff_t pos, const ptrdiff_t sizeof_elem)
{
  assert(((int)(0 <= pos) & (int)(pos < my_vec_size_impl(vec, sizeof_elem))) == 1);

  const intptr_t er_iter = (intptr_t)vec->begin + pos * sizeof_elem;
  const intptr_t last_elem = (intptr_t)vec->end - sizeof_elem;
  if (er_iter < last_elem) {
    /* Not erasing the last element. Copy the last element into the position
     * to be erased. Note that this also covers the case of erasing the sole
     * remaining (i.e. last) element from a vector of size one, which would otherwise lead
     * to an overlapping range in the memcpy below (which sanitizers should detect). */
    memcpy((void *)er_iter, (const void *)last_elem, (size_t)sizeof_elem);
  }
  /* else: when erasing the last element there is no need to copy anything,
   *       just "pop" the end of the vector. */

  /* "pop" the last element, decreasing the vector size by one element. */
  vec->end = (void *)((intptr_t)vec->end - sizeof_elem);
}

#define my_vec_shrink_to_fit(ElemT, vec, alloc) \
  (my_vec_shrink_to_fit_impl((vec), (alloc), (ptrdiff_t)sizeof(ElemT), (ptrdiff_t)alignof(ElemT)))
static int my_vec_shrink_to_fit_impl(my_vec *vec,
  const tph_poisson_allocator *alloc,
  const ptrdiff_t sizeof_elem,
  const ptrdiff_t alignof_elem)
{
  if (vec->end == vec->begin) {
    /* Empty vector, no elements. Wipe capacity!
     * This includes the case of a zero-initialized vector. */
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    memset((void *)vec, 0, sizeof(my_vec));
    return TPH_POISSON_SUCCESS;
  }

  const ptrdiff_t cap_bytes = vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem);
  const ptrdiff_t size_bytes = (intptr_t)vec->end - (intptr_t)vec->begin;
  if (cap_bytes - size_bytes >= sizeof_elem) {
    /* There is enough additional memory to add one or more elements, shrink the buffer.
     * Create new buffer that holds exactly the amount of data present in the existing vector. */
    my_vec new_vec;
    memset((void *)&new_vec, 0, sizeof(my_vec));
    int ret =
      my_vec_reserve_impl(&new_vec, alloc, size_bytes / sizeof_elem, sizeof_elem, alignof_elem);
    if (ret != TPH_POISSON_SUCCESS) { return TPH_POISSON_BAD_ALLOC; }

    /* Copy contents to the new buffer and free the old buffer. */
    //    if (new_vec.begin == NULL) { return TPH_POISSON_BAD_ALLOC; } /* TMP */
    assert(new_vec.begin != NULL);
    memcpy(new_vec.begin, vec->begin, (size_t)size_bytes);
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);

    /* Configure existing vector to use the new buffer. */
    vec->mem = new_vec.mem;
    vec->mem_size = new_vec.mem_size;
    vec->begin = new_vec.begin;
    vec->end = (void *)((intptr_t)new_vec.begin + size_bytes);
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

// clang-format off
#ifdef _MSC_VER
  #define TPH_PRETTY_FUNCTION __FUNCSIG__
#else 
  #define TPH_PRETTY_FUNCTION __func__
#endif
#define REQUIRE(expr) \
  ((bool)(expr) ? (void)0 : require_fail(#expr, __FILE__, __LINE__, TPH_PRETTY_FUNCTION))
// clang-format on

typedef struct Elem_
{
  int8_t i;
  float f;
} Elem;
static_assert(sizeof(Elem) != alignof(Elem), "Elem layout");

/**
 * @brief Verifies that vector state is sane. Returns true if the vector is in a valid state;
 * otherwise false.
 * @param ElemT Vector element type.
 * @param vec   Vector.
 * @return True if the vector is in a valid state; otherwise false.
 */
#if 0
#define check_vec_invariants(ElemT, vec) \
  (check_vec_invariants_impl((vec), sizeof(ElemT), alignof(ElemT)))
static bool
  check_vec_invariants_impl(const my_vec *vec, const size_t elem_size, const size_t elem_align)
{
  bool valid = vec != NULL;
  valid &= (elem_size > 0);
  valid &= (elem_align > 0);
  if (!valid) { return false; } /* Bad args */
  valid = (vec->mem == NULL);
  valid &= (vec->mem_size == 0);
  valid &= (vec->begin == NULL);
  valid &= (vec->end == NULL);
  if (valid) { return true; } /* Zero-initialized. */
  /* Zero or more elements in vector (non-zero capacity). */
  valid = (vec->mem != NULL);
  valid &= (vec->mem_size > 0);
  valid &= ((uintptr_t)vec->begin >= (uintptr_t)vec->mem);
  valid &= ((uintptr_t)vec->begin <= (uintptr_t)vec->end);
  valid &= ((uintptr_t)vec->begin % elem_align == 0);
  valid &= ((intptr_t)vec->end <= (intptr_t)vec->mem + vec->mem_size);
  valid &= ((uintptr_t)vec->end % elem_align == 0);
  valid &= (((uintptr_t)vec->end - (uintptr_t)vec->begin) % elem_size == 0);
  valid &= (((uintptr_t)vec->begin - (uintptr_t)vec->mem) < elem_align);
  return valid;
}

static void print_my_vec(const my_vec *vec)
{
  /* clang-format off */
  printf("mem      = 0x%" PRIXPTR "\n"
         "mem_size = %td [bytes]\n"
         "begin    = 0x%" PRIXPTR "\n"
         "end      = 0x%" PRIXPTR "\n"
         "size     = %td, %td [bytes]\n"
         "cap      = %td\n",
    (uintptr_t)vec->mem,
    vec->mem_size,
    (uintptr_t)vec->begin,
    (uintptr_t)vec->end,
    my_vec_size(Elem, vec), (intptr_t)vec->end - (intptr_t)vec->begin,
    my_vec_capacity(Elem, vec));
  /* clang-format on */
  printf("data     = [ ");
  const Elem *iter = (const Elem *)vec->begin;
  const Elem *const iend = (const Elem *)vec->end;
  for (; iter != iend; ++iter) {
    printf("(%" PRIi8 ", %.3f)%s", iter->i, (double)iter->f, iter + 1 == iend ? "" : ", ");
  }
  printf(" ]\n\n");
}
#endif

static void print_my_vec_f(const my_vec *vec)
{
  /* clang-format off */
  printf("mem      = 0x%" PRIXPTR "\n"
         "mem_size = %td [bytes]\n"
         "begin    = 0x%" PRIXPTR "\n"
         "end      = 0x%" PRIXPTR "\n"
         "size     = %td, %td [bytes]\n"
         "cap      = %td\n",
    (uintptr_t)vec->mem,
    vec->mem_size,
    (uintptr_t)vec->begin,
    (uintptr_t)vec->end,
    my_vec_size(float, vec), (ptrdiff_t)((intptr_t)vec->end - (intptr_t)vec->begin),
    my_vec_capacity(float, vec));
  /* clang-format on */
  printf("data     = [ ");
  const float *iter = (const float *)vec->begin;
  const float *const iend = (const float *)vec->end;
  for (; iter != iend; ++iter) { printf("%.3f%s", (double)*iter, iter + 1 == iend ? "" : ", "); }
  printf(" ]\n\n");
}

static void test_reserve(void)
{
  my_vec vec;
  memset(&vec, 0, sizeof(my_vec));
  const float elems[] = { 0.F, 1.F, 2.F, 3.F };
  my_vec_append(float, &vec, &tph_poisson_default_alloc, elems, sizeof(elems) / sizeof(elems[0]));
  print_my_vec_f(&vec);

  const float elem2 = 4.F;
  my_vec_append(float, &vec, &tph_poisson_default_alloc, &elem2, 1);
  print_my_vec_f(&vec);

  const float elem3 = 5.F;
  my_vec_append(float, &vec, &tph_poisson_default_alloc, &elem3, 1);
  print_my_vec_f(&vec);
}

static void test_append(void)
{
  for (size_t i = 0; i < sizeof(float); ++i) {
    /* Create an allocator that returns misaligned memory (except when i == 0). */
    vec_test_alloc_ctx alloc_ctx = { .align_offset = (ptrdiff_t)i };
    tph_poisson_allocator alloc = { vec_test_malloc, vec_test_free, /*ctx=*/&alloc_ctx };

    my_vec vec;
    memset(&vec, 0, sizeof(my_vec));
    const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, 34.F };
    const ptrdiff_t n = (ptrdiff_t)(sizeof(values) / sizeof(values[0]));

    /* Append to zero-initialized vector. No existing capacity. */
    REQUIRE(my_vec_append(float, &vec, &alloc, values, n) == TPH_POISSON_SUCCESS);
    REQUIRE(my_vec_size(float, &vec) == n);
    REQUIRE(my_vec_capacity(float, &vec) == n);
    REQUIRE((uintptr_t)vec.begin % alignof(float) == 0);
    REQUIRE((uintptr_t)vec.end % alignof(float) == 0);

    const float *iter = (const float *)vec.begin;
    REQUIRE(flt_eq(iter + 0, &values[0]));
    REQUIRE(flt_eq(iter + 1, &values[1]));
    REQUIRE(flt_eq(iter + 2, &values[2]));
    REQUIRE(flt_eq(iter + 3, &values[3]));
    REQUIRE(flt_eq(iter + 4, &values[4]));
    REQUIRE(flt_eq(iter + 5, &values[5]));
    REQUIRE(flt_eq(iter + 6, &values[6]));

    /* Append to empty vector with existing capacity. */
    for (ptrdiff_t j = 0; j < n; ++j) { my_vec_erase_unordered(float, &vec, /*pos=*/0); }
    REQUIRE(my_vec_size(float, &vec) == 0);
    REQUIRE(my_vec_capacity(float, &vec) == n);
    void *mem0 = vec.mem;
    REQUIRE(my_vec_append(float, &vec, &alloc, values, /*count=*/2) == TPH_POISSON_SUCCESS);
    REQUIRE(my_vec_size(float, &vec) == 2);
    REQUIRE(my_vec_capacity(float, &vec) == n);
    REQUIRE(vec.mem == mem0);
    /* Iterator still valid, no reallocations. */
    REQUIRE(flt_eq(iter + 0, &values[0]));
    REQUIRE(flt_eq(iter + 1, &values[1]));

    /* Append to vector with elements. */
    REQUIRE(my_vec_append(float, &vec, &alloc, &values[2], /*count=*/2) == TPH_POISSON_SUCCESS);
    REQUIRE(my_vec_size(float, &vec) == 4);
    REQUIRE(my_vec_capacity(float, &vec) == n);
    /* Iterator still valid, no reallocations. */
    REQUIRE(flt_eq(iter + 0, &values[0]));
    REQUIRE(flt_eq(iter + 1, &values[1]));
    REQUIRE(flt_eq(iter + 2, &values[2]));
    REQUIRE(flt_eq(iter + 3, &values[3]));

    /* Append causes reallocation and element copy. */
    REQUIRE(my_vec_append(float, &vec, &alloc, &values[0], /*count=*/4) == TPH_POISSON_SUCCESS);
    REQUIRE(my_vec_size(float, &vec) == 8);
    REQUIRE(my_vec_capacity(float, &vec) == 2 * n);
    iter = (const float *)vec.begin;
    REQUIRE(flt_eq(iter + 0, &values[0]));
    REQUIRE(flt_eq(iter + 1, &values[1]));
    REQUIRE(flt_eq(iter + 2, &values[2]));
    REQUIRE(flt_eq(iter + 3, &values[3]));
    REQUIRE(flt_eq(iter + 4, &values[0]));
    REQUIRE(flt_eq(iter + 5, &values[1]));
    REQUIRE(flt_eq(iter + 6, &values[2]));
    REQUIRE(flt_eq(iter + 7, &values[3]));

    /* Check that bad allocation propagates to caller. */
    alloc_ctx.fail = 1;
    float values2[256] = { 0.F };
    REQUIRE(my_vec_append(float, &vec, &alloc, values2, sizeof(values) / sizeof(values[0]))
            == TPH_POISSON_BAD_ALLOC);

    my_vec_free(&vec, &alloc);
  }
}

static void test_erase(void)
{
  /* Append some values. Not using custom allocator here since erasing elements is
   * independent of allocator. Allocator is used only for setup/teardown. */
  const tph_poisson_allocator *alloc = &tph_poisson_default_alloc;
  my_vec vec;
  memset(&vec, 0, sizeof(my_vec));
  const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, 34.F };
  const ptrdiff_t n = (ptrdiff_t)(sizeof(values) / sizeof(values[0]));
  REQUIRE(my_vec_append(float, &vec, alloc, values, n) == TPH_POISSON_SUCCESS);
  REQUIRE(my_vec_size(float, &vec) == n);
  /* Erasing does not cause reallocation, so iterator is valid throughout. */
  const float *iter = (const float *)vec.begin;
  void *mem0 = vec.mem;
  const ptrdiff_t mem_size0 = vec.mem_size;

  /* Remove first of several elements. */
  my_vec_erase_unordered(float, &vec, /*pos=*/0);
  REQUIRE(my_vec_size(float, &vec) == n - 1);
  REQUIRE(flt_eq(iter + 0, &values[6]));
  REQUIRE(flt_eq(iter + 1, &values[1]));
  REQUIRE(flt_eq(iter + 2, &values[2]));
  REQUIRE(flt_eq(iter + 3, &values[3]));
  REQUIRE(flt_eq(iter + 4, &values[4]));
  REQUIRE(flt_eq(iter + 5, &values[5]));

  /* Remove last of several elements. */
  my_vec_erase_unordered(float, &vec, /*pos=*/5);
  REQUIRE(my_vec_size(float, &vec) == n - 2);
  REQUIRE(flt_eq(iter + 0, &values[6]));
  REQUIRE(flt_eq(iter + 1, &values[1]));
  REQUIRE(flt_eq(iter + 2, &values[2]));
  REQUIRE(flt_eq(iter + 3, &values[3]));
  REQUIRE(flt_eq(iter + 4, &values[4]));

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 3);
  REQUIRE(flt_eq(iter + 0, &values[6]));
  REQUIRE(flt_eq(iter + 1, &values[4]));
  REQUIRE(flt_eq(iter + 2, &values[2]));
  REQUIRE(flt_eq(iter + 3, &values[3]));

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 4);
  REQUIRE(flt_eq(iter + 0, &values[6]));
  REQUIRE(flt_eq(iter + 1, &values[3]));
  REQUIRE(flt_eq(iter + 2, &values[2]));

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 5);
  REQUIRE(flt_eq(iter + 0, &values[6]));
  REQUIRE(flt_eq(iter + 1, &values[2]));

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 6);
  REQUIRE(flt_eq(iter + 0, &values[6]));

  /* Remove only remaining element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/0);
  REQUIRE(my_vec_size(float, &vec) == 0);

  /* Capacity unchanged. */
  REQUIRE(vec.mem_size == mem_size0);

  /* No reallocation. */
  REQUIRE((uintptr_t)vec.mem == (uintptr_t)mem0);
  REQUIRE((uintptr_t)vec.begin == (uintptr_t)iter);

  /* Vector is empty. */
  REQUIRE((uintptr_t)vec.begin == (uintptr_t)vec.end);

  my_vec_free(&vec, alloc);
}

static void test_shrink_to_fit(void)
{
  for (size_t i = 0; i < sizeof(float); ++i) {
    /* Create an allocator that returns misaligned memory (except when i == 0). */
    vec_test_alloc_ctx alloc_ctx = { .align_offset = (ptrdiff_t)i };
    tph_poisson_allocator alloc = { vec_test_malloc, vec_test_free, /*ctx=*/&alloc_ctx };

    /* Append some values. */
    my_vec vec;
    memset(&vec, 0, sizeof(my_vec));

    /* No-op on zero-initialized vector. */
    REQUIRE(my_vec_shrink_to_fit(float, &vec, &alloc) == TPH_POISSON_SUCCESS);

    /* Append some values to an empty vector. */
    REQUIRE(my_vec_size(float, &vec) == 0);
    REQUIRE(my_vec_capacity(float, &vec) == 0);
    const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, 34.F };
    const ptrdiff_t n = (ptrdiff_t)(sizeof(values) / sizeof(values[0]));
    REQUIRE(my_vec_append(float, &vec, &alloc, values, n) == TPH_POISSON_SUCCESS);

    /* size == capacity means that shrink_to_fit should be a no-op.
     * No reallocation should happen. */
    REQUIRE(my_vec_size(float, &vec) == n);
    REQUIRE(my_vec_capacity(float, &vec) == n);
    void *mem0 = vec.mem;
    REQUIRE(my_vec_shrink_to_fit(float, &vec, &alloc) == TPH_POISSON_SUCCESS);
    REQUIRE(my_vec_size(float, &vec) == n);
    REQUIRE(my_vec_capacity(float, &vec) == n);
    REQUIRE(vec.mem == mem0);

    /* Append two more values, causing capacity to double. Verify that shrinking removes
     * extraneous capacity. This requires a reallocation and copying elements to the
     * smaller buffer. */
    REQUIRE(my_vec_append(float, &vec, &alloc, &values[0], /*count=*/2) == TPH_POISSON_SUCCESS);
    mem0 = vec.mem;
    REQUIRE(my_vec_size(float, &vec) == n + 2);
    REQUIRE(my_vec_capacity(float, &vec) == 2 * n);
    REQUIRE(my_vec_shrink_to_fit(float, &vec, &alloc) == TPH_POISSON_SUCCESS);
    REQUIRE(my_vec_size(float, &vec) == n + 2);
    REQUIRE(my_vec_capacity(float, &vec) == n + 2);
    REQUIRE(vec.mem != mem0);
    const float *iter = (const float *)vec.begin;
    REQUIRE(flt_eq(iter++, &values[0]));
    REQUIRE(flt_eq(iter++, &values[1]));
    REQUIRE(flt_eq(iter++, &values[2]));
    REQUIRE(flt_eq(iter++, &values[3]));
    REQUIRE(flt_eq(iter++, &values[4]));
    REQUIRE(flt_eq(iter++, &values[5]));
    REQUIRE(flt_eq(iter++, &values[6]));
    REQUIRE(flt_eq(iter++, &values[0]));
    REQUIRE(flt_eq(iter++, &values[1]));
    REQUIRE(iter == (const float *)vec.end);

    /* Check that bad allocation propagates to caller. First reserve capacity so that
     * shrinking is required. */
    REQUIRE(my_vec_reserve(float, &vec, &alloc, /*count=*/2 * n) == TPH_POISSON_SUCCESS);
    alloc_ctx.fail = 1;
    REQUIRE(my_vec_shrink_to_fit(float, &vec, &alloc) == TPH_POISSON_BAD_ALLOC);

    my_vec_free(&vec, &alloc);
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

  printf("test_erase...\n");
  test_erase();

  printf("test_shrink_to_fit...\n");
  test_shrink_to_fit();

  return EXIT_SUCCESS;
}

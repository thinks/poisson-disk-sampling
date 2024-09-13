#include <inttypes.h>// PRIXPTR, etc
#include <stdbool.h>// bool, true, false
#include <stdio.h>// printf
#include <stdlib.h>// EXIT_SUCCESS

#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

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
    if (cap <= (PTRDIFF_MAX >> 1)) { cap <<= 1; }
    if (cap < req_cap) { cap = req_cap; }

    int ret = my_vec_reserve_impl(vec, alloc, cap, sizeof_elem, alignof_elem);
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
    if (new_vec.begin == NULL) { return TPH_POISSON_BAD_ALLOC; } /* TMP */
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
    my_vec_size(float, vec), (intptr_t)vec->end - (intptr_t)vec->begin,
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
  my_vec vec;
  memset(&vec, 0, sizeof(my_vec));
  my_vec_reserve(Elem, &vec, &tph_poisson_default_alloc, 100);
  REQUIRE(check_vec_invariants(Elem, &vec));
  print_my_vec(&vec);

  REQUIRE(my_vec_size(Elem, &vec) == 0);

  const Elem elems[] = { { 0, 0.F }, { 1, 1.F }, { 2, 2.F }, { 3, 3.F }, { 4, 4.F }, { 5, 5.F } };

  REQUIRE(
    my_vec_append(Elem, &vec, &tph_poisson_default_alloc, elems, sizeof(elems) / sizeof(elems[0]))
    == TPH_POISSON_SUCCESS);
  REQUIRE(check_vec_invariants(Elem, &vec));
  print_my_vec(&vec);

  REQUIRE(my_vec_size(Elem, &vec) == sizeof(elems) / sizeof(elems[0]));

  REQUIRE(vec.mem != NULL);
  REQUIRE(vec.mem_size >= (ptrdiff_t)sizeof(elems));
  REQUIRE((uintptr_t)vec.begin == (uintptr_t)vec.mem);
  REQUIRE((uintptr_t)vec.begin % alignof(Elem) == 0);

  REQUIRE((intptr_t)vec.end < (intptr_t)vec.mem + vec.mem_size);
  REQUIRE((uintptr_t)vec.end == (uintptr_t)vec.begin + sizeof(elems));
  REQUIRE((uintptr_t)vec.end % alignof(Elem) == 0);

  REQUIRE(((uintptr_t)vec.end - (uintptr_t)vec.begin) % sizeof(Elem) == 0);

  /* This should double the vector capacity. */
  const Elem elems2[] = { { 6, 6.F }, { 7, 7.F } };
  REQUIRE(my_vec_append(
            Elem, &vec, &tph_poisson_default_alloc, elems2, sizeof(elems2) / sizeof(elems2[0]))
          == TPH_POISSON_SUCCESS);
  REQUIRE(my_vec_size(Elem, &vec)
          == sizeof(elems) / sizeof(elems[0]) + sizeof(elems2) / sizeof(elems2[0]));
  print_my_vec(&vec);

  /* Erase non-last element. */
  my_vec_erase_unordered(Elem, &vec, 3);
  print_my_vec(&vec);

  /* Erase last element. */
  my_vec_erase_unordered(Elem, &vec, 6);
  print_my_vec(&vec);

  REQUIRE(my_vec_shrink_to_fit(Elem, &vec, &tph_poisson_default_alloc) == TPH_POISSON_SUCCESS);
  print_my_vec(&vec);

  const Elem elem3 = { 8, 8.F };
  REQUIRE(my_vec_append(Elem, &vec, &tph_poisson_default_alloc, &elem3, 1) == TPH_POISSON_SUCCESS);
  print_my_vec(&vec);
}

static void test_erase(void)
{
  /* Append some values. */
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
  REQUIRE(memcmp((const void *)(iter + 0), (const void *)(&values[6]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 1), (const void *)(&values[1]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 2), (const void *)(&values[2]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 3), (const void *)(&values[3]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 4), (const void *)(&values[4]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 5), (const void *)(&values[5]), sizeof(values[0])) == 0);

  /* Remove last of several elements. */
  my_vec_erase_unordered(float, &vec, /*pos=*/5);
  REQUIRE(my_vec_size(float, &vec) == n - 2);
  REQUIRE(memcmp((const void *)(iter + 0), (const void *)(&values[6]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 1), (const void *)(&values[1]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 2), (const void *)(&values[2]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 3), (const void *)(&values[3]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 4), (const void *)(&values[4]), sizeof(values[0])) == 0);

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 3);
  REQUIRE(memcmp((const void *)(iter + 0), (const void *)(&values[6]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 1), (const void *)(&values[4]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 2), (const void *)(&values[2]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 3), (const void *)(&values[3]), sizeof(values[0])) == 0);

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 4);
  REQUIRE(memcmp((const void *)(iter + 0), (const void *)(&values[6]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 1), (const void *)(&values[3]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 2), (const void *)(&values[2]), sizeof(values[0])) == 0);

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 5);
  REQUIRE(memcmp((const void *)(iter + 0), (const void *)(&values[6]), sizeof(values[0])) == 0);
  REQUIRE(memcmp((const void *)(iter + 1), (const void *)(&values[2]), sizeof(values[0])) == 0);

  /* Remove second element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/1);
  REQUIRE(my_vec_size(float, &vec) == n - 6);
  REQUIRE(memcmp((const void *)(iter + 0), (const void *)(&values[6]), sizeof(values[0])) == 0);

  /* Remove only remaining element. */
  my_vec_erase_unordered(float, &vec, /*pos=*/0);
  REQUIRE(my_vec_size(float, &vec) == 0);

  /* Capacity unchanged. */
  REQUIRE(vec.mem_size == mem_size0);

  /* No reallocation. */
  REQUIRE((uintptr_t)vec.mem == (uintptr_t)mem0);
  REQUIRE((uintptr_t)vec.begin == (uintptr_t)iter);

  /* Vector is empty. */
  REQUIRE((uintptr_t)vec.begin == (uintptr_t)vec.mem);
  REQUIRE((uintptr_t)vec.begin == (uintptr_t)vec.end);

  my_vec_free(&vec, alloc);
}

static void test_shrink_to_fit(void)
{
  /* Append some values. */
  const tph_poisson_allocator *alloc = &tph_poisson_default_alloc;
  my_vec vec;
  memset(&vec, 0, sizeof(my_vec));

  /* No-op on zero-initialized vector. */
  REQUIRE(my_vec_shrink_to_fit(float, &vec, alloc) == TPH_POISSON_SUCCESS);

  /* Append some values to an empty vector. */
  REQUIRE(my_vec_size(float, &vec) == 0);
  REQUIRE(my_vec_capacity(float, &vec) == 0);
  const float values[] = { 0.F, 1.F, 13.F, 42.F, 33.F, 18.F, 34.F };
  const ptrdiff_t n = (ptrdiff_t)(sizeof(values) / sizeof(values[0]));
  REQUIRE(my_vec_append(float, &vec, alloc, values, n) == TPH_POISSON_SUCCESS);
  /* size == capacity  ->  shrink_to_fit is a no-op. */
  print_my_vec_f(&vec);
  REQUIRE(my_vec_shrink_to_fit(float, &vec, alloc) == TPH_POISSON_SUCCESS);
  REQUIRE(my_vec_size(float, &vec) == n);
  const ptrdiff_t cap = my_vec_capacity(float, &vec);
  REQUIRE(cap == n);

  my_vec_free(&vec, alloc);
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  // rpmalloc_initialize();

  printf("test_reserve...\n");
  test_reserve();

  printf("test_append...\n");
  test_append();

  printf("test_erase...\n");
  test_erase();

  printf("test_shrink_to_fit...\n");
  test_shrink_to_fit();

  // rpmalloc_finalize();

  return EXIT_SUCCESS;
}

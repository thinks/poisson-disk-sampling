#include <inttypes.h>
#include <stdio.h>


#define TPH_POISSON_IMPLEMENTATION
#include <thinks/tph_poisson.h>

static void *vec_align(void *ptr, const size_t alignment)
{
  return (void *)(((uintptr_t)ptr + (alignment - 1)) & ~(alignment - 1));
}


typedef struct vec_
{
  void *mem;
  ptrdiff_t capacity; /** [bytes] */
  ptrdiff_t size; /** [bytes] */
} vec;

/**
 * @brief Frees all memory associated with the vector.
 * @param vec - Vector.
 * @return void
 */
static void vec_free(vec *v)
{
  if (v) {
    free(v->mem);
    // alloc->free(
    //   , TPH_POISSON_SIZEOF(tph_poisson_vec_header) + hdr->capacity,
    //   alloc->ctx);
  }
}

/**
 * @brief Returns the number of elements in the vector.
 * @param Type Type of elements in vector.
 * @param vec  Vector.
 * @return The number of elements in the vector.
 */
#define vec_size(Type, vec) (vec->size / (ptrdiff_t)sizeof(Type))

/**
 * @brief Add elements to the end of the vector.
 * @param vec    Vector.
 * @param values Pointer to values to be added.
 * @param count  Number of elements to copy from values.
 * @param alloc  Allocator.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
#define vec_append(Type, vec, values, count)         \
  (/*static_assert(sizeof(Type) == sizeof(*(values)), ""),*/ \
    _vec_append((vec), (values), (count) * sizeof(*(values)), alignof(Type)))
static int _vec_append(vec *v, const void *values, const ptrdiff_t nbytes, size_t elem_align)
{
  const ptrdiff_t req_cap = v->size + nbytes;
  if (v->capacity < req_cap) {
    /* Try doubling the vector capacity and check if it's enough to hold
     * the new elements. */
    ptrdiff_t new_cap = v->capacity ? v->capacity : (ptrdiff_t)1;
    if (new_cap <= (PTRDIFF_MAX >> 1)) { new_cap <<= 1; }
    if (new_cap < req_cap) { new_cap = req_cap; }
    void *new_mem = realloc(v->mem, new_cap + elem_align);
    if (!new_mem) { return 2; }
    /* re-align!*/
    v->mem = new_mem;
    v->capacity = new_cap;
  }
  memcpy((void *)((uintptr_t)vec_align(v->mem, elem_align) + (size_t)v->size), values, nbytes);
  v->size += nbytes;
  return 0;
}

/**
 * @brief ...
 * @param vec   Vector.
 * @param pos   Starting index of elements to be erased; pos <= 0 is always a
 * no-op.
 * @param count Number of elements to erase; count <= 0 is always a no-op.
 */
#define vec_erase_unordered(Type, vec, pos, count) \
  (_vec_erase_unordered((vec), (pos) * sizeof(Type), (count) * sizeof(Type), alignof(Type)))
static int _vec_erase_unordered(vec *v, ptrdiff_t pos, ptrdiff_t count, size_t elem_align)
{
  if (!((pos >= 0) & (pos < v->size) & (count > 0))) { return 2; /*invalid args*/ }
  if (pos + count >= v->size) {
    /* There are no elements behind the specified range. No re-ordering is necessary, simply
     * decrease the size of the vector. */
    v->size = pos;
  } else {
    /* Copy elements from the end of the vector into the positions to be erased and then decrease
     * the size of the vector. */
    ptrdiff_t n_src = count;
    ptrdiff_t remain = v->size - pos - count;
    if (n_src > remain) { n_src = remain; }
    void *aligned_mem = vec_align(v->mem, elem_align);
    memcpy((void *)((uintptr_t)aligned_mem + (size_t)pos),
      (void *)((uintptr_t)aligned_mem + (size_t)(v->size - n_src)),
      n_src);
    v->size -= count;
  }
  return 0;
}

/**
 * @brief ...
 */
#define vec_shrink_to_fit(Type, vec) (_vec_shrink_to_fit((vec), alignof(Type)))
static int _vec_shrink_to_fit(vec *v, size_t elem_align)
{
  assert(v->size <= v->capacity);
  if (v->size == v->capacity) { return TPH_POISSON_SUCCESS; }
  void *new_mem = realloc(v->mem, /*new_size=*/v->size + elem_align);
  if (!new_mem) { return 1; }
  v->mem = new_mem;
  v->capacity = v->size;
  return 0;
}

#define vec_data(Type, vec) ((Type*)(vec_align((vec)->mem, alignof(Type))))

static void print_vecf(vec* v)
{
  printf("mem = %" PRIxPTR " | cap = %" PRIu32 " | size = %" PRIu32 "\n",
    (uintptr_t)v->mem,
    (uint32_t)v->capacity,
    (uint32_t)v->size);
  printf("v = { ");
  float *d = vec_data(float, v);
  for (size_t i = 0; i < vec_size(float, v); ++i) {
    printf(i != vec_size(float, v) - 1 ? "%.3f, " : "%.3f", d[i]);
  }
  printf(" }\n");
}

int main(int argc, char *argv[])
{
  vec v = { 0 };

  float a[] = { 2, 3, 4, 5, 6, 7 };
  vec_append(float, &v, a, 6);
  print_vecf(&v);


  vec_free(&v);

  return 0;
}


/**
 * - The first call to tphvec_reserve() determines which allocator the
 *   vector uses for its entire life-span.
 *
 * - TODO : tphvec_set_allocator(), must be called before any function that
 *          potentially allocs/reallocs the vector, e.g. tphvec_reserve(),
 *          tphvec_append(), tphvec_resize(), etc.
 *          This will allocate the header (using the custom allocator), and
 *          set size = capacity = 0. The provided allocator is stored in the
 *          header.
 *          A call to tphvec_set_allocator(void** vec_ptr, tphvec_allocator_t *allocator)
 *          must assume that *vec_ptr == null, otherwise the call is a no-op. (A more
 *          sophisticated version could first allocate space using the custom allocator and
 *          memcpy the contents, then free the current vector using the old allocator. This
 *          would, however, cause a memory spike, since the vector needs to be duplicated
 *          in memory during this operation.)
 *
 * - [MACRO] tphvec(type) -> type *
 * - [???]   tphvec_assign()
 * -         tphvec_get_allocator()
 * - [???]   tphvec_front()
 * - [???]   tphvec_back()
 * -         tphvec_empty()
 * -         tphvec_size()
 * -         tphvec_capacity()
 * -         tphvec_reserve()
 * -         tphvec_shrink_to_fit()
 * -         tphvec_clear()
 * -         tphvec_append()
 * -         tphvec_resize()
 * -         tphvec_unordered_erase()
 * - [???]   tphvec_pop_back()
 * -         tphvec_free()
 */

#include <stddef.h> /* size_t */
#include <stdint.h> /* SIZE_MAX */
#include <stdlib.h> /* malloc, realloc, free */
#include <string.h> /* memcpy, memmove */

#include <inttypes.h>
#include <stdio.h>

#if 0
/* in case C library malloc() needs extra protection,
 * allow these defines to be overridden.
 */
#ifndef cvector_clib_free
#include <stdlib.h> /* for free */
#define cvector_clib_free free
#endif
#ifndef cvector_clib_malloc
#include <stdlib.h> /* for malloc */
#define cvector_clib_malloc malloc
#endif
#ifndef cvector_clib_calloc
#include <stdlib.h> /* for calloc */
#define cvector_clib_calloc calloc
#endif
#ifndef cvector_clib_realloc
#include <stdlib.h> /* for realloc */
#define cvector_clib_realloc realloc
#endif
#ifndef cvector_clib_assert
#include <assert.h> /* for assert */
#define cvector_clib_assert assert
#endif
#ifndef cvector_clib_memcpy
#include <string.h> /* for memcpy */
#define cvector_clib_memcpy memcpy
#endif
#ifndef cvector_clib_memmove
#include <string.h> /* for memmove */
#define cvector_clib_memmove memmove
#endif
#endif /*0*/

typedef void *(*tphvec_alloc_fn)(void *user_ctx, size_t size);
typedef void *(*tphvec_realloc_fn)(void *user_ctx, void *p, size_t size);
typedef void (*tphvec_free_fn)(void *user_ctx, void *p);

/* clang-format off */
typedef struct tphvec_allocator_t_ {
    void            *mem_ctx;
    tphvec_alloc_fn   alloc;
    tphvec_realloc_fn realloc;
    tphvec_free_fn    free;
} tphvec_allocator_t;

typedef struct tphvec_header_t_ {
    size_t              capacity;
    size_t              size;
    tphvec_allocator_t *allocator;
} tphvec_header_t;

#define TPHVEC_SUCCESS       0
#define TPHVEC_BAD_ALLOC     1
#define TPHVEC_BAD_ALLOCATOR 2
#define TPHVEC_INVALID_ARG   3
/* clang-format on */


#ifndef tphvec_assert
#include <assert.h>
#define tphvec_assert(_X_) assert((_X_))
#endif

/**
 * @brief tphvec - The vector type used in this library.
 *
 * @param type - The type of elements in the vector, e.g. <int>, <float>.
 */
#define tphvec(type) type *

/**
 * @brief _tphvec_vec_to_hdr - For internal use, converts a vector to a
 * header pointer. Assumes that the vector is not NULL.
 * @param vec - A vector.
 * @return A header pointer.
 * @internal
 */
static inline tphvec_header_t *_tphvec_vec_to_hdr(const void *vec)
{
  tphvec_assert(vec);
  return &((tphvec_header_t *)vec)[-1];
}

/**
 * @brief _tphvec_hdr_to_vec - For internal use, converts a header pointer to
 * a vector. Assumes that the header pointer is not NULL.
 * @param hdr - A pointer to a header.
 * @return The vector as a <void*>.
 * @internal
 */
static inline void *_tphvec_hdr_to_vec(const tphvec_header_t *hdr)
{
  tphvec_assert(hdr);
  return (void *)&(hdr[1]);
}

static inline size_t _tphvec_next_capacity(size_t cap, size_t req_cap)
{
  cap = cap ? cap : (size_t)1;
  while (cap < req_cap) { cap = cap <= (SIZE_MAX >> 1) ? cap << 1 : SIZE_MAX; }
  return cap;
}


/**
 * @brief _tphvec_capacity - Returns the number of bytes that the container has currently allocated
 * space for.
 * @param vec - A vector.
 * @return Capacity of the currently allocated storage in bytes.
 */
static inline size_t _tphvec_capacity(const void *vec)
{
  return vec ? _tphvec_vec_to_hdr(vec)->capacity : (size_t)0;
}

/**
 * @brief tphvec_capacity - Returns the number of elements that the container has currently
 * allocated space for.
 * @param vec - A vector.
 * @return Capacity of the currently allocated storage as a <size_t>.
 */
#define tphvec_capacity(vec) (_tphvec_capacity((vec)) / sizeof(*(vec)))


/**
 * @brief _tphvec_size - Returns the number of bytes in the container.
 * @param vec - A vector.
 * @return The number of bytes in the container.
 */
static inline size_t _tphvec_size(const void *vec)
{
  return vec ? _tphvec_vec_to_hdr(vec)->size : (size_t)0;
}

/**
 * @brief tphvec_size - Returns the number of elements in the container.
 * @param vec - A vector.
 * @return The number of bytes in the container.
 */
#define tphvec_size(vec) (_tphvec_size((vec)) / sizeof(*(vec)))


/**
 * @brief tphvec_get_allocator - Returns the allocator associated with the container.
 * @param vec - A vector.
 * @return The associated allocator, or NULL if none has been set.
 */
static inline tphvec_allocator_t *tphvec_get_allocator(const void *vec)
{
  return vec ? _tphvec_vec_to_hdr(vec)->allocator : (tphvec_allocator_t *)NULL;
}


/**
 * @brief tphvec_empty - Returns non-zero if the vector is empty.
 * @param vec - A vector.
 * @return Non-zero if empty, zero if non-empty.
 */
static inline int tphvec_empty(const void *vec) { return _tphvec_size(vec) == 0; }


/**
 * @brief tphvec_reserve - Requests that the vector capacity be at least
 * enough to contain <cap> elements. If <cap> is greater than the current vector
 * capacity, the function causes the container to reallocate its storage
 * increasing its capacity to <cap> (or greater).
 * @param vec     - A vector.
 * @param new_cap - New capacity of the vector, in bytes.
 * @param allocator - Custom allocator, if NULL malloc/realloc are used.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
int _tphvec_reserve(void **vec, size_t new_cap, tphvec_allocator_t *allocator)
{
  if (*vec) {
    /* Existing vector. */
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec);
    if (hdr->capacity >= new_cap) { return TPHVEC_SUCCESS; }
    tphvec_allocator_t *a = hdr->allocator;
    tphvec_header_t *new_hdr =
      (tphvec_header_t *)(a ? a->realloc(a->mem_ctx, hdr, sizeof(tphvec_header_t) + new_cap)
                            : realloc(hdr, sizeof(tphvec_header_t) + new_cap));
    if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
    new_hdr->capacity = new_cap;
    *vec = _tphvec_hdr_to_vec(new_hdr);
  } else {
    /* Initialize new vector. */
    if (allocator && !(allocator->alloc && allocator->realloc && allocator->free)) {
      return TPHVEC_BAD_ALLOCATOR;
    }
    tphvec_header_t *hdr =
      (tphvec_header_t *)(allocator ? allocator->alloc(
                                        allocator->mem_ctx, sizeof(tphvec_header_t) + new_cap)
                                    : malloc(sizeof(tphvec_header_t) + new_cap));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }
    hdr->capacity = new_cap;
    hdr->size = 0;
    hdr->allocator = allocator;
    *vec = _tphvec_hdr_to_vec(hdr);
  }
  return TPHVEC_SUCCESS;
}

/**
 * @brief tphvec_reserve - Convenience macro.
 * @param vec       - A vector.
 * @param new_cap   - Minimum number of elements to reserve memory for.
 * @param allocator - Custom allocator, if NULL malloc/realloc are used.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
#define tphvec_reserve(vec, new_cap, allocator) \
  _tphvec_reserve((void **)&(vec), (new_cap) * sizeof(*(vec)), (allocator))


#if 0
/**
 * @brief tphvec_erase - removes the element at index i from the vector.
 * @param vec - The vector.
 * @param i - Index of element to remove.
 * @return void
 */
#define tph_vector_erase(vec, i)                                                              \
  do {                                                                                        \
    if ((vec)) {                                                                              \
      const size_t cv_sz__ = tphvec_size((vec));                                              \
      if ((i) < cv_sz__) {                                                                    \
        /*tph_vector_elem_destructor_t elem_destructor__ = tph_vector_elem_destructor(vec);*/ \
        /*if (elem_destructor__) { elem_destructor__(&(vec)[i]); }*/                          \
        tphvec_set_size((vec), cv_sz__ - 1);                                                  \
        memmove((vec) + (i), (vec) + (i) + 1, sizeof(*(vec)) * (cv_sz__ - 1 - (i)));          \
      }                                                                                       \
    }                                                                                         \
  } while (0)
#endif


/**
 * @brief tphvec_clear - Erase all the elements in the vector, capacity is unchanged.
 * @param vec - The vector.
 * @return void
 */
static inline void tphvec_clear(void *vec)
{
  if (vec) { _tphvec_vec_to_hdr(vec)->size = 0; }
}


/**
 * @brief tphvec_free - Frees all memory associated with the vector.
 * @param vec - A vector.
 * @return void
 */
static inline void tphvec_free(void *vec)
{
  if (vec) {
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(vec);
    tphvec_allocator_t *a = hdr->allocator;
    if (a) {
      a->free(a->mem_ctx, (void *)hdr);
    } else {
      free((void *)hdr);
    }
  }
}


/**
 * @brief _tphvec_append - Add elements to the end of the vector.
 * @param vec_ptr - Pointer to a vector.
 * @param values  - Pointer to values to be added.
 * @param nbytes  - Number of bytes to copy from <values> to vector.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
int _tphvec_append(void **vec_ptr, void *values, size_t nbytes)
{
  if (*vec_ptr) {
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    const size_t req_cap = hdr->size + nbytes;
    if (hdr->capacity < req_cap) {
      const size_t new_cap = _tphvec_next_capacity(hdr->capacity, req_cap);
      tphvec_allocator_t *a = hdr->allocator;
      tphvec_header_t *new_hdr =
        (tphvec_header_t *)(a ? a->realloc(a->mem_ctx, hdr, sizeof(tphvec_header_t) + new_cap)
                              : realloc(hdr, sizeof(tphvec_header_t) + new_cap));
      if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
      hdr = new_hdr;
      hdr->capacity = new_cap;
      *vec_ptr = _tphvec_hdr_to_vec(hdr);
    }
    memcpy(*vec_ptr + hdr->size, values, nbytes);
    hdr->size += nbytes;
  } else {
    tphvec_header_t *hdr = (tphvec_header_t *)(malloc(sizeof(tphvec_header_t) + nbytes));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }
    hdr->capacity = nbytes;
    hdr->size = nbytes;
    *vec_ptr = _tphvec_hdr_to_vec(hdr);
    memcpy(*vec_ptr, values, nbytes);
  }
  return TPHVEC_SUCCESS;
}

#define tphvec_append(vec, values, count) \
  _tphvec_append((void **)&(vec), (values), (count) * sizeof(*(values)))


#if 0
/**
 * @brief tphvec_pop_back - removes the last element from the vector
 * @param vec - the vector
 * @return void
 */
#define tphvec_pop_back(vec)                                                         \
  do {                                                                               \
    /*cvector_elem_destructor_t elem_destructor__ = cvector_elem_destructor(vec);*/  \
    /*if (elem_destructor__) { elem_destructor__(&(vec)[cvector_size(vec) - 1]); }*/ \
    tphvec_set_size((vec), tphvec_size(vec) - 1);                                    \
  } while (0)
#endif

/**
 * @brief _tphvec_shrink_to_fit - Requests the removal of unused capacity.
 * @param vec_ptr - Pointer to a vector.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
int _tphvec_shrink_to_fit(void **vec_ptr)
{
  if (*vec_ptr) {
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    tphvec_assert(hdr->size <= hdr->capacity);
    if (hdr->size == hdr->capacity) { return TPHVEC_SUCCESS; }
    tphvec_allocator_t *a = hdr->allocator;
    tphvec_header_t *new_hdr =
      (tphvec_header_t *)(a ? a->realloc(a->mem_ctx, hdr, sizeof(tphvec_header_t) + hdr->size)
                            : realloc(hdr, sizeof(tphvec_header_t) + hdr->size));
    if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
    new_hdr->capacity = new_hdr->size;
    *vec_ptr = _tphvec_hdr_to_vec(new_hdr);
  }
  tphvec_assert(_tphvec_capacity(*vec_ptr) == _tphvec_size(*vec_ptr));
  return TPHVEC_SUCCESS;
}

/**
 * @brief tphvec_shrink_to_fit - Requests the removal of unused capacity.
 * @param vec - A vector.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
#define tphvec_shrink_to_fit(vec) _tphvec_shrink_to_fit((void **)&(vec))


/**
 * @brief _tphvec_resize - Changes the number of elements stored.
 * @param vec_ptr - Pointer to a vector.
 * @param count   - New size of the vector in bytes.
 * @param value   - The value to initialize the new elements with.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
int _tphvec_resize(void **vec_ptr, size_t count, void *value, size_t sizeof_value)
{
  if (*vec_ptr) {
    /* Existing vector. */
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    if (count < hdr->size) {
      /* Shrink vector, no new elements added! */
      hdr->size = count;
    } else if (count > hdr->size) {
      if (hdr->capacity < count) {
        /* Grow capacity. */
        tphvec_allocator_t *a = hdr->allocator;
        tphvec_header_t *new_hdr =
          (tphvec_header_t *)(a ? a->realloc(a->mem_ctx, hdr, sizeof(tphvec_header_t) + count)
                                : realloc(hdr, sizeof(tphvec_header_t) + count));
        if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
        hdr = new_hdr;
        hdr->capacity = count;
      }
      /* Initialize the first new element of the vector by copying the
         provided value. The following elements are copied in increasingly
         large chunks in order to minimize the number of function calls. */
      memcpy(*vec_ptr + hdr->size, value, sizeof_value);
      void* src = *vec_ptr + hdr->size;
      hdr->size += sizeof_value;
      size_t n = sizeof_value;
      printf("count = %zu\n", count);
      while (hdr->size < count) {
        printf("w = %zu, n = %zu\n", hdr->size, n);
        memcpy(*vec_ptr + hdr->size, src, n);
        hdr->size += n;
        n = ((hdr->size << 1) > count ? count - hdr->size : hdr->size);
        /* CHECK OVERFLOW??? */
      }
    }
    /* else: hdr->size == count, nothing to do! */
  } else {
    /* Initialize a new vector. */
    tphvec_header_t *hdr = (tphvec_header_t *)(malloc(sizeof(tphvec_header_t) + count));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }
    hdr->capacity = count;
    hdr->allocator = NULL;
    *vec_ptr = _tphvec_hdr_to_vec(hdr);
    if (count < sizeof_value) {
      /* Not enough capacity to initialize an element. */
      hdr->size = 0;
    } else {
      /* Initialize the first element of the vector by copying the
         provided value. The following elements are copied in increasingly
         large chunks in order to minimize the number of function calls. */
      memcpy(*vec_ptr, value, sizeof_value);
      hdr->size = sizeof_value;
      size_t n = sizeof_value;
      printf("count = %zu\n", count);
      while (hdr->size < count) {
        printf("w = %zu, n = %zu\n", hdr->size, n);
        memcpy(*vec_ptr + hdr->size, *vec_ptr, n);
        hdr->size += n;
        n = ((hdr->size << 1) > count ? count - hdr->size : hdr->size);
        /* CHECK OVERFLOW??? */
      }
    }
  }
  return TPHVEC_SUCCESS;
}

#define tphvec_resize(vec, count, value) \
  (_tphvec_resize((void **)&(vec), (count) * sizeof(*(vec)), &(value), sizeof((value))))

#if 0
#define tphvec_resize(vec, new_size, value)                                  \
  do {                                                                       \
    const size_t tphvec_resize__new_size = (size_t)(new_size);               \
    tphvec_header_t *tphvec_resize__b = NULL;                                \
    size_t tphvec_resize__size = 0;                                          \
    if ((vec)) {                                                             \
      tphvec_resize__b = _tphvec_vec_to_base((vec));                         \
      tphvec_resize__size = tphvec_resize__b->size;                          \
      if (tphvec_resize__new_size > tphvec_resize__size) {                   \
        tphvec_reserve((vec), tphvec_resize__new_size);                      \
        do {                                                                 \
          (vec)[tphvec_resize__size++] = (value);                            \
        } while (tphvec_resize__size < tphvec_resize__new_size);             \
      }                                                                      \
      tphvec_resize__b->size = tphvec_resize__new_size;                      \
    } else {                                                                 \
      /* Initialize a new vector. */                                         \
      tphvec_resize__b = (tphvec_header_t *)malloc(                          \
        sizeof(tphvec_header_t) + tphvec_resize__new_size * sizeof(*(vec))); \
      tphvec_assert(tphvec_resize__b);                                       \
      tphvec_resize__b->size = tphvec_resize__new_size;                      \
      tphvec_resize__b->capacity = tphvec_resize__new_size;                  \
      tphvec_resize__b->allocator = NULL;                                    \
      (vec) = _tphvec_base_to_vec(tphvec_resize__b);                         \
      do {                                                                   \
        (vec)[tphvec_resize__size++] = (value);                              \
      } while (tphvec_resize__size < tphvec_resize__new_size);               \
    }                                                                        \
  } while (0)
#endif

/* -------------------------- */

static void *tph_alloc_fn(void *mem_ctx, size_t n)
{
  (void)mem_ctx;
  return malloc(n);
}

static void *tph_realloc_fn(void *mem_ctx, void *p, size_t n)
{
  (void)mem_ctx;
  return realloc(p, n);
}

static void tph_free_fn(void *mem_ctx, void *p)
{
  (void)mem_ctx;
  free(p);
}

static void print_vecf(float *vec)
{
  printf("cap = %" PRIu32 "\nsize = %" PRIu32 "\nalloc = %p\n",
    (uint32_t)tphvec_capacity(vec),
    (uint32_t)tphvec_size(vec),
    (void *)tphvec_get_allocator(vec));
  const size_t sz = tphvec_size(vec);
  printf("v = { ");
  for (size_t i = 0; i < tphvec_size(vec); ++i) {
    printf(i != tphvec_size(vec) - 1 ? "%.3f, " : "%.3f", vec[i]);
  }
  printf(" }\n");
}

int main(int argc, char *argv[])
{
  printf("cap: %zu, next_cap: %zu\n", 0, _tphvec_next_capacity(0, 0));
  printf("cap: %zu, next_cap: %zu\n", 5, _tphvec_next_capacity(5, 13));
  printf("cap: %zu, next_cap: %zu\n", 5, _tphvec_next_capacity(5, SIZE_MAX - 10));

  tphvec(float) v = NULL;
  tphvec_reserve(v, 10, NULL);
  print_vecf(v);
  printf("\n");

  float vals[] = { 41.F, 42.F, 43.F };
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  print_vecf(v);
  printf("\n");

  tphvec_shrink_to_fit(v);
  print_vecf(v);
  printf("\n");

  tphvec_free(v);

  {
    tphvec(float) v2 = NULL;
    float value = 333.F;
    tphvec_resize(v2, 13, value);
    print_vecf(v2);
    printf("\n");
  }
  {
    tphvec(float) v2 = NULL;
    float value = 333.F;
    tphvec_resize(v2, 1, value);
    print_vecf(v2);
    printf("\n");
    float value2 = 44.F;
    tphvec_resize(v2, 4, value2);
    print_vecf(v2);
    printf("\n");
  }
  {
    tphvec(float) v2 = NULL;
    float value = 333.F;
    tphvec_resize(v2, 0, value);
    print_vecf(v2);
    printf("\n");
  }


#if 0
  float *v2 = NULL;
  tphvec_resize(v2, 16, 42.F);
  tphvec_free(v2);
  print_vecf(v2);
  printf("\n");
#endif
  return 0;
}
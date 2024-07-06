
/* -------------------------- */
/*
 * Copyright (c) 2015 Evan Teran
 *
 * License: The MIT License (MIT)
 */

/* cvector heap implemented using C library malloc() */

#include <stddef.h> /* size_t */
#include <stdlib.h> /* malloc, realloc, free */
#include <string.h> /* memcpy, memmove */

#include <inttypes.h>
#include <stdint.h>
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

typedef void *(*tphvec_alloc_t)(void *user_ctx, size_t size);
typedef void *(*tphvec_realloc_t)(void *user_ctx, void *p, size_t size);
typedef void (*tphvec_free_t)(void *user_ctx, void *p);

/* clang-format off */
typedef struct tphvec_allocator_t_ {
    void            *mem_ctx;
    tphvec_alloc_t   alloc;
    tphvec_realloc_t realloc;
    tphvec_free_t    free;
} tphvec_allocator_t;

typedef struct tphvec_metadata_t_ {
    size_t              size;
    size_t              capacity;
    tphvec_allocator_t *allocator;
} tphvec_metadata_t;
/* clang-format on */

#ifndef tphvec_assert
#include <assert.h>
#define tphvec_assert(_X_) assert((_X_))
#endif

/**
 * @brief tphvec_decl - The vector type used in this library.
 *
 * @param type - The type of vector to act on, e.g. <int>, <float>.
 */
#define tphvec_decl(type) type *

/**
 * @brief _tphvec_vec_to_base - For internal use, converts a vector pointer to a
 * metadata pointer.
 * @param vec - The vector.
 * @return The metadata pointer of the vector.
 * @internal
 */
#define _tphvec_vec_to_base(vec) (&((tphvec_metadata_t *)(vec))[-1])

/**
 * @brief _tphvec_base_to_vec - For internal use, converts a metadata pointer to
 * a vector pointer.
 * @param ptr - Pointer to the metadata.
 * @return The vector.
 * @internal
 */
#define _tphvec_base_to_vec(ptr) ((void *)&((tphvec_metadata_t *)(ptr))[1])

/**
 * @brief tphvec_capacity - Gets the current capacity of the vector.
 * @param vec - The vector.
 * @return Vector capacity as <size_t>.
 */
#define tphvec_capacity(vec) ((vec) ? _tphvec_vec_to_base((vec))->capacity : (size_t)0)

/**
 * @brief tphvec_size - Gets the current size of the vector.
 * @param vec - The vector.
 * @return Vector size as <size_t>.
 */
#define tphvec_size(vec) ((vec) ? _tphvec_vec_to_base((vec))->size : (size_t)0)

/**
 * @brief tphvec_allocator - Gets the current allocator.
 * @param vec - The vector.
 * @return Vector allocator as <tphvec_allocator_t*>, or NULL.
 */
#define tphvec_allocator(vec) ((vec) ? _tphvec_vec_to_base((vec))->allocator : NULL)

/**
 * @brief tphvec_empty - Returns non-zero if the vector is empty.
 * @param vec - The vector.
 * @return non-zero if empty, zero if non-empty.
 */
#define tphvec_empty(vec) (tphvec_size((vec)) == 0)

/**
 * @brief tphvec_init - Initialize a vector. The vector must be NULL for this to do anything.
 * @param vec       - The vector.
 * @param cap       - Capacity to reserve.
 * @param allocator - Allocator.
 * @return void
 */
#define tphvec_init(vec, cap, _allocator)                                                       \
  do {                                                                                          \
    if (!(vec)) {                                                                               \
      const size_t init_sz__ = sizeof(tphvec_metadata_t) + (size_t)(cap) * sizeof(*(vec));      \
      tphvec_metadata_t *init_b__ =                                                             \
        (tphvec_metadata_t *)((_allocator) && ((tphvec_allocator_t *)(_allocator))->alloc       \
                                ? ((tphvec_allocator_t *)(_allocator))                          \
                                    ->alloc(                                                    \
                                      ((tphvec_allocator_t *)(_allocator))->mem_ctx, init_sz__) \
                                : malloc(init_sz__));                                           \
      tphvec_assert(init_b__);                                                                  \
      init_b__->allocator = (_allocator);                                                       \
      (vec) = _tphvec_base_to_vec(init_b__);                                                    \
      _tphvec_set_capacity((vec), (size_t)(cap));                                               \
      _tphvec_set_size((vec), 0);                                                               \
    }                                                                                           \
  } while (0)

/**
 * @brief tphvec_reserve - Requests that the vector capacity be at least
 * enough to contain n elements. If n is greater than the current vector
 * capacity, the function causes the container to reallocate its storage
 * increasing its capacity to n (or greater).
 * @param vec - The vector.
 * @param n - Minimum capacity for the vector.
 * @return void
 */
#define tphvec_reserve(vec, n)                                      \
  do {                                                              \
    if (tphvec_capacity((vec)) < (n)) { _tphvec_grow((vec), (n)); } \
    tphvec_assert(tphvec_capacity((vec)) >= (n));                   \
  } while (0)

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
 * @brief tphvec_clear - Erase all of the elements in the vector, capacity is unchanged.
 * @param vec - The vector.
 * @return void
 */
#define tphvec_clear(vec)                  \
  do {                                     \
    if (vec) { _tphvec_set_size(vec, 0); } \
  } while (0)

/**
 * @brief tphvec_free - Frees all memory associated with the vector.
 * @param vec - The vector.
 * @return void
 */
#define tphvec_free(vec)                                   \
  do {                                                     \
    if ((vec)) {                                           \
      tphvec_metadata_t *b__ = _tphvec_vec_to_base((vec)); \
      tphvec_allocator_t *a__ = b__->allocator;            \
      if (a__ && a__->free)                                \
        a__->free(a__->mem_ctx, (void *)b__);              \
      else                                                 \
        free(b__);                                         \
    }                                                      \
  } while (0)

/**
 * @brief tphvec_next_grow - Returns the computed size of next
 * vector grow. Size is increased by multiplication of 2.
 * @param size - Current size.
 * @return Size after next vector grow.
 * @internal
 */
#define _tphvec_next_grow(size) ((size) ? ((size) << 1) : 1)

/**
 * @brief tphvec_push_back - adds an element to the end of the vector.
 * @param vec   - the vector.
 * @param value - the value to add.
 * @return void
 */
#define tphvec_push_back(vec, value)                                                    \
  do {                                                                                  \
    const size_t cap__ = tphvec_capacity((vec));                                        \
    if (cap__ <= tphvec_size((vec))) { _tphvec_grow((vec), _tphvec_next_grow(cap__)); } \
    (vec)[tphvec_size((vec))] = (value);                                                \
    _tphvec_set_size((vec), tphvec_size((vec)) + 1);                                    \
  } while (0)

/**
 * @brief tphvec_push_back - adds <n> elements to the end of the vector.
 * @param vec    - The vector.
 * @param values - The values to add.
 * @param n      - The number of elements to add.
 * @return void
 */
#define tphvec_push_back_range(vec, values, n)                         \
  do {                                                                 \
    while (tphvec_capacity((vec)) < tphvec_size((vec)) + (n)) {        \
      _tphvec_grow((vec), _tphvec_next_grow(tphvec_capacity((vec))));  \
    }                                                                  \
    memcpy(&(vec)[tphvec_size((vec))], (values), (n) * sizeof *(vec)); \
    tphvec_set_size((vec), tphvec_size(vec) + (n));                    \
  } while (0)

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
 * @brief _tphvec_set_capacity - For internal use, sets the allocator variable
 * of the vector.
 * @param vec       - The vector.
 * @param allocator - The new allocator <tphvec_allocator_t*> to set.
 * @return void
 * @internal
 */
#define _tphvec_set_allocator(vec, new_allocator)             \
  do {                                                        \
    if ((vec)) {                                              \
      /* TODO: MUST FREE EXISTING ALLOCATIONS, IF ANY!!*/     \
      tphvec_vec_to_base((vec))->allocator = (new_allocator); \
    }                                                         \
  } while (0)

/**
 * @brief _tphvec_set_capacity - For internal use, sets the capacity of the vector.
 * @param vec     - The vector.
 * @param new_cap - The new capacity <size_t> to set.
 * @return void
 * @internal
 */
#define _tphvec_set_capacity(vec, new_cap)                           \
  do {                                                               \
    if ((vec)) { _tphvec_vec_to_base((vec))->capacity = (new_cap); } \
  } while (0)

/**
 * @brief _tphvec_set_size - For internal use, sets the size of the vector.
 * @param vec      - The vector.
 * @param new_size - The new size <size_t> to set.
 * @return void
 * @internal
 */
#define _tphvec_set_size(vec, new_size)                           \
  do {                                                            \
    if ((vec)) { _tphvec_vec_to_base((vec))->size = (new_size); } \
  } while (0)

#if 0
/**
 * @brief cvector_set_elem_destructor - set the element destructor function
 * used to clean up removed elements. The vector must NOT be NULL for this to do
 * anything.
 * @param vec - the vector
 * @param elem_destructor_fn - function pointer of type
 * cvector_elem_destructor_t used to destroy elements
 * @return void
 */
#define cvector_set_elem_destructor(vec, elem_destructor_fn)                       \
  do {                                                                             \
    if (vec) { cvector_vec_to_base(vec)->elem_destructor = (elem_destructor_fn); } \
  } while (0)
#endif

/**
 * @brief _tphvec_grow - For internal use, ensures that the vector is at least
 * <count> elements big
 * @param vec   - The vector.
 * @param count - The new capacity <size_t> to set.
 * @return void
 * @internal
 */
#define _tphvec_grow(vec, count)                                                                 \
  do {                                                                                           \
    const size_t cv_sz__ = sizeof(tphvec_metadata_t) + (count) * sizeof(*(vec));                 \
    if ((vec)) {                                                                                 \
      /* Re-allocate existing vector. */                                                         \
      tphvec_metadata_t *b1__ = _tphvec_vec_to_base((vec));                                      \
      void *b2__ = b1__->allocator                                                               \
                     ? b1__->allocator->realloc(b1__->allocator->mem_ctx, (void *)b1__, cv_sz__) \
                     : realloc(b1__, cv_sz__);                                                   \
      tphvec_assert(b2__);                                                                       \
      (vec) = _tphvec_base_to_vec(b2__);                                                         \
    } else {                                                                                     \
      /* Initialize a new vector. */                                                             \
      void *cv_p__ = malloc(cv_sz__);                                                            \
      assert(cv_p__);                                                                            \
      (vec) = _tphvec_base_to_vec(cv_p__);                                                       \
      _tphvec_set_size((vec), 0);                                                                \
      /*tphvec_set_alloc_fn(vec, NULL);*/                                                        \
      /*tphvec_set_realloc_fn(vec, NULL);*/                                                      \
      /*tphvec_set_free_fn(vec, NULL); */                                                        \
    }                                                                                            \
    _tphvec_set_capacity((vec), (count));                                                        \
  } while (0)

/**
 * @brief tphvec_shrink_to_fit - requests the container to reduce its capacity
 * to fit its size.
 * @param vec - The vector.
 * @return void
 */
#define tphvec_shrink_to_fit(vec)               \
  do {                                          \
    if ((vec)) {                                \
      const size_t cv_sz___ = tphvec_size(vec); \
      _tphvec_grow((vec), cv_sz___);            \
    }                                           \
  } while (0)

/**
 * @brief tphvec_resize - Resizes the container to contain count elements.
 * @param vec      - The vector.
 * @param new_size - New size <size_t> of the vector.
 * @param value    - The value to initialize new elements with.
 * @return void
 */
#define tphvec_resize(vec, new_size, value)           \
  do {                                                \
    if ((vec)) {                                      \
      const size_t new_size__ = (size_t)(new_size);   \
      size_t size__ = _tphvec_vec_to_base(vec)->size; \
      if (new_size__ > size__) {                      \
        tphvec_reserve((vec), new_size__);            \
        do {                                          \
          (vec)[size__++] = (value);                  \
        } while (size__ < new_size__);                \
      }                                               \
      _tphvec_set_size((vec), new_size__);            \
    }                                                 \
  } while (0) /* -------------------------- */

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

int main(int argc, char *argv[])
{
  tphvec_decl(int) v = NULL;
  tphvec_init(v, 10, NULL);


  printf("size = %" PRIu32 "\ncap = %" PRIu32 "\n",
    (uint32_t)tphvec_size(v),
    (uint32_t)tphvec_capacity(v));

  tphvec_push_back(v, 42);
  printf("size = %zu\ncap = %zu\n", tphvec_size(v), tphvec_capacity(v));

  tphvec_resize(v, 192, 42);
  printf("size = %zu\ncap = %zu\n", tphvec_size(v), tphvec_capacity(v));

  tphvec_free(v);

  return 0;
}
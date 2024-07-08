
/* -------------------------- */
/*
 * Copyright (c) 2015 Evan Teran
 *
 * License: The MIT License (MIT)
 */

/* cvector heap implemented using C library malloc() */

/**
 * - The first call to tphvec_reserve() determines which allocator the
 *   vector uses for its entire life-span.
 */

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

typedef struct tphvec_header_t_ {
    size_t              size;
    size_t              capacity;
    tphvec_allocator_t *allocator;
} tphvec_header_t;
/* clang-format on */

#define TPHVEC_SUCCESS 0
#define TPHVEC_BAD_ALLOC 1
#define TPHVEC_BAD_ALLOCATOR 2


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
 * @brief _tphvec_vec_to_base - For internal use, converts a vector pointer to a
 * metadata pointer. Assumes that the vector pointer is not NULL.
 * @param vec - The vector.
 * @return The metadata pointer of the vector as a <tphvec_header_t*>.
 * @internal
 */
// #define _tphvec_vec_to_base(vec) (&((tphvec_header_t *)(vec))[-1])
tphvec_header_t *_tphvec_vec_to_hdr(void *vec) { return &((tphvec_header_t *)vec)[-1]; }

/**
 * @brief _tphvec_base_to_vec - For internal use, converts a metadata pointer to
 * a vector pointer. Assumes that the metadata pointer is not NULL.
 * @param ptr - Pointer to the metadata.
 * @return The vector as a <void*>.
 * @internal
 */
// #define _tphvec_base_to_vec(ptr) ((void *)&((tphvec_header_t *)(ptr))[1])
/*inline*/ void *_tphvec_hdr_to_vec(void *ptr) { return (void *)&((tphvec_header_t *)(ptr))[1]; }

/**
 * @brief tphvec_capacity - Gets the current capacity of the vector in bytes.
 * @param vec - The vector.
 * @return Vector capacity in bytes as <size_t>.
 */
static inline size_t _tphvec_capacity(void *vec)
{
  return vec ? _tphvec_vec_to_hdr(vec)->capacity : (size_t)0;
}

#define tphvec_capacity(vec) (_tphvec_capacity((vec)) / sizeof(*(vec)))


/**
 * @brief tphvec_size - Gets the current size of the vector in bytes.
 * @param vec - The vector.
 * @return Vector size in bytes as <size_t>.
 */
static inline size_t _tphvec_size(void *vec)
{
  return vec ? _tphvec_vec_to_hdr(vec)->size : (size_t)0;
}

#define tphvec_size(vec) (_tphvec_size((vec)) / sizeof(*(vec)))

/**
 * @brief tphvec_allocator - Gets the current allocator.
 * @param vec - The vector.
 * @return Vector allocator as <tphvec_allocator_t*>, or NULL.
 */
// #define tphvec_allocator(vec) ((vec) ? _tphvec_vec_to_base((vec))->allocator : NULL)
tphvec_allocator_t *tphvec_allocator(void *vec)
{
  return vec ? _tphvec_vec_to_hdr(vec)->allocator : (tphvec_allocator_t *)NULL;
}


/**
 * @brief tphvec_empty - Returns non-zero if the vector is empty.
 * @param vec - The vector.
 * @return non-zero if empty, zero if non-empty.
 */
// #define tphvec_empty(vec) (tphvec_size((vec)) == 0)
int tphvec_empty(void *vec) { return tphvec_size(vec) == 0; }

#if 0
/**
 * @brief tphvec_init - Initialize a vector. The vector must be NULL for this to do anything.
 * @param vec        - The vector.
 * @param cap        - Capacity to reserve.
 * @param _allocator - Custom allocator <tphvec_allocator_t*>, if NULL use malloc.
 * @return void
 */
#if 0
#define tphvec_init(vec, cap, _allocator)                                                        \
  do {                                                                                           \
    if (!(vec)) {                                                                                \
      tphvec_header_t *tphvec_init__b = NULL;                                                    \
      if ((_allocator)) {                                                                        \
        tphvec_allocator_t *tphvec_init__a = (tphvec_allocator_t *)(_allocator);                 \
        tphvec_assert(tphvec_init__a->alloc && tphvec_init__a->realloc && tphvec_init__a->free); \
        tphvec_init__b = (tphvec_header_t *)tphvec_init__a->alloc(                               \
          tphvec_init__a->mem_ctx, sizeof(tphvec_header_t) + (size_t)(cap) * sizeof(*(vec)));    \
        tphvec_assert(tphvec_init__b);                                                           \
        tphvec_init__b->allocator = tphvec_init__a;                                              \
      } else {                                                                                   \
        tphvec_init__b =                                                                         \
          (tphvec_header_t *)malloc(sizeof(tphvec_header_t) + (size_t)(cap) * sizeof(*(vec)));   \
        tphvec_assert(tphvec_init__b);                                                           \
        tphvec_init__b->allocator = NULL;                                                        \
      }                                                                                          \
      tphvec_init__b->size = 0;                                                                  \
      tphvec_init__b->capacity = (size_t)(cap);                                                  \
      (vec) = _tphvec_base_to_vec(tphvec_init__b);                                               \
    }                                                                                            \
  } while (0)
#endif
int tphvec_init(void **vec, size_t cap, tphvec_allocator_t *allocator)
{
  if (*vec) {
    // BAD_INIT
    return 1;
  }

  tphvec_assert(
    !allocator || (allocator && allocator->alloc && allocator->realloc && allocator->free));
  tphvec_header_t *base = (tphvec_header_t *)(allocator ? allocator->alloc(allocator->mem_ctx,
                                                            sizeof(tphvec_header_t) + cap)
                                                        : malloc(sizeof(tphvec_header_t) + cap));
  if (!base) {
    return 2;// BAD_ALLOC
  }
  base->size = 0;
  base->capacity = cap;
  base->allocator = allocator;
  *vec = _tphvec_hdr_to_vec(base);
}
#endif

/**
 * @brief tphvec_reserve - Requests that the vector capacity be at least
 * enough to contain <cap> elements. If <cap> is greater than the current vector
 * capacity, the function causes the container to reallocate its storage
 * increasing its capacity to <cap> (or greater).
 * @param vec - The vector.
 * @param cap - New minimum capacity for the vector.
 * @return void
 */
#if 0
#define tphvec_reserve(vec, cap)                                                         \
  do {                                                                                   \
    if (tphvec_capacity((vec)) < (size_t)(cap)) { _tphvec_grow_capacity((vec), (cap)); } \
    tphvec_assert(tphvec_capacity((vec)) >= (size_t)(cap));                              \
  } while (0)
#endif
int _tphvec_reserve(void **vec, size_t cap, tphvec_allocator_t *allocator)
{
  tphvec_header_t *hdr = NULL;
  if (*vec) {
    /* Existing vector. */
    hdr = _tphvec_vec_to_hdr(*vec);
    if (hdr->allocator != allocator) { return TPHVEC_BAD_ALLOCATOR; }
    if (hdr->capacity >= cap) { return TPHVEC_SUCCESS; }

    tphvec_header_t *new_hdr =
      (tphvec_header_t *)(allocator ? allocator->realloc(
                                        allocator->mem_ctx, hdr, sizeof(tphvec_header_t) + cap)
                                    : realloc(hdr, sizeof(tphvec_header_t) + cap));
    if (!new_hdr) { return TPHVEC_BAD_ALLOC; }

    new_hdr->capacity = cap;
    /*new_hdr->allocator = allocator;*/
    *vec = _tphvec_hdr_to_vec(new_hdr);
  } else {
    /* Initialize new vector. */
    if (allocator && !(allocator->alloc && allocator->realloc && allocator->free)) {
      return TPHVEC_BAD_ALLOCATOR;
    }

    hdr = (tphvec_header_t *)(allocator ? allocator->alloc(
                                            allocator->mem_ctx, sizeof(tphvec_header_t) + cap)
                                        : malloc(sizeof(tphvec_header_t) + cap));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }

    hdr->size = 0;
    hdr->capacity = cap;
    hdr->allocator = allocator;
    *vec = _tphvec_hdr_to_vec(hdr);
  }
  return 0; /* SUCCESS */
}

/**
 * @brief tphvec_reserve - Convenience macro.
 * @param vec       - The vector.
 * @param cap       - Minimum capacity to reserve.
 * @param allocator - Custom allocator.
 */
#define tphvec_reserve(vec, cap, allocator) \
  _tphvec_reserve((void **)&(vec), (cap) * sizeof(*(vec)), (allocator))

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
#if 0
#define tphvec_clear(vec)                                \
  do {                                                   \
    if ((vec)) { _tphvec_vec_to_base((vec))->size = 0; } \
  } while (0)
#endif
void tphvec_clear(void *vec)
{
  if (vec) { _tphvec_vec_to_hdr(vec)->size = 0; }
}

/**
 * @brief tphvec_free - Frees all memory associated with the vector.
 * @param vec - The vector.
 * @return void
 */
#if 0
#define tphvec_free(vec)                                                       \
  do {                                                                         \
    if ((vec)) {                                                               \
      tphvec_header_t *tphvec_free__b = _tphvec_vec_to_base((vec));            \
      tphvec_allocator_t *tphvec_free__a = tphvec_free__b->allocator;          \
      if (tphvec_free__a) {                                                    \
        tphvec_assert(tphvec_free__a->free);                                   \
        tphvec_free__a->free(tphvec_free__a->mem_ctx, (void *)tphvec_free__b); \
      } else {                                                                 \
        free((void *)tphvec_free__b);                                          \
      }                                                                        \
    }                                                                          \
  } while (0)
#endif
void tphvec_free(void *vec)
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

#if 0
/**
 * @brief tphvec_next_grow - Returns the computed size of next
 * vector grow. Size is increased by multiplication of 2.
 * Not checking for overflow here!
 * @param size - Current size.
 * @return Size after next vector grow as a <size_t>.
 * @internal
 */
#define _tphvec_next_grow(size) ((size) ? (size_t)((size) << 1) : (size_t)1)
#endif

/**
 * @brief tphvec_push_back - adds an element to the end of the vector.
 * @param vec   - the vector.
 * @param value - the value to add.
 * @return void
 */
int _tphvec_push_back(void **vec, void *values, size_t nbytes)
{
  if (*vec) {
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec);
    const size_t req_cap = hdr->size + nbytes;
    if (hdr->capacity < req_cap) {
      size_t new_cap = hdr->capacity ? hdr->capacity : (size_t)1;
      while (new_cap < req_cap) { new_cap <<= 1; }
      // CHECK new_cap OVERFLOW!!

      int ret = _tphvec_reserve(vec, new_cap, hdr->allocator);
      if (ret != 0) { return ret; }
      hdr = _tphvec_vec_to_hdr(*vec);
    }
    memcpy(*vec + hdr->size, values, nbytes);
    hdr->size += nbytes;
  } else {
  }
  return 0;
}

#define tphvec_push_back(vec, values, n) \
  _tphvec_push_back((void**)&(vec), (values), (n) * sizeof(*(values)))

#if 0
/**
 * @brief tphvec_push_back_range - adds <n> elements to the end of the vector.
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
#endif

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
 * @brief _tphvec_grow - For internal use, ensures that the vector capacity is at least <cap>.
 * @param vec - The vector.
 * @param cap - The new capacity <size_t> to set.
 * @return void
 * @internal
 */
#define _tphvec_grow_capacity(vec, cap)                                                           \
  do {                                                                                            \
    if ((vec)) {                                                                                  \
      /* Re-allocate existing vector. */                                                          \
      tphvec_header_t *_tphvec_grow_capacity__b1 = _tphvec_vec_to_base((vec));                    \
      tphvec_header_t *_tphvec_grow_capacity__b2 = NULL;                                          \
      if (_tphvec_grow_capacity__b1->allocator) {                                                 \
        tphvec_assert(_tphvec_grow_capacity__b1->allocator->alloc                                 \
                      && _tphvec_grow_capacity__b1->allocator->realloc                            \
                      && _tphvec_grow_capacity__b1->allocator->free);                             \
        _tphvec_grow_capacity__b2 =                                                               \
          (tphvec_header_t *)_tphvec_grow_capacity__b1->allocator->realloc(                       \
            _tphvec_grow_capacity__b1->allocator->mem_ctx,                                        \
            (void *)_tphvec_grow_capacity__b1,                                                    \
            sizeof(tphvec_header_t) + (size_t)(cap) * sizeof(*(vec)));                            \
        tphvec_assert(_tphvec_grow_capacity__b2);                                                 \
      } else {                                                                                    \
        _tphvec_grow_capacity__b2 = (tphvec_header_t *)realloc((void *)_tphvec_grow_capacity__b1, \
          sizeof(tphvec_header_t) + (size_t)(cap) * sizeof(*(vec)));                              \
      }                                                                                           \
      tphvec_assert(_tphvec_grow_capacity__b2);                                                   \
      _tphvec_grow_capacity__b2->capacity = (size_t)(cap);                                        \
      (vec) = _tphvec_base_to_vec(_tphvec_grow_capacity__b2);                                     \
    } else {                                                                                      \
      /* Initialize a new vector. */                                                              \
      tphvec_header_t *_tphvec_grow_capacity__b0 =                                                \
        (tphvec_header_t *)malloc(sizeof(tphvec_header_t) + (size_t)(cap) * sizeof(*(vec)));      \
      tphvec_assert(_tphvec_grow_capacity__b0);                                                   \
      _tphvec_grow_capacity__b0->size = 0;                                                        \
      _tphvec_grow_capacity__b0->capacity = (size_t)(cap);                                        \
      _tphvec_grow_capacity__b0->allocator = NULL;                                                \
      (vec) = _tphvec_base_to_vec(_tphvec_grow_capacity__b0);                                     \
    }                                                                                             \
  } while (0)

/**
 * @brief tphvec_shrink_to_fit - requests the container to reduce its capacity
 * to fit its size.
 * @param vec - The vector.
 * @return void
 */
#define tphvec_shrink_to_fit(vec)                                                  \
  do {                                                                             \
    if ((vec)) { _tphvec_grow_capacity((vec), _tphvec_vec_to_base((vec))->size); } \
  } while (0)

/**
 * @brief tphvec_resize - Resizes the container to contain count elements.
 * @param vec      - The vector.
 * @param new_size - New size <size_t> of the vector.
 * @param value    - New elements are initialized to this value.
 * @return void
 */
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
  printf("size = %" PRIu32 "\ncap = %" PRIu32 "\n",
    (uint32_t)tphvec_size(vec),
    (uint32_t)tphvec_capacity(vec));
  const size_t sz = tphvec_size(vec);
  printf("v = { ");
  for (size_t i = 0; i < tphvec_size(vec); ++i) {
    printf(i != tphvec_size(vec) - 1 ? "%.3f, " : "%.3f", vec[i]);
  }
  printf(" }\n");
}

int main(int argc, char *argv[])
{
  tphvec(float) v = NULL;
  tphvec_reserve(v, 10, NULL);
  print_vecf(v);
  printf("\n");

  float vals[] = { 41.F, 42.F, 43.F };
  tphvec_push_back(v, vals, 3);
  tphvec_push_back(v, vals, 3);
  tphvec_push_back(v, vals, 3);
  tphvec_push_back(v, vals, 3);
  tphvec_push_back(v, vals, 3);
  tphvec_push_back(v, vals, 3);
  print_vecf(v);
  printf("\n");

#if 0
  tphvec_resize(v, 16, 42.F);
  print_vecf(v);
  printf("\n");

  tphvec_free(v);

  float *v2 = NULL;
  tphvec_resize(v2, 16, 42.F);
  tphvec_free(v2);
  print_vecf(v2);
  printf("\n");
#endif
  return 0;
}
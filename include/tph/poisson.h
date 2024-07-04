#ifndef TPH_POISSON_H
#define TPH_POISSON_H

#define TPH_MAJOR_VERSION 0
#define TPH_MINOR_VERSION 4
#define TPH_PATCH_VERSION 0

#include <math.h>// sqrtf
#include <stddef.h>// size_t, NULL
#include <stdint.h>// uint32_t, etc

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TPH_REAL_TYPE
#define TPH_REAL_TYPE float
#endif

#ifndef TPH_SQRT
#define TPH_SQRT(_X_) sqrtf(_X_)
#endif

#ifndef TPH_CEIL
#define TPH_CEIL(_X_) ceilf(_X_)
#endif

#ifndef TPH_FLOOR
#define TPH_FLOOR(_X_) floorf(_X_)
#endif

typedef TPH_REAL_TYPE tph_real;

// clang-format off
typedef struct tph_poisson_points_           tph_poisson_points;
typedef struct tph_poisson_args_             tph_poisson_args;
typedef struct tph_poisson_sampling_         tph_poisson_sampling;
typedef struct tph_poisson_context_internal_ tph_poisson_context_internal;
// clang-format on

#pragma pack(push, 1)

struct tph_poisson_points_
{
  const tph_real *pos;
};

struct tph_poisson_args_
{
  tph_real radius;
  uint32_t dims;
  const tph_real *bounds_min;
  const tph_real *bounds_max;
  uint32_t max_sample_attempts;
  uint32_t seed;
};

struct tph_poisson_sampling_
{
  tph_poisson_context_internal *internal;
  uint32_t dims;
  uint32_t numpoints;
};

#pragma pack(pop)

#define TPH_POISSON_SUCCESS 0
#define TPH_POISSON_BAD_ALLOC 1
#define TPH_POISSON_INVALID_ARGS 2

/**
 * Uses malloc.
 */
extern int32_t tph_poisson_generate(const tph_poisson_args *args, tph_poisson_sampling *sampling);

typedef void *(*FTPHAllocFn)(void *user_ctx, size_t size);
typedef void (*FTPHFreeFn)(void *user_ctx, void *p);

extern int32_t tph_poisson_generate_useralloc(const tph_poisson_args *args,
  void *userallocctx,
  FTPHAllocFn alloc_fn,
  FTPHFreeFn free_fn,
  tph_poisson_sampling *sampling);

extern void tph_poisson_free(tph_poisson_sampling *sampling);

extern tph_poisson_points tph_poisson_get_points(const tph_poisson_sampling *sampling);

#ifdef __cplusplus
}// extern "C"
#endif

#endif// TPH_POISSON_H

#ifdef TPH_POISSON_IMPLEMENTATION
#undef TPH_POISSON_IMPLEMENTATION

/* -------------------------- */
/*
 * Copyright (c) 2015 Evan Teran
 *
 * License: The MIT License (MIT)
 */

#ifndef CVECTOR_H_
#define CVECTOR_H_

/* cvector heap implemented using C library malloc() */

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

/* NOTE: Similar to C's qsort and bsearch, you will receive a T*
 * for a vector of Ts. This means that you cannot use `free` directly
 * as a destructor. Instead if you have for example a cvector_vector_type(int *)
 * you will need to supply a function which casts `elem_ptr` to an `int**`
 * and then does a free on what that pointer points to:
 *
 * ex:
 *
 * void free_int(void *p) { free(*(int **)p); }
 */
typedef void (*cvector_elem_destructor_t)(void *elem_ptr);

typedef struct cvector_metadata_t
{
  size_t size;
  size_t capacity;
  cvector_elem_destructor_t elem_destructor;
} cvector_metadata_t;

/**
 * @brief cvector_vector_type - The vector type used in this library
 */
#define cvector_vector_type(type) type *

/**
 * @brief cvector - Syntactic sugar to retrieve a vector type
 *
 * @param type The type of vector to act on.
 */
#define cvector(type) cvector_vector_type(type)

/*
 * @brief cvector_iterator - The iterator type used for cvector
 */
#define cvector_iterator(type) cvector_vector_type(type)

/**
 * @brief cvector_vec_to_base - For internal use, converts a vector pointer to a metadata pointer
 * @param vec - the vector
 * @return the metadata pointer of the vector
 * @internal
 */
#define cvector_vec_to_base(vec) (&((cvector_metadata_t *)(vec))[-1])

/**
 * @brief cvector_base_to_vec - For internal use, converts a metadata pointer to a vector pointer
 * @param ptr - pointer to the metadata
 * @return the vector
 * @internal
 */
#define cvector_base_to_vec(ptr) ((void *)&((cvector_metadata_t *)(ptr))[1])

/**
 * @brief cvector_capacity - gets the current capacity of the vector
 * @param vec - the vector
 * @return the capacity as a size_t
 */
#define cvector_capacity(vec) ((vec) ? cvector_vec_to_base(vec)->capacity : (size_t)0)

/**
 * @brief cvector_size - gets the current size of the vector
 * @param vec - the vector
 * @return the size as a size_t
 */
#define cvector_size(vec) ((vec) ? cvector_vec_to_base(vec)->size : (size_t)0)

/**
 * @brief cvector_elem_destructor - get the element destructor function used
 * to clean up elements
 * @param vec - the vector
 * @return the function pointer as cvector_elem_destructor_t
 */
#define cvector_elem_destructor(vec) ((vec) ? cvector_vec_to_base(vec)->elem_destructor : NULL)

/**
 * @brief cvector_empty - returns non-zero if the vector is empty
 * @param vec - the vector
 * @return non-zero if empty, zero if non-empty
 */
#define cvector_empty(vec) (cvector_size(vec) == 0)

/**
 * @brief cvector_reserve - Requests that the vector capacity be at least enough
 * to contain n elements. If n is greater than the current vector capacity, the
 * function causes the container to reallocate its storage increasing its
 * capacity to n (or greater).
 * @param vec - the vector
 * @param n - Minimum capacity for the vector.
 * @return void
 */
#define cvector_reserve(vec, n)                       \
  do {                                                \
    size_t cv_cap__ = cvector_capacity(vec);          \
    if (cv_cap__ < (n)) { cvector_grow((vec), (n)); } \
  } while (0)

/*
 * @brief cvector_init - Initialize a vector.  The vector must be NULL for this to do anything.
 * @param vec - the vector
 * @param capacity - vector capacity to reserve
 * @param elem_destructor_fn - element destructor function
 * @return void
 */
#define cvector_init(vec, capacity, elem_destructor_fn)         \
  do {                                                          \
    if (!(vec)) {                                               \
      cvector_reserve((vec), capacity);                         \
      cvector_set_elem_destructor((vec), (elem_destructor_fn)); \
    }                                                           \
  } while (0)

/**
 * @brief cvector_erase - removes the element at index i from the vector
 * @param vec - the vector
 * @param i - index of element to remove
 * @return void
 */
#define cvector_erase(vec, i)                                                                     \
  do {                                                                                            \
    if (vec) {                                                                                    \
      const size_t cv_sz__ = cvector_size(vec);                                                   \
      if ((i) < cv_sz__) {                                                                        \
        cvector_elem_destructor_t elem_destructor__ = cvector_elem_destructor(vec);               \
        if (elem_destructor__) { elem_destructor__(&(vec)[i]); }                                  \
        cvector_set_size((vec), cv_sz__ - 1);                                                     \
        cvector_clib_memmove((vec) + (i), (vec) + (i) + 1, sizeof(*(vec)) * (cv_sz__ - 1 - (i))); \
      }                                                                                           \
    }                                                                                             \
  } while (0)

/**
 * @brief cvector_clear - erase all of the elements in the vector
 * @param vec - the vector
 * @return void
 */
#define cvector_clear(vec)                                                                \
  do {                                                                                    \
    if (vec) {                                                                            \
      cvector_elem_destructor_t elem_destructor__ = cvector_elem_destructor(vec);         \
      if (elem_destructor__) {                                                            \
        size_t i__;                                                                       \
        for (i__ = 0; i__ < cvector_size(vec); ++i__) { elem_destructor__(&(vec)[i__]); } \
      }                                                                                   \
      cvector_set_size(vec, 0);                                                           \
    }                                                                                     \
  } while (0)

/**
 * @brief cvector_free - frees all memory associated with the vector
 * @param vec - the vector
 * @return void
 */
#define cvector_free(vec)                                                                 \
  do {                                                                                    \
    if (vec) {                                                                            \
      void *p1__ = cvector_vec_to_base(vec);                                              \
      cvector_elem_destructor_t elem_destructor__ = cvector_elem_destructor(vec);         \
      if (elem_destructor__) {                                                            \
        size_t i__;                                                                       \
        for (i__ = 0; i__ < cvector_size(vec); ++i__) { elem_destructor__(&(vec)[i__]); } \
      }                                                                                   \
      cvector_clib_free(p1__);                                                            \
    }                                                                                     \
  } while (0)

/**
 * @brief cvector_begin - returns an iterator to first element of the vector
 * @param vec - the vector
 * @return a pointer to the first element (or NULL)
 */
#define cvector_begin(vec) (vec)

/**
 * @brief cvector_end - returns an iterator to one past the last element of the vector
 * @param vec - the vector
 * @return a pointer to one past the last element (or NULL)
 */
#define cvector_end(vec) ((vec) ? &((vec)[cvector_size(vec)]) : NULL)

/* user request to use logarithmic growth algorithm */
#define CVECTOR_LOGARITHMIC_GROWTH
#ifdef CVECTOR_LOGARITHMIC_GROWTH

/**
 * @brief cvector_compute_next_grow - returns an the computed size in next vector grow
 * size is increased by multiplication of 2
 * @param size - current size
 * @return size after next vector grow
 */
#define cvector_compute_next_grow(size) ((size) ? ((size) << 1) : 1)

#else

/**
 * @brief cvector_compute_next_grow - returns an the computed size in next vector grow
 * size is increased by 1
 * @param size - current size
 * @return size after next vector grow
 */
#define cvector_compute_next_grow(size) ((size) + 1)

#endif /* CVECTOR_LOGARITHMIC_GROWTH */

/**
 * @brief cvector_push_back - adds an element to the end of the vector
 * @param vec - the vector
 * @param value - the value to add
 * @return void
 */
#define cvector_push_back(vec, value)                           \
  do {                                                          \
    size_t cv_cap__ = cvector_capacity(vec);                    \
    if (cv_cap__ <= cvector_size(vec)) {                        \
      cvector_grow((vec), cvector_compute_next_grow(cv_cap__)); \
    }                                                           \
    (vec)[cvector_size(vec)] = (value);                         \
    cvector_set_size((vec), cvector_size(vec) + 1);             \
  } while (0)

/**
 * @brief cvector_insert - insert element at position pos to the vector
 * @param vec - the vector
 * @param pos - position in the vector where the new elements are inserted.
 * @param val - value to be copied (or moved) to the inserted elements.
 * @return void
 */
#define cvector_insert(vec, pos, val)                                                      \
  do {                                                                                     \
    size_t cv_cap__ = cvector_capacity(vec);                                               \
    if (cv_cap__ <= cvector_size(vec)) {                                                   \
      cvector_grow((vec), cvector_compute_next_grow(cv_cap__));                            \
    }                                                                                      \
    if ((pos) < cvector_size(vec)) {                                                       \
      cvector_clib_memmove(                                                                \
        (vec) + (pos) + 1, (vec) + (pos), sizeof(*(vec)) * ((cvector_size(vec)) - (pos))); \
    }                                                                                      \
    (vec)[(pos)] = (val);                                                                  \
    cvector_set_size((vec), cvector_size(vec) + 1);                                        \
  } while (0)

/**
 * @brief cvector_pop_back - removes the last element from the vector
 * @param vec - the vector
 * @return void
 */
#define cvector_pop_back(vec)                                                    \
  do {                                                                           \
    cvector_elem_destructor_t elem_destructor__ = cvector_elem_destructor(vec);  \
    if (elem_destructor__) { elem_destructor__(&(vec)[cvector_size(vec) - 1]); } \
    cvector_set_size((vec), cvector_size(vec) - 1);                              \
  } while (0)

/**
 * @brief cvector_copy - copy a vector
 * @param from - the original vector
 * @param to - destination to which the function copy to
 * @return void
 */
#define cvector_copy(from, to)                                                 \
  do {                                                                         \
    if ((from)) {                                                              \
      cvector_grow(to, cvector_size(from));                                    \
      cvector_set_size(to, cvector_size(from));                                \
      cvector_clib_memcpy((to), (from), cvector_size(from) * sizeof(*(from))); \
    }                                                                          \
  } while (0)

/**
 * @brief cvector_swap - exchanges the content of the vector by the content of another vector of the
 * same type
 * @param vec - the original vector
 * @param other - the other vector to swap content with
 * @param type - the type of both vectors
 * @return void
 */
#define cvector_swap(vec, other, type)           \
  do {                                           \
    if (vec && other) {                          \
      cvector_vector_type(type) cv_swap__ = vec; \
      vec = other;                               \
      other = cv_swap__;                         \
    }                                            \
  } while (0)

/**
 * @brief cvector_set_capacity - For internal use, sets the capacity variable of the vector
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 * @internal
 */
#define cvector_set_capacity(vec, size)                       \
  do {                                                        \
    if (vec) { cvector_vec_to_base(vec)->capacity = (size); } \
  } while (0)

/**
 * @brief cvector_set_size - For internal use, sets the size variable of the vector
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 * @internal
 */
#define cvector_set_size(vec, _size)                       \
  do {                                                     \
    if (vec) { cvector_vec_to_base(vec)->size = (_size); } \
  } while (0)

/**
 * @brief cvector_set_elem_destructor - set the element destructor function
 * used to clean up removed elements. The vector must NOT be NULL for this to do anything.
 * @param vec - the vector
 * @param elem_destructor_fn - function pointer of type cvector_elem_destructor_t used to destroy
 * elements
 * @return void
 */
#define cvector_set_elem_destructor(vec, elem_destructor_fn)                       \
  do {                                                                             \
    if (vec) { cvector_vec_to_base(vec)->elem_destructor = (elem_destructor_fn); } \
  } while (0)

/**
 * @brief cvector_grow - For internal use, ensures that the vector is at least <count> elements big
 * @param vec - the vector
 * @param count - the new capacity to set
 * @return void
 * @internal
 */
#define cvector_grow(vec, count)                                                  \
  do {                                                                            \
    const size_t cv_sz__ = (count) * sizeof(*(vec)) + sizeof(cvector_metadata_t); \
    if (vec) {                                                                    \
      void *cv_p1__ = cvector_vec_to_base(vec);                                   \
      void *cv_p2__ = cvector_clib_realloc(cv_p1__, cv_sz__);                     \
      cvector_clib_assert(cv_p2__);                                               \
      (vec) = cvector_base_to_vec(cv_p2__);                                       \
    } else {                                                                      \
      void *cv_p__ = cvector_clib_malloc(cv_sz__);                                \
      cvector_clib_assert(cv_p__);                                                \
      (vec) = cvector_base_to_vec(cv_p__);                                        \
      cvector_set_size((vec), 0);                                                 \
      cvector_set_elem_destructor((vec), NULL);                                   \
    }                                                                             \
    cvector_set_capacity((vec), (count));                                         \
  } while (0)

/**
 * @brief cvector_shrink_to_fit - requests the container to reduce its capacity to fit its size
 * @param vec - the vector
 * @return void
 */
#define cvector_shrink_to_fit(vec)               \
  do {                                           \
    if (vec) {                                   \
      const size_t cv_sz___ = cvector_size(vec); \
      cvector_grow(vec, cv_sz___);               \
    }                                            \
  } while (0)

/**
 * @brief cvector_at - returns a reference to the element at position n in the vector.
 * @param vec - the vector
 * @param n - position of an element in the vector.
 * @return the element at the specified position in the vector.
 */
#define cvector_at(vec, n) \
  ((vec) ? (((int)(n) < 0 || (size_t)(n) >= cvector_size(vec)) ? NULL : &(vec)[n]) : NULL)

/**
 * @brief cvector_front - returns a reference to the first element in the vector. Unlike member
 * cvector_begin, which returns an iterator to this same element, this function returns a direct
 * reference.
 * @return a reference to the first element in the vector container.
 */
#define cvector_front(vec) ((vec) ? ((cvector_size(vec) > 0) ? cvector_at(vec, 0) : NULL) : NULL)

/**
 * @brief cvector_back - returns a reference to the last element in the vector.Unlike member
 * cvector_end, which returns an iterator just past this element, this function returns a direct
 * reference.
 * @return a reference to the last element in the vector.
 */
#define cvector_back(vec) \
  ((vec) ? ((cvector_size(vec) > 0) ? cvector_at(vec, cvector_size(vec) - 1) : NULL) : NULL)

/**
 * @brief cvector_resize - resizes the container to contain count elements.
 * @param vec - the vector
 * @param count - new size of the vector
 * @param value - the value to initialize new elements with
 * @return void
 */
#define cvector_resize(vec, count, value)                            \
  do {                                                               \
    if (vec) {                                                       \
      size_t cv_sz_count__ = (size_t)(count);                        \
      size_t cv_sz__ = cvector_vec_to_base(vec)->size;               \
      if (cv_sz_count__ > cv_sz__) {                                 \
        cvector_reserve((vec), cv_sz_count__);                       \
        cvector_set_size((vec), cv_sz_count__);                      \
        do {                                                         \
          (vec)[cv_sz__++] = (value);                                \
        } while (cv_sz__ < cv_sz_count__);                           \
      } else {                                                       \
        while (cv_sz_count__ < cv_sz__--) { cvector_pop_back(vec); } \
      }                                                              \
    }                                                                \
  } while (0)

#endif /* CVECTOR_H_ */

/* -------------------------- */

#include <assert.h>// assert
#include <stdlib.h>// malloc, realloc, free

// Structs

#pragma pack(push, 1)

typedef struct tph_poisson_grid_
{
  tph_real dx;
  tph_real dx_inv;
  uint32_t dims;
  const tph_real* bounds_min;
  const tph_real* bounds_max;
  int32_t *size;
  uint32_t *cells;
} tph_poisson_grid;

struct tph_poisson_context_internal_
{
#if 0
  void *mem;
  jcv_edge *edges;
  jcv_halfedge *beachline_start;
  jcv_halfedge *beachline_end;
  jcv_halfedge *last_inserted;
  jcv_priorityqueue *eventqueue;

  jcv_site *sites;
  jcv_site *bottomsite;
  int numsites;
  int currentsite;
  int _padding;

  jcv_memoryblock *memblocks;
  jcv_edge *edgepool;
  jcv_halfedge *halfedgepool;
  void **eventmem;
  jcv_clipper clipper;
#endif

  tph_real *pos;

  void *mem_ctx;// Given by the user
  FTPHAllocFn alloc;
  FTPHFreeFn free;
};

#pragma pack(pop)

static void tph_poisson_grid_free(tph_poisson_grid *grid,
  const tph_poisson_context_internal *internal)
{
  internal->free(internal->mem_ctx, grid->size);
  internal->free(internal->mem_ctx, grid->cells);
}

static int32_t tph_poisson_grid_create(tph_poisson_grid *grid,
  tph_real radius,
  uint32_t dims,
  const tph_real *bounds_min,
  const tph_real *bounds_max,
  const tph_poisson_context_internal *internal)
{
  const tph_real dx = ((tph_real)0.999 * radius) / TPH_SQRT((tph_real)dims);
  const tph_real dx_inv = (tph_real)1 / dx;

  int32_t *size = (int32_t *)internal->alloc(internal->mem_ctx, dims * sizeof(int32_t));
  if (size == NULL) { return TPH_POISSON_BAD_ALLOC; }

  uint32_t linear_size = 1;
  for (uint32_t i = 0; i < dims; ++i) {
    assert(bounds_max[i] > bounds_min[i]);
    size[i] = (int32_t)TPH_CEIL((bounds_max[i] - bounds_min[i]) * dx_inv);
    assert(size[i] > 0);
    linear_size *= size[i];
  }

  uint32_t *cells = (uint32_t *)internal->alloc(internal->mem_ctx, linear_size * sizeof(int32_t));
  if (cells == NULL) {
    internal->free(internal->mem_ctx, size);
    return TPH_POISSON_BAD_ALLOC;
  }
  for (size_t i = 0; i < linear_size; ++i) {
    // Initialize cells with value -1, indicating no sample there.
    // Cell values are later set to indices of samples.
    cells[i] = 0xFFFFFFFF;
  }

  grid->dx = dx;
  grid->dx_inv = dx_inv;
  grid->dims = dims;
  grid->bounds_min = bounds_min;
  grid->bounds_max = bounds_max;
  grid->size = size;
  grid->cells = cells;

  return TPH_POISSON_SUCCESS;
}

static void tph_poisson_grid_sample_to_index(const tph_poisson_grid *grid,
  const tph_real *sample,
  int32_t *index)
{
  for (uint32_t i = 0; i < grid->dims; ++i) { 
    index[i] = (int32_t)TPH_FLOOR((sample[i] - grid->bounds_min[i]) * grid->dx_inv);
  }
}

static uint32_t
  tph_poisson_grid_linear_index(const tph_poisson_grid *grid, const int32_t *index)
{
  assert(0 <= index[0] && index[0] < grid->size[0]);
  uint32_t k = (size_t)index[0];
  uint32_t d = 1;
  for (uint32_t i = 1; i < grid->dims; ++i) {
    assert(0 <= index[i] && index[i] < grid->size[i]);
    // Note: Not checking for "overflow".
    d *= (uint32_t)grid->size[i - 1];
    k += (uint32_t)(index[i] * d);
  }
  return k;
}

static uint32_t *tph_poisson_grid_cell(tph_poisson_grid *grid, size_t lin_index)
{
  return &grid->cells[lin_index];
}

static void *tph_alloc_fn(void *mem_ctx, size_t size)
{
  (void)mem_ctx;
  return malloc(size);
}

static void tph_free_fn(void *mem_ctx, void *p)
{
  (void)mem_ctx;
  free(p);
}

static int32_t tph_valid_args(const tph_poisson_args *args)
{
  if (args == NULL) { return 0; }
  int valid = 1;
  valid &= (args->radius > 0.F);
  valid &= (args->max_sample_attempts > 0);
  valid &= (args->dims >= 1);
  for (uint32_t i = 0; i < args->dims; ++i) {
    valid &= (args->bounds_max[i] > args->bounds_min[i]);
  }
  return valid;
}

static void tph_poisson_add_sample(const tph_real *sample,
  tph_real *samples,
  uint32_t *active_indices,
  tph_poisson_grid* grid,
  int32_t* index)
{
  for (uint32_t i = 0; i < grid->dims; ++i) { cvector_push_back(samples, sample[i]); }
  const uint32_t sample_index = cvector_size(samples) / grid->dims - 1;
  cvector_push_back(active_indices, sample_index);
  tph_poisson_grid_sample_to_index(grid, sample, index);
  const uint32_t lin_index = tph_poisson_grid_linear_index(grid, index);
  uint32_t *cell = tph_poisson_grid_cell(grid, lin_index);
  assert(sample_index != 0xFFFFFFFF);
  *cell = sample_index;
}

int32_t tph_poisson_generate(const tph_poisson_args *args, tph_poisson_sampling *s)
{
  return tph_poisson_generate_useralloc(args, NULL, tph_alloc_fn, tph_free_fn, s);
}

int32_t tph_poisson_generate_useralloc(const tph_poisson_args *args,
  void *user_alloc_ctx,
  FTPHAllocFn alloc_fn,
  FTPHFreeFn free_fn,
  tph_poisson_sampling *s)
{
  assert(s != NULL);
  if (s->internal != NULL) { tph_poisson_free(s); }
  s->internal = NULL;
  s->dims = 0;
  s->numpoints = 0;

  if (!tph_valid_args(args) || alloc_fn == NULL || free_fn == NULL) {
    return TPH_POISSON_INVALID_ARGS;
  }

  tph_poisson_context_internal *internal =
    (tph_poisson_context_internal *)alloc_fn(user_alloc_ctx, sizeof(tph_poisson_context_internal));
  if (internal == NULL) { return TPH_POISSON_BAD_ALLOC; }
  internal->mem_ctx = user_alloc_ctx;
  internal->alloc = alloc_fn;
  internal->free = free_fn;

  // Acceleration grid.
  tph_poisson_grid grid;
  int32_t ret = tph_poisson_grid_create(
    &grid, args->radius, args->dims, args->bounds_min, args->bounds_max, internal);
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_free(s);
    return ret;
  }

  cvector_vector_type(tph_real) samples = NULL;
  cvector_vector_type(uint32_t) active_indices = NULL;

  tph_real *rand_pos = (tph_real *)alloc_fn(internal->mem_ctx, args->dims * sizeof(tph_real));

  tph_poisson_add_sample(rand_pos, );

#if 1// TMP TEST!!
  internal->pos = (tph_real *)internal->alloc(internal->mem_ctx, 5 * args->dims * sizeof(tph_real));
  if (internal->pos == NULL) { return TPH_POISSON_BAD_ALLOC; }
  for (uint32_t i = 0; i < 5; ++i) {
    internal->pos[i * args->dims] = (tph_real)(args->dims * i);
    internal->pos[i * args->dims + 1] = (tph_real)(args->dims * i + 1);
  }
#endif

  tph_poisson_grid_free(&grid, internal);
  cvector_free(samples);
  cvector_free(active_indices);

  s->internal = internal;
  s->dims = args->dims;
  s->numpoints = 5;

  return TPH_POISSON_SUCCESS;

#if 0
  jcv_context_internal *internal = jcv_alloc_internal(num_points, userallocctx, allocfn, freefn);

  internal->beachline_start = jcv_halfedge_new(internal, 0, 0);
  internal->beachline_end = jcv_halfedge_new(internal, 0, 0);

  internal->beachline_start->left = 0;
  internal->beachline_start->right = internal->beachline_end;
  internal->beachline_end->left = internal->beachline_start;
  internal->beachline_end->right = 0;

  internal->last_inserted = 0;

  int max_num_events = num_points * 2;// beachline can have max 2*n-5 parabolas
  jcv_pq_create(internal->eventqueue, max_num_events, (void **)internal->eventmem);

  internal->numsites = num_points;
  jcv_site *sites = internal->sites;

  for (int i = 0; i < num_points; ++i) {
    sites[i].p = points[i];
    sites[i].edges = 0;
    sites[i].index = i;
  }

  qsort(sites, (size_t)num_points, sizeof(jcv_site), jcv_point_cmp);

  jcv_clipper box_clipper;
  if (clipper == 0) {
    box_clipper.test_fn = jcv_boxshape_test;
    box_clipper.clip_fn = jcv_boxshape_clip;
    box_clipper.fill_fn = jcv_boxshape_fillgaps;
    clipper = &box_clipper;
  }
  internal->clipper = *clipper;

  jcv_rect tmp_rect;
  tmp_rect.min.x = tmp_rect.min.y = JCV_FLT_MAX;
  tmp_rect.max.x = tmp_rect.max.y = -JCV_FLT_MAX;
  jcv_prune_duplicates(internal, &tmp_rect);

  // Prune using the test second
  if (internal->clipper.test_fn) {
    // e.g. used by the box clipper in the test_fn
    internal->clipper.min = rect ? rect->min : tmp_rect.min;
    internal->clipper.max = rect ? rect->max : tmp_rect.max;

    jcv_prune_not_in_shape(internal, &tmp_rect);

    // The pruning might have made the bounding box smaller
    if (!rect) {
      // In the case of all sites being all on a horizontal or vertical line, the
      // rect area will be zero, and the diagram generation will most likely fail
      jcv_rect_round(&tmp_rect);
      jcv_rect_inflate(&tmp_rect, 10);

      internal->clipper.min = tmp_rect.min;
      internal->clipper.max = tmp_rect.max;
    }
  }

  internal->rect = rect ? *rect : tmp_rect;

  d->min = internal->rect.min;
  d->max = internal->rect.max;
  d->numsites = internal->numsites;
  d->internal = internal;

  internal->bottomsite = jcv_nextsite(internal);

  jcv_priorityqueue *pq = internal->eventqueue;
  jcv_site *site = jcv_nextsite(internal);

  int finished = 0;
  while (!finished) {
    jcv_point lowest_pq_point;
    if (!jcv_pq_empty(pq)) {
      jcv_halfedge *he = (jcv_halfedge *)jcv_pq_top(pq);
      lowest_pq_point.x = he->vertex.x;
      lowest_pq_point.y = he->y;
    }

    if (site != 0 && (jcv_pq_empty(pq) || jcv_point_less(&site->p, &lowest_pq_point))) {
      jcv_site_event(internal, site);
      site = jcv_nextsite(internal);
    } else if (!jcv_pq_empty(pq)) {
      jcv_circle_event(internal);
    } else {
      finished = 1;
    }
  }

  for (jcv_halfedge *he = internal->beachline_start->right; he != internal->beachline_end;
       he = he->right) {
    jcv_finishline(internal, he->edge);
  }

  jcv_fillgaps(d);
#endif
}

void tph_poisson_free(tph_poisson_sampling *s)
{
  if (s == NULL || s->internal == NULL) { return; }
  tph_poisson_context_internal *internal = s->internal;
  FTPHFreeFn free_fn = internal->free;
  void *mem_ctx = internal->mem_ctx;
  free_fn(mem_ctx, internal->pos);
  free_fn(mem_ctx, internal);
  s->internal = NULL;
}

tph_poisson_points tph_poisson_get_points(const tph_poisson_sampling *s)
{
  tph_poisson_points p = { .pos = s != NULL && s->internal != NULL ? s->internal->pos : NULL };
  return p;
}

#endif// TPH_POISSON_IMPLEMENTATION
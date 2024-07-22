/**
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

#ifndef TPHVEC_H
#define TPHVEC_H

#include <stddef.h> /* size_t, ptrdiff_t */
#include <stdint.h> /* PTRDIFF_MAX */
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

#ifndef tphvec_assert
#include <assert.h>
#define tphvec_assert(_X_) assert((_X_))
#endif

/* clang-format off */
typedef struct {
  void *(*malloc)(ptrdiff_t size, void *ctx);
  void *(*realloc)(void *ptr, ptrdiff_t old_size, ptrdiff_t new_size, void *ctx);
  void  (*free)(void *ptr, ptrdiff_t size, void *ctx);
  void *ctx;    
} tphvec_allocator_t;

/* Error codes. */
#define TPHVEC_SUCCESS       0
#define TPHVEC_BAD_ALLOC     1
#define TPHVEC_INVALID_ARG   2
/* clang-format on */


/**
 * @brief The vector type used in this library.
 * @param type The type of elements in the vector, e.g. int, float, etc.
 */
#define tphvec(type) type *


/**
 * @brief tphvec_free - Frees all memory associated with the vector.
 * @param vec - Vector.
 * @return void
 */
extern void tphvec_free(void *vec);


/**
 * @brief Returns the allocator associated with the container.
 * @param vec Vector.
 * @return The associated allocator, or NULL if none has been set.
 */
extern tphvec_allocator_t *tphvec_get_allocator(const void *vec);


/**
 * @brief Sets the allocator to be used by the vector.
 * @param vec Vector.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
#define tphvec_set_allocator(vec, allocator) (_tphvec_set_allocator((void **)&(vec), (allocator)))
extern int _tphvec_set_allocator(void **vec_ptr, tphvec_allocator_t *allocator);


/**
 * @brief Checks if the container has no elements.
 * @param vec - Vector.
 * @return Non-zero if empty, zero otherwise.
 */
#define tphvec_empty(vec) (_tphvec_empty((vec)))
extern int _tphvec_empty(const void *vec);


/**
 * @brief Returns the number of elements in the vector.
 * @param vec Vector.
 * @return The number of elements in the vector.
 */
#define tphvec_size(vec) (_tphvec_size((vec), sizeof(*(vec))))
extern ptrdiff_t _tphvec_size(const void *vec, size_t sizeof_el);


/**
 * @brief Requests that the vector capacity be at least
 * enough to contain <cap> elements. If <cap> is greater than the current vector
 * capacity, the function causes the container to reallocate its storage
 * increasing its capacity to <cap> (or greater).
 * @param vec       - Vector.
 * @param new_cap   - Minimum number of elements to reserve memory for.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
#define tphvec_reserve(vec, new_cap) (_tphvec_reserve((void **)&(vec), (new_cap) * sizeof(*(vec))))
extern int _tphvec_reserve(void **vec_ptr, ptrdiff_t new_cap);


/**
 * @brief Returns the number of elements that the container has currently allocated space for.
 * @param vec Vector.
 * @return Capacity of the currently allocated storage in number of elements as a ptrdiff_t.
 */
#define tphvec_capacity(vec) (_tphvec_capacity((vec), sizeof(*(vec))))
extern ptrdiff_t _tphvec_capacity(const void *vec, size_t sizeof_el);


/**
 * @brief Requests the removal of unused capacity.
 * @param vec Vector.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
#define tphvec_shrink_to_fit(vec) (_tphvec_shrink_to_fit((void **)&(vec)))
extern int _tphvec_shrink_to_fit(void **vec_ptr);


/**
 * @brief tphvec_clear - Erase all the elements in the vector, capacity is unchanged.
 * @param vec - Vector.
 * @return void
 */
extern void tphvec_clear(void *vec);


/**
 * @brief Add elements to the end of the vector.
 * @param vec    Vector.
 * @param values Pointer to values to be added.
 * @param count  Number of elements to copy from values.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
#define tphvec_append(vec, values, count) \
  (_tphvec_append((void **)&(vec), (values), (count) * sizeof(*(values))))
extern int _tphvec_append(void **vec_ptr, void *values, ptrdiff_t nbytes);


/**
 * @brief Resizes the container to contain count elements, does nothing if count ==
 * tphvec_size(vec).
 * @param vec   Vector.
 * @param count New size of the vector.
 * @param value The value to initialize the new elements with.
 * @return TPHVEC_SUCCESS, or a non-zero error code.
 */
#define tphvec_resize(vec, count, value) \
  (_tphvec_resize((void **)&(vec), (count) * sizeof(*(vec)), &(value), sizeof((value))))
extern int _tphvec_resize(void **vec_ptr, ptrdiff_t count, void *value, ptrdiff_t sizeof_value);

#define tphvec_erase_unordered(vec, pos, count) \
  (_tphvec_erase_unordered((vec), (pos), (count), sizeof(*(vec))))
extern void _tphvec_erase_unordered(void *vec, ptrdiff_t pos, ptrdiff_t count, size_t sizeof_elem);

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


#endif /* TPHVEC_H */

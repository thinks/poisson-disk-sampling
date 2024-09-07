/*
 * Copyright(c) 2024 Tommy Hinks
 * For LICENSE (MIT), USAGE, and HISTORY see the end of this file.
 */

#ifndef TPH_POISSON_H
#define TPH_POISSON_H

#define TPH_POISSON_MAJOR_VERSION 0
#define TPH_POISSON_MINOR_VERSION 4
#define TPH_POISSON_PATCH_VERSION 0

/*
 * TODOS:
 * - Build and run tests with sanitizers!!
 * - clang-tidy
 */

#include <stddef.h>// size_t, ptrdiff_t, NULL
#include <stdint.h>// uint32_t, uintptr_t, etc

#ifdef __cplusplus
extern "C" {
#endif

/* BEGIN PUBLIC API --------------------------------------------------------- */

/* clang-format off */
#if (defined(TPH_POISSON_REAL_TYPE)                                                                    \
    && (!defined(TPH_POISSON_SQRT) || !defined(TPH_POISSON_CEIL) || !defined(TPH_POISSON_FLOOR)))      \
||  (defined(TPH_POISSON_SQRT)                                                                         \
    && (!defined(TPH_POISSON_REAL_TYPE) || !defined(TPH_POISSON_CEIL) || !defined(TPH_POISSON_FLOOR))) \
||  (defined(TPH_POISSON_CEIL)                                                                         \
    && (!defined(TPH_POISSON_REAL_TYPE) || !defined(TPH_POISSON_SQRT) || !defined(TPH_POISSON_FLOOR))) \
||  (defined(TPH_POISSON_FLOOR)                                                                        \
    && (!defined(TPH_POISSON_REAL_TYPE) || !defined(TPH_POISSON_SQRT) || !defined(TPH_POISSON_CEIL))) 
#error \
  "TPH_POISSON_REAL_TYPE, TPH_POISSON_SQRT, TPH_POISSON_CEIL and TPH_POISSON_FLOOR must all be defined; or none of them."
#endif
#if !defined(TPH_POISSON_REAL_TYPE) && !defined(TPH_POISSON_SQRT) && !defined(TPH_POISSON_CEIL) \
  && !defined(TPH_POISSON_FLOOR)
#define TPH_POISSON_REAL_TYPE float
#include <math.h>
#define TPH_POISSON_SQRT  sqrtf
#define TPH_POISSON_CEIL  ceilf
#define TPH_POISSON_FLOOR floorf
#endif
/* clang-format on */

typedef TPH_POISSON_REAL_TYPE tph_poisson_real;

/* clang-format off */
typedef struct tph_poisson_args_              tph_poisson_args;
typedef struct tph_poisson_allocator_         tph_poisson_allocator;
typedef struct tph_poisson_sampling_          tph_poisson_sampling;
typedef struct tph_poisson_sampling_internal_ tph_poisson_sampling_internal;

typedef void *(*tph_poisson_malloc_fn)(ptrdiff_t size, void *ctx);
typedef void (*tph_poisson_free_fn)(void *ptr, ptrdiff_t size, void *ctx);
/* clang-format on */

#pragma pack(push, 1)

struct tph_poisson_allocator_
{
  tph_poisson_malloc_fn malloc;
  tph_poisson_free_fn free;
  void *ctx;
};

struct tph_poisson_args_
{
  const tph_poisson_real *bounds_min;
  const tph_poisson_real *bounds_max;
  uint64_t seed;
  tph_poisson_real radius;
  int32_t ndims;
  uint32_t max_sample_attempts;
};

struct tph_poisson_sampling_
{
  tph_poisson_sampling_internal *internal;
  ptrdiff_t nsamples;
  int32_t ndims;
};

#pragma pack(pop)

/* clang-format off */

/* Possible return values from tph_poisson_create. */
#define TPH_POISSON_SUCCESS       0
#define TPH_POISSON_BAD_ALLOC     1
#define TPH_POISSON_INVALID_ARGS  2
#define TPH_POISSON_OVERFLOW      3

/* clang-format on */

/**
 * Generates a list of samples with the guarantees:
 *   (1) No two samples are closer to each other than args.radius;
 *   (2) No sample is outside the region [args.bounds_min, args.bounds_max].
 *
 * The algorithm tries to fit as many samples as possible into the provided region
 * without violating the above requirements. After creation, the samples can be
 * accessed using the tph_poisson_get_samples function.
 *
 * Errors:
 *   TPH_POISSON_BAD_ALLOC - Failed memory allocation.
 *   TPH_POISSON_INVALID_ARGS - The arguments are invalid if:
 *   - args.radius is <= 0, or
 *   - args.ndims is < 1, or
 *   - args.bounds_min[i] >= args.bounds_max[i], or
 *   - args.max_sample_attempts == 0, or
 *   - an invalid allocator is provided.
 *   TPH_POISSON_OVERFLOW - The number of samples exceeds the maximum number.
 *
 * Note that when an error is returned the sampling does not need to be destroyed
 * using the tph_poisson_destroy function.
 *
 * @param sampling Sampling to store samples.
 * @param args     Arguments.
 * @param alloc    Optional custom allocator (may be null).
 * @return TPH_POISSON_SUCCESS if no errors; otherwise a non-zero error code.
 */
extern int tph_poisson_create(const tph_poisson_args *args,
  const tph_poisson_allocator *alloc,
  tph_poisson_sampling *sampling);

/**
 * @brief Frees all memory used by the sampling. Note that the sampling itself is not free'd.
 * @param sampling Sampling to store samples.
 */
extern void tph_poisson_destroy(tph_poisson_sampling *sampling);

/**
 * Returns a pointer to the samples in the provided sampling. Samples are stored as
 * N-dimensional points, i.e. the first N values are the coordinates of the first point, etc.
 * Note that sampling.ndims and sampling.nsamples can be used to unpack the raw samples into points.
 * @param sampling Sampling to store samples.
 * @return Pointer to samples, or NULL if the sampling has not been successfully initialized by a
 * call to the tph_poisson_create function (without being destroy by the tph_poisson_destroy
 * function in between).
 */
extern const tph_poisson_real *tph_poisson_get_samples(tph_poisson_sampling *sampling);

/* END PUBLIC API ----------------------------------------------------------- */

#ifdef __cplusplus
}// extern "C"
#endif

#endif// TPH_POISSON_H

/* BEGIN IMPLEMENTATION ------------------------------------------------------*/

#ifdef TPH_POISSON_IMPLEMENTATION
#undef TPH_POISSON_IMPLEMENTATION

#include <stdalign.h>// alignof
#include <stdbool.h>// bool, true, false

#if defined(_MSC_VER) && !defined(__cplusplus)
#define TPH_POISSON_INLINE __inline
#else
#define TPH_POISSON_INLINE inline
#endif

#ifndef tph_poisson_assert
#include <assert.h>
#define tph_poisson_assert(_X_) assert((_X_))
#endif

#ifndef TPH_POISSON_MEMCPY
#include <string.h>
#define TPH_POISSON_MEMCPY(dst, src, n) memcpy((dst), (src), (n))
#endif

#ifndef TPH_POISSON_MEMSET
#include <string.h>
#define TPH_POISSON_MEMSET(s, c, n) memset((s), (c), (n))
#endif

/*
 * MEMORY
 */

/* clang-format off */
#if ( defined(TPH_POISSON_MALLOC) && !defined(TPH_POISSON_FREE)) || \
    (!defined(TPH_POISSON_MALLOC) &&  defined(TPH_POISSON_FREE))
#error \
  "TPH_POISSON_MALLOC and TPH_POISSON_FREE must both be defined; or none of them."
#endif
#if !defined(TPH_POISSON_MALLOC) && !defined(TPH_POISSON_FREE)
#include <stdlib.h>
#define TPH_POISSON_MALLOC malloc
#define TPH_POISSON_FREE free
#endif
/* clang-format on */

/* Adapt libc memory functions to our allocator interface. These functions will be called if no
 * custom allocator is provided at run-time. */

static TPH_POISSON_INLINE void *tph_poisson_malloc(ptrdiff_t size, void *ctx)
{
  (void)ctx;
  return size > 0 ? TPH_POISSON_MALLOC((size_t)size) : (void *)NULL;
}

static TPH_POISSON_INLINE void tph_poisson_free(void *ptr, ptrdiff_t size, void *ctx)
{
  (void)size;
  (void)ctx;
  TPH_POISSON_FREE(ptr);
}

/**
 * @brief Returns a pointer aligned to the provided alignment. The address pointed
 * to is a multiple of the alignment. Assumes that alignment is a power of two (which
 * seems to always be the case when querying the value using the alignof function). The
 * alignment is done by (potentially) moving the pointer 'forward' until it satisfies
 * the address is a multiple of alignment.
 * @param ptr       Pointer to align.
 * @param alignment Requested alignment. Assumed to be a power of two.
 * @return An aligned pointer.
 */
static TPH_POISSON_INLINE void *tph_poisson_align(void *ptr, const size_t alignment)
{
  tph_poisson_assert((alignment > 0) & ((alignment & (alignment - 1)) == 0));
  return (void *)(((uintptr_t)ptr + (alignment - 1)) & ~(alignment - 1));
}

static tph_poisson_allocator tph_poisson_default_alloc = { tph_poisson_malloc,
  tph_poisson_free,
  /*ctx=*/NULL };

/*
 * PSEUDO-RANDOM NUMBER GENERATION
 */

typedef struct
{
  uint64_t s;
} tph_poisson_splitmix64_state;

/**
 * @brief Returns a pseudo-random number generated using the SplitMix64 algorithm and mutates the
 * state in preparation for subsequent calls.
 * @param state State to be mutated into the next number in the sequence.
 * @return A pseudo-random number
 */
static uint64_t tph_poisson_splitmix64(tph_poisson_splitmix64_state *state)
{
  uint64_t result = (state->s += 0x9E3779B97f4A7C15);
  result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
  result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
  return result ^ (result >> 31);
}

typedef struct
{
  uint64_t s[4];
} tph_poisson_xoshiro256p_state;

/**
 * @brief Initializes a xoshiro256p state.
 * @param state The state to initialize.
 * @param seed  Seed value, can be zero.
 */
static void tph_poisson_xoshiro256p_init(tph_poisson_xoshiro256p_state *state, const uint64_t seed)
{
  /* As suggested at https://prng.di.unimi.it, use SplitMix64 to initialize the state of a
   * generator starting from a 64-bit seed. It has been shown that initialization must be
   * performed with a generator radically different in nature from the one used to avoid
   * correlation on similar seeds. */
  tph_poisson_splitmix64_state sm_state = { seed };
  state->s[0] = tph_poisson_splitmix64(&sm_state);
  state->s[1] = tph_poisson_splitmix64(&sm_state);
  state->s[2] = tph_poisson_splitmix64(&sm_state);
  state->s[3] = tph_poisson_splitmix64(&sm_state);
}

/**
 * @brief Returns a pseudo-random number generated using the xoshiro256+ algorithm and mutates the
 * state in preparation for subsequent calls. Assumes that the value of the passed in state is not
 * all zeros.
 * @param state State to be mutated into the next number in the sequence.
 * @return A pseudo-random number.
 */
static uint64_t tph_poisson_xoshiro256p_next(tph_poisson_xoshiro256p_state *state)
{
  uint64_t *s = state->s;
  const uint64_t result = s[0] + s[3];
  const uint64_t t = s[1] << 17;
  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];
  s[2] ^= t;

  /* Hard-code: s[3] = rotl(s[3], 45) */
  s[3] = (s[3] << 45) | (s[3] >> 49);
  return result;
}

/*
 * MISC
 */

/**
 * @brief Returns a floating point number in [0..1).
 * @param x Bit representation.
 * @return A number in [0..1).
 */
static TPH_POISSON_INLINE double tph_poisson_to_double(const uint64_t x)
{
  /* Convert to double, as suggested at https://prng.di.unimi.it. This conversion prefers the high
   * bits of x (usually, a good idea). */
  return (double)(x >> 11) * 0x1.0p-53;
}

/*
 * VECTOR
 */

typedef struct tph_poisson_vec_
{
  void *mem;
  ptrdiff_t mem_size; /** [bytes] */
  void *begin;
  void *end;
} tph_poisson_vec;

/**
 * @brief Returns non-zero if the vector is in a valid state; otherwise zero. Only used for
 * debugging.
 * @param ElemT Vector element type.
 * @param vec   Vector.
 * @return True if the vector is in a valid state; otherwise false.
 */
#define tph_poisson_vec_invariants(ElemT, vec) \
  (tph_poisson_vec_invariants_impl((vec), sizeof(ElemT), alignof(ElemT)))
static TPH_POISSON_INLINE bool tph_poisson_vec_invariants_impl(const tph_poisson_vec *vec,
  const size_t elem_size,
  const size_t elem_align)
{
  bool valid = (vec->mem == NULL);
  valid &= (vec->mem_size == 0);
  valid &= (vec->begin == NULL);
  valid &= (vec->end == NULL);
  if (valid) { return true; } /* Zero-initialized. */
  /* Zero or more elements in vector (non-zero capacity). */
  valid = (vec->mem != NULL);
  valid &= (vec->mem_size > 0);
  valid &= (elem_size > 0);
  valid &= (elem_align > 0);
  valid &= ((intptr_t)vec->mem <= (intptr_t)vec->begin);
  valid &= ((intptr_t)vec->begin <= (intptr_t)vec->end);
  valid &= ((intptr_t)vec->end <= (intptr_t)vec->mem + vec->mem_size);
  valid &= (((uintptr_t)vec->end - (uintptr_t)vec->begin) % elem_size == 0);
  valid &= ((uintptr_t)vec->begin % elem_align == 0);
  valid &= ((uintptr_t)vec->end % elem_align == 0);
  valid &= (((uintptr_t)vec->begin - (uintptr_t)vec->mem) < elem_align);
  return valid;
}

/**
 * @brief Frees all memory associated with the vector.
 * @param vec   Vector.
 * @param alloc Allocator.
 */
static TPH_POISSON_INLINE void tph_poisson_vec_free(tph_poisson_vec *vec,
  tph_poisson_allocator *alloc)
{
  alloc->free(vec->mem, vec->mem_size, alloc->ctx);
  TPH_POISSON_MEMSET((void *)vec, 0, sizeof(tph_poisson_vec));
}

/**
 * @brief Returns the number of elements in the vector.
 * @param ElemT Vector element type.
 * @param vec   Vector.
 * @return The number of elements in the vector.
 */
#define tph_poisson_vec_size(ElemT, vec) (tph_poisson_vec_size_impl((vec), sizeof(ElemT)))
static TPH_POISSON_INLINE ptrdiff_t tph_poisson_vec_size_impl(const tph_poisson_vec *vec,
  const size_t elem_size)
{
  return (ptrdiff_t)((uintptr_t)vec->begin == (uintptr_t)vec->end
                       ? 0
                       : ((uintptr_t)vec->end - (uintptr_t)vec->begin) / elem_size);
}

/**
 * @brief Add elements to the end of the vector.
 * @param ElemT  Vector element type.
 * @param vec    Vector.
 * @param alloc  Allocator.
 * @param values Pointer to values to be added.
 * @param count  Number of elements to copy from values.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
#define tph_poisson_vec_append(ElemT, vec, alloc, values, count) \
  (tph_poisson_vec_append_impl((vec), (alloc), (values), (count), sizeof(ElemT), alignof(ElemT)))
static int tph_poisson_vec_append_impl(tph_poisson_vec *vec,
  tph_poisson_allocator *alloc,
  const void *values,
  const ptrdiff_t count,
  const size_t elem_size,
  const size_t elem_align)
{
  if (((int)(values == NULL) | (int)(count <= 0)) != 0) { return TPH_POISSON_INVALID_ARGS; }
  const ptrdiff_t n = count * (ptrdiff_t)elem_size;
  if (vec->mem_size == 0) {
    /* No existing memory buffer. */
    const ptrdiff_t mem_size = n + (ptrdiff_t)elem_align;
    void *mem = alloc->malloc(mem_size, alloc->ctx);
    if (mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    vec->mem = mem;
    vec->mem_size = mem_size;
    vec->begin = tph_poisson_align(vec->mem, elem_align);
    vec->end = vec->begin;
  } else if ((intptr_t)vec->end + n > (intptr_t)vec->mem + vec->mem_size) {
    /* Try doubling the vector capacity and check if it's enough to hold
     * the new elements. If not, set the new capacity to exactly hold the
     * new elements. */
    const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;
    const ptrdiff_t req_mem_size = size + n + (ptrdiff_t)elem_align;
    ptrdiff_t new_mem_size = vec->mem_size;
    if (new_mem_size <= (PTRDIFF_MAX >> 1)) { new_mem_size <<= 1; }
    if (new_mem_size < req_mem_size) { new_mem_size = req_mem_size; }
    void *new_mem = alloc->malloc(new_mem_size, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *new_begin = tph_poisson_align(new_mem, elem_align);
    if (size > 0) { TPH_POISSON_MEMCPY(new_begin, vec->begin, (size_t)size); }
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    vec->mem = new_mem;
    vec->mem_size = new_mem_size;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)vec->begin + size);
  }
  tph_poisson_assert((intptr_t)vec->end + n <= (intptr_t)vec->mem + vec->mem_size);
  TPH_POISSON_MEMCPY(vec->end, values, (size_t)n);
  vec->end = (void *)((intptr_t)vec->end + n);
  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Erase elements from the vector, starting at index pos erasing count number of elements.
 * @param vec       Vector.
 * @param pos       First element to be erased; pos < 0 is always invalid.
 * @param count     Number of elements to erase; count <= 0 is always invalid.
 * @param elem_size Size (in bytes) of an element.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
#define tph_poisson_vec_erase_unordered(ElemT, vec, pos, count) \
  (tph_poisson_vec_erase_unordered_impl((vec), (pos), (count), sizeof(ElemT)))
static int tph_poisson_vec_erase_unordered_impl(tph_poisson_vec *vec,
  const ptrdiff_t pos,
  const ptrdiff_t count,
  const size_t elem_size)
{
  ptrdiff_t n = count * (ptrdiff_t)elem_size;
  const intptr_t er_begin = (intptr_t)vec->begin + pos * (ptrdiff_t)elem_size;
  const intptr_t er_end = (intptr_t)er_begin + n;

  /* clang-format off */
  bool invalid_args = (er_begin == er_end);          /* count == 0 */
  invalid_args |= (er_begin < (intptr_t)vec->begin); /* pos < 0 */
  invalid_args |= (er_begin >= (intptr_t)vec->end);  /* pos >= size */
  if (invalid_args) { return TPH_POISSON_INVALID_ARGS; }
  /* clang-format on */

  if (er_end >= (intptr_t)vec->end) {
    /* There are no elements behind the specified range. No re-ordering is necessary, simply
     * decrease the size of the vector. */
    vec->end = (void *)er_begin;
  } else {
    /* Copy elements from the end of the vector into the positions to be erased and then decrease
     * the size of the vector. */
    intptr_t src = (intptr_t)vec->end - n;
    if (src < er_end) {
      src = er_end;
      n = (intptr_t)vec->end - src;
    }

    TPH_POISSON_MEMCPY((void *)er_begin, (const void *)src, (size_t)n);
    vec->end = (void *)((intptr_t)vec->end - (er_end - er_begin));
  }
  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Requests the removal of unused capacity.
 * @param ElemT Vector element type.
 * @param vec   Vector.
 * @param alloc Allocator.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
#define tph_poisson_vec_shrink_to_fit(ElemT, vec, alloc) \
  (tph_poisson_vec_shrink_to_fit_impl((vec), (alloc), sizeof(ElemT), alignof(ElemT)))
static int tph_poisson_vec_shrink_to_fit_impl(tph_poisson_vec *vec,
  tph_poisson_allocator *alloc,
  const size_t elem_size,
  const size_t elem_align)
{
  if (vec->end == vec->begin) {
    /* Empty vector, no elements. Wipe everything!
     * This includes the case of a zero-initialized vector. */
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    TPH_POISSON_MEMSET((void *)vec, 0, sizeof(tph_poisson_vec));
  } else if ((intptr_t)vec->mem + vec->mem_size - (intptr_t)vec->end >= (ptrdiff_t)elem_size) {
    /* There is enough additional memory to add one or more elements, shrink the buffer. */
    const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;
    const ptrdiff_t new_mem_size = size + (ptrdiff_t)elem_align;
    void *new_mem = alloc->malloc(new_mem_size, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *new_begin = tph_poisson_align(new_mem, elem_align);
    TPH_POISSON_MEMCPY(new_begin, vec->begin, (size_t)size);
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    vec->mem = new_mem;
    vec->mem_size = new_mem_size;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)vec->begin + size);
  }
  return TPH_POISSON_SUCCESS;
}

/*
 * STRUCTS
 */

struct tph_poisson_sampling_internal_
{
  tph_poisson_allocator alloc;
  void *mem;
  ptrdiff_t mem_size;

  tph_poisson_vec samples; /** ElemT = tph_poisson_real */
};

typedef struct tph_poisson_context_
{
  void *mem;
  ptrdiff_t mem_size;

  tph_poisson_real radius; /** No two samples are closer to each other than the radius. */
  int32_t ndims; /** Number of dimensions, typically 2 or 3. */
  uint32_t max_sample_attempts; /** Maximum attempts when spawning samples from existing ones. */
  tph_poisson_real *bounds_min; /** Hyper-rectangle lower bound. */
  tph_poisson_real *bounds_max; /** Hyper-rectangle upper bound. */
  tph_poisson_xoshiro256p_state prng_state; /** Pseudo-random number generator state. */

  tph_poisson_vec active_indices; /** ElemT = ptrdiff_t */

  tph_poisson_real grid_dx; /** Uniform cell extent. */
  tph_poisson_real grid_dx_rcp; /** 1 / dx */
  ptrdiff_t *grid_size; /** Number of grid cells in each dimension. */
  ptrdiff_t *grid_stride; /** Strides in each dimension, used to compute linear index. */
  uint32_t *grid_cells; /** Grid cells storing indices to points inside them. */

  /* Arrays of size ndims. Pre-allocated in the context to provide 'scratch' variables that are used
   * during the creation of a sampling, but don't need to be stored afterwards. */
  tph_poisson_real *sample;
  ptrdiff_t *grid_index;
  ptrdiff_t *min_grid_index;
  ptrdiff_t *max_grid_index;
} tph_poisson_context;

/**
 * @brief Returns an allocated instance of sampling internal data. If allocator is NULL,
 * a default allocator is used. The instance must be free'd using the free function that
 * it stores (see tph_poisson_destroy).
 * @param alloc Allocator.
 * @return A dynamically allocated initialized instance of sampling internal data.
 */
static tph_poisson_sampling_internal *tph_poisson_alloc_internal(const tph_poisson_allocator *alloc)
{
  tph_poisson_assert(!alloc || (alloc && ((alloc->malloc != NULL) & (alloc->free != NULL))));

  const ptrdiff_t mem_size =
    (ptrdiff_t)(sizeof(tph_poisson_sampling_internal) + alignof(tph_poisson_sampling_internal));
  tph_poisson_malloc_fn malloc_fn =
    alloc != NULL ? alloc->malloc : tph_poisson_default_alloc.malloc;
  void *alloc_ctx = alloc != NULL ? alloc->ctx : tph_poisson_default_alloc.ctx;
  void *mem = malloc_fn(mem_size, alloc_ctx);
  if (mem == NULL) { return NULL; }
  TPH_POISSON_MEMSET(mem, 0, (size_t)mem_size);
  void *aligned_mem = tph_poisson_align(mem, alignof(tph_poisson_sampling_internal));
  tph_poisson_sampling_internal *internal = (tph_poisson_sampling_internal *)aligned_mem;
  internal->alloc.malloc = malloc_fn;
  internal->alloc.free = alloc != NULL ? alloc->free : tph_poisson_default_alloc.free;
  internal->alloc.ctx = alloc_ctx;
  internal->mem = mem;
  internal->mem_size = mem_size;
  return internal;
}

/**
 * @brief Initialize the context using the provided allocator and arguments. Sets up the
 * data structures needed to perform a single run, but that don't need to be kept alive
 * after the run has been completed.
 * @param ctx   Context.
 * @param alloc Allocator.
 * @param args  Arguments.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
static int tph_poisson_context_init(const tph_poisson_allocator *alloc,
  const tph_poisson_args *args,
  tph_poisson_context *ctx)
{
  /* clang-format off */
  bool valid_args = (args != NULL);
  if (!valid_args) { return TPH_POISSON_INVALID_ARGS; }
  valid_args &= (args->radius > 0);
  valid_args &= (args->ndims > 0);
  valid_args &= (args->max_sample_attempts > 0);
  valid_args &= (args->bounds_min != NULL);
  valid_args &= (args->bounds_max != NULL);
  if (!valid_args) { return TPH_POISSON_INVALID_ARGS; }
  for (int32_t i = 0; i < args->ndims; ++i) {
    valid_args &= (args->bounds_max[i] > args->bounds_min[i]);
  }
  if (!valid_args) { return TPH_POISSON_INVALID_ARGS; }
  /* clang-format on */

  ctx->radius = args->radius;
  ctx->ndims = args->ndims;
  ctx->max_sample_attempts = args->max_sample_attempts;

  /* Use a slightly smaller radius to avoid numerical issues. */
  ctx->grid_dx =
    ((tph_poisson_real)0.999 * ctx->radius) / TPH_POISSON_SQRT((tph_poisson_real)ctx->ndims);
  ctx->grid_dx_rcp = (tph_poisson_real)1 / ctx->grid_dx;

  /* Seed pseudo-random number generator. */
  tph_poisson_xoshiro256p_init(&ctx->prng_state, args->seed);

  /* Compute grid linear size so that we know how much memory to allocate for grid cells. */
  ptrdiff_t grid_linear_size = 1;
  for (int32_t i = 0; i < ctx->ndims; ++i) {
    /* Not checking for overflow! */
    grid_linear_size *=
      (ptrdiff_t)TPH_POISSON_CEIL((args->bounds_max[i] - args->bounds_min[i]) * ctx->grid_dx_rcp);
  }

  /* clang-format off */
  ctx->mem_size = 
    /* bounds_min, bounds_max, sample */ 
    (ptrdiff_t)(ctx->ndims * 3) * (ptrdiff_t)sizeof(tph_poisson_real) 
      + (ptrdiff_t)alignof(tph_poisson_real) +
    /* grid_index, min_grid_index, max_grid_index, grid.size, grid.stride*/
    (ptrdiff_t)(ctx->ndims * 5) * (ptrdiff_t)sizeof(ptrdiff_t) 
      + (ptrdiff_t)alignof(ptrdiff_t) +  
    /* grid.cells */         
    grid_linear_size * (ptrdiff_t)sizeof(uint32_t) + (ptrdiff_t)alignof(uint32_t); 
  ctx->mem = alloc->malloc(ctx->mem_size, alloc->ctx);
  /* clang-format on */
  if (ctx->mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
  TPH_POISSON_MEMSET(ctx->mem, 0, (size_t)ctx->mem_size);

  /* Initialize context pointers. Make sure alignment is correct. */
  void *ptr = ctx->mem;

#ifdef TPH_POISSON_CTX_ALLOC
#error "TPH_POISSON_CTX_ALLOC already defined!"
#endif
  /* clang-format off */
#define TPH_POISSON_CTX_ALLOC(type, count, var)                                           \
  do {                                                                                    \
    tph_poisson_assert((uintptr_t)ptr < (uintptr_t)ctx->mem + (size_t)ctx->mem_size);     \
    ptr = ((var) = (type *)ptr) + (count);                                                \
    tph_poisson_assert((uintptr_t)(var) + (size_t)((ptrdiff_t)sizeof(type) * (count) - 1) \
                       < (uintptr_t)ctx->mem + (size_t)ctx->mem_size);                    \
    tph_poisson_assert(((uintptr_t)(var) & (alignof(type) - 1)) == 0);                    \
  } while (0)
  /* clang-format on */

  ptr = tph_poisson_align(ptr, alignof(ptrdiff_t));
  TPH_POISSON_CTX_ALLOC(ptrdiff_t, ctx->ndims, ctx->grid_index);
  TPH_POISSON_CTX_ALLOC(ptrdiff_t, ctx->ndims, ctx->min_grid_index);
  TPH_POISSON_CTX_ALLOC(ptrdiff_t, ctx->ndims, ctx->max_grid_index);
  TPH_POISSON_CTX_ALLOC(ptrdiff_t, ctx->ndims, ctx->grid_size);
  TPH_POISSON_CTX_ALLOC(ptrdiff_t, ctx->ndims, ctx->grid_stride);
  ptr = tph_poisson_align(ptr, alignof(tph_poisson_real));
  TPH_POISSON_CTX_ALLOC(tph_poisson_real, ctx->ndims, ctx->bounds_min);
  TPH_POISSON_CTX_ALLOC(tph_poisson_real, ctx->ndims, ctx->bounds_max);
  TPH_POISSON_CTX_ALLOC(tph_poisson_real, ctx->ndims, ctx->sample);
  ptr = tph_poisson_align(ptr, alignof(uint32_t));
  TPH_POISSON_CTX_ALLOC(uint32_t, grid_linear_size, ctx->grid_cells);
#undef TPH_POISSON_CTX_ALLOC

  /* Copy bounds into context memory buffer to improve locality. */
  TPH_POISSON_MEMCPY(
    ctx->bounds_min, args->bounds_min, (size_t)(ctx->ndims * (ptrdiff_t)sizeof(tph_poisson_real)));
  TPH_POISSON_MEMCPY(
    ctx->bounds_max, args->bounds_max, (size_t)(ctx->ndims * (ptrdiff_t)sizeof(tph_poisson_real)));

  /* Initialize grid size and stride. */
  ctx->grid_size[0] =
    (ptrdiff_t)TPH_POISSON_CEIL((ctx->bounds_max[0] - ctx->bounds_min[0]) * ctx->grid_dx_rcp);
  ctx->grid_stride[0] = 1;
  for (int32_t i = 1; i < ctx->ndims; ++i) {
    ctx->grid_size[i] =
      (ptrdiff_t)TPH_POISSON_CEIL((ctx->bounds_max[i] - ctx->bounds_min[i]) * ctx->grid_dx_rcp);
    ctx->grid_stride[i] = ctx->grid_stride[i - 1] * ctx->grid_size[i - 1];
  }

  /* Initialize cells with sentinel value 0xFFFFFFFF, indicating no sample there.
   * Cell values are later set to sample indices. */
  TPH_POISSON_MEMSET(
    ctx->grid_cells, 0xFF, (size_t)(grid_linear_size * (ptrdiff_t)sizeof(uint32_t)));

  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Frees all memory allocated by the context.
 * @param ctx Context.
 */
static void tph_poisson_context_destroy(tph_poisson_context *ctx, tph_poisson_allocator *alloc)
{
  tph_poisson_assert(ctx && alloc);
  tph_poisson_vec_free(&ctx->active_indices, alloc);
  alloc->free(ctx->mem, ctx->mem_size, alloc->ctx);
}

/**
 * @brief Returns true if p is element-wise inclusively inside b_min and b_max; otherwise false.
 * Assumes that b_min is element-wise less than b_max.
 * @param p     Position to test.
 * @param b_min Minimum bounds.
 * @param b_max Maximum bounds.
 * @param ndims Number of values in p, b_min, and b_max.
 * @return Non-zero if p is element-wise inclusively inside the bounded region; otherwise zero.
 */
static bool tph_poisson_inside(const tph_poisson_real *p,
  const tph_poisson_real *b_min,
  const tph_poisson_real *b_max,
  const int32_t ndims)
{
  /* Note returns true if ndims <= 0.  */
  bool inside = true;
  for (int32_t i = 0; i < ndims; ++i) {
    /* No early exit, always check all dims. */
    tph_poisson_assert(b_min[i] < b_max[i]);
    inside &= (p[i] >= b_min[i]);
    inside &= (p[i] <= b_max[i]);
  }
  return inside;
}

/**
 * @brief Add a sample, which is assumed here to fulfill all the Poisson requirements. Updates the
 * necessary internal data structures and the context.
 * @param ctx      Context.
 * @param internal Internal data.
 * @param sample   Sample to add.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
static int tph_poisson_add_sample(tph_poisson_context *ctx,
  tph_poisson_sampling_internal *internal,
  const tph_poisson_real *sample)
{
  tph_poisson_assert(tph_poisson_inside(sample, ctx->bounds_min, ctx->bounds_max, ctx->ndims));
  tph_poisson_assert(tph_poisson_vec_size(tph_poisson_real, &internal->samples) % ctx->ndims == 0);
  const ptrdiff_t sample_index =
    tph_poisson_vec_size(tph_poisson_real, &internal->samples) / ctx->ndims;
  if ((uint32_t)sample_index == 0xFFFFFFFF) {
    /* The sample index cannot be the same as the sentinel value of the grid. */
    return TPH_POISSON_OVERFLOW;
  }

  tph_poisson_assert(tph_poisson_vec_invariants(tph_poisson_real, &internal->samples));
  int ret = tph_poisson_vec_append(
    tph_poisson_real, &internal->samples, &internal->alloc, sample, ctx->ndims);
  tph_poisson_assert(tph_poisson_vec_invariants(tph_poisson_real, &internal->samples));
  if (ret != TPH_POISSON_SUCCESS) { return ret; }
  tph_poisson_assert(tph_poisson_vec_invariants(ptrdiff_t, &ctx->active_indices));
  ret = tph_poisson_vec_append(ptrdiff_t,
    &ctx->active_indices,
    &internal->alloc,
    &sample_index,
    /*count=*/1);
  tph_poisson_assert(tph_poisson_vec_invariants(ptrdiff_t, &ctx->active_indices));
  if (ret != TPH_POISSON_SUCCESS) { return ret; }

  /* Compute linear grid index. */
  tph_poisson_assert(ctx->grid_stride[0] == 1);
  ptrdiff_t xi = (ptrdiff_t)TPH_POISSON_FLOOR((sample[0] - ctx->bounds_min[0]) * ctx->grid_dx_rcp);
  tph_poisson_assert((0 <= xi) & (xi < ctx->grid_size[0]));
  ptrdiff_t k = xi;
  for (int32_t i = 1; i < ctx->ndims; ++i) {
    xi = (ptrdiff_t)TPH_POISSON_FLOOR((sample[i] - ctx->bounds_min[i]) * ctx->grid_dx_rcp);
    tph_poisson_assert((0 <= xi) & (xi < ctx->grid_size[i]));
    /* Not checking for overflow! */
    k += xi * ctx->grid_stride[i];
  }

  /* Record sample index in grid. Each grid cell can hold up to one sample,
   * and once a cell has been assigned a sample it should not be updated.
   * It is assumed here that the cell has its sentinel value before being
   * assigned a sample index. */
  tph_poisson_assert(ctx->grid_cells[k] == 0xFFFFFFFF);
  ctx->grid_cells[k] = (uint32_t)sample_index;
  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Generate a pseudo-random sample position that is guaranteed be at a distance
 * [radius, 2 * radius] from the provided center position.
 * @param ctx    Context.
 * @param center Center position.
 * @param sample Output sample position.
 */
static void tph_poisson_rand_annulus_sample(tph_poisson_context *ctx,
  const tph_poisson_real *center,
  tph_poisson_real *sample)
{
  int32_t i = 0;
  tph_poisson_real sqr_mag = 0;
  for (;;) {
    /* Generate a random component in the range [-2, 2] for each dimension.
     * Use sample storage to temporarily store components. */
    sqr_mag = 0;
    for (i = 0; i < ctx->ndims; ++i) {
      /* clang-format off */
      sample[i] = (tph_poisson_real)(-2 + 4 * tph_poisson_to_double(
          tph_poisson_xoshiro256p_next(&ctx->prng_state)));
      /* clang-format on */
      sqr_mag += sample[i] * sample[i];
    }

    /* The randomized offset is not guaranteed to be within the radial
     * distance that we need to guarantee. If we found an offset with
     * magnitude in the range (1, 2] we are done, otherwise generate a new
     * offset. Continue until a valid offset is found. */
    if (((int)((tph_poisson_real)1 < sqr_mag) & (int)(sqr_mag <= (tph_poisson_real)4)) == 1) {
      /* Found a valid offset.
       * Add the offset scaled by radius to the center coordinate to
       * produce the final sample. */
      for (i = 0; i < ctx->ndims; ++i) { sample[i] = center[i] + ctx->radius * sample[i]; }
      break;
    }
  }
}

/**
 * @brief Computes the grid index range in which the provided sample position needs to check for
 * other samples that are possible closer than the radius (given in context).
 * @param ctx            Context.
 * @param sample         Input sample position.
 * @param min_grid_index Minimum grid index.
 * @param max_grid_index Maximum grid index.
 */
static void tph_poisson_grid_index_bounds(tph_poisson_context *ctx,
  const tph_poisson_real *sample,
  ptrdiff_t *min_grid_index,
  ptrdiff_t *max_grid_index)
{
#ifdef TPH_POISSON_GRID_CLAMP
#error "TPH_POISSON_GRID_CLAMP already defined!"
#endif
  /* clang-format off */
#define TPH_POISSON_GRID_CLAMP(grid_index, grid_size) \
  if ((grid_index) < 0) {                             \
    (grid_index) = 0;                                 \
  } else if ((grid_index) >= (grid_size)) {           \
    (grid_index) = (grid_size) - 1;                   \
  }
  /* clang-format on */

  tph_poisson_real si = 0;
  for (int32_t i = 0; i < ctx->ndims; ++i) {
    tph_poisson_assert(ctx->grid_size[i] > 0);
    si = sample[i] - ctx->bounds_min[i];
    min_grid_index[i] = (ptrdiff_t)TPH_POISSON_FLOOR((si - ctx->radius) * ctx->grid_dx_rcp);
    max_grid_index[i] = (ptrdiff_t)TPH_POISSON_FLOOR((si + ctx->radius) * ctx->grid_dx_rcp);
    TPH_POISSON_GRID_CLAMP(min_grid_index[i], ctx->grid_size[i]);
    TPH_POISSON_GRID_CLAMP(max_grid_index[i], ctx->grid_size[i]);
  }
#undef TPH_POISSON_GRID_CLAMP
}

/**
 * @brief Returns true if there exists another sample within the radius used to
 * construct the grid; otherwise false.
 * @param ctx                 Context.
 * @param sample              Input sample position.
 * @param active_sample_index Index of the existing sample that 'spawned' the sample tested here.
 * @param min_grid_index      Minimum grid index.
 * @param max_grid_index      Maximum grid index.
 */
static bool tph_poisson_existing_sample_within_radius(tph_poisson_context *ctx,
  const tph_poisson_vec *samples,
  const tph_poisson_real *sample,
  const ptrdiff_t active_sample_index,
  const ptrdiff_t *min_grid_index,
  const ptrdiff_t *max_grid_index)
{
  const tph_poisson_real r_sqr = ctx->radius * ctx->radius;
  tph_poisson_real di = 0;
  tph_poisson_real d_sqr = -1;
  const tph_poisson_real *cell_sample = NULL;
  int32_t i = -1;
  ptrdiff_t k = -1;
  bool test_cell = false;
  const int32_t ndims = ctx->ndims;
  TPH_POISSON_MEMCPY(
    ctx->grid_index, min_grid_index, (size_t)(ndims * (ptrdiff_t)sizeof(ptrdiff_t)));
  do {
    /* Compute linear grid index. */
    tph_poisson_assert((0 <= ctx->grid_index[0]) & (ctx->grid_index[0] < ctx->grid_size[0]));
    k = ctx->grid_index[0];
    for (i = 1; i < ndims; ++i) {
      /* Not checking for overflow! */
      tph_poisson_assert((0 <= ctx->grid_index[i]) & (ctx->grid_index[i] < ctx->grid_size[i]));
      k += ctx->grid_index[i] * ctx->grid_stride[i];
    }

    test_cell = (ctx->grid_cells[k] != 0xFFFFFFFF);
    test_cell &= (ctx->grid_cells[k] != (uint32_t)active_sample_index);
    if (test_cell) {
      /* Compute (squared) distance to the existing sample and then check if the existing sample is
       * closer than (squared) radius to the provided sample. */
      cell_sample =
        (const tph_poisson_real *)samples->begin + (ptrdiff_t)ctx->grid_cells[k] * ndims;
      di = sample[0] - cell_sample[0];
      d_sqr = di * di;
      for (i = 1; i < ndims; ++i) {
        di = sample[i] - cell_sample[i];
        d_sqr += di * di;
      }
      if (d_sqr < r_sqr) { return true; }
    }

    /* Iterate over grid index range. Enumerate every grid index between min_grid_index and
     * max_grid_index (inclusive) exactly once. Assumes that min_index is element-wise less than or
     * equal to max_index. */
    for (i = 0; i < ndims; ++i) {
      tph_poisson_assert(min_grid_index[i] <= max_grid_index[i]);
      ctx->grid_index[i]++;
      if (ctx->grid_index[i] <= max_grid_index[i]) { break; }
      ctx->grid_index[i] = min_grid_index[i];
    }
    /* If the above loop ran to completion, without triggering the break, the grid_index has been
     * set to its original value (min_grid_index). Since this was the starting value for grid_index
     * we exit the outer loop when this happens. */
  } while (i != ndims);

  /* No existing sample was found to be closer to the provided sample than the radius. */
  return false;
}

/**
 * @brief Generate a random sample within the given bounds (in context).
 * @param ctx    Context.
 * @param sample Output sample.
 */
static void tph_poisson_rand_sample(tph_poisson_context *ctx, tph_poisson_real *sample)
{
  int32_t i = 0;
  for (; i < ctx->ndims; ++i) {
    tph_poisson_assert(ctx->bounds_max[i] > ctx->bounds_min[i]);
    sample[i] =
      ctx->bounds_min[i]
      + (tph_poisson_real)(tph_poisson_to_double(tph_poisson_xoshiro256p_next(&ctx->prng_state)))
          * (ctx->bounds_max[i] - ctx->bounds_min[i]);
    /* Clamp to avoid numerical issues. */
    if (sample[i] < ctx->bounds_min[i]) {
      sample[i] = ctx->bounds_min[i];
    } else if (sample[i] > ctx->bounds_max[i]) {
      sample[i] = ctx->bounds_max[i];
    }
  }
}

int tph_poisson_create(const tph_poisson_args *args,
  const tph_poisson_allocator *alloc,
  tph_poisson_sampling *sampling)
{
  /* Allocator must provide all functions, allocator context is optional (may be null). */
  if (sampling == NULL) { return TPH_POISSON_INVALID_ARGS; }
  if (alloc != NULL && ((int)(alloc->malloc == NULL) | (int)(alloc->free == NULL)) == 1) {
    return TPH_POISSON_INVALID_ARGS;
  }

  /* Allocate internal data. */
  if (sampling->internal != NULL) { tph_poisson_destroy(sampling); }
  sampling->internal = tph_poisson_alloc_internal(alloc);
  if (sampling->internal == NULL) { return TPH_POISSON_BAD_ALLOC; }
  tph_poisson_sampling_internal *internal = sampling->internal;

  /* Initialize context. Validates arguments and allocates buffers. */
  tph_poisson_context ctx;
  TPH_POISSON_MEMSET(&ctx, 0, sizeof(tph_poisson_context));
  int ret = tph_poisson_context_init(&internal->alloc, args, &ctx);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy context here. */
    tph_poisson_destroy(sampling);
    return ret;
  }

  /* Add first sample randomly within bounds. No need to check (non-existing) neighbors. */
  tph_poisson_rand_sample(&ctx, ctx.sample);
  ret = tph_poisson_add_sample(&ctx, internal, ctx.sample);
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_context_destroy(&ctx, &internal->alloc);
    tph_poisson_destroy(sampling);
    return ret;
  }

  tph_poisson_assert(tph_poisson_vec_size(ptrdiff_t, &ctx.active_indices) == 1);
  ptrdiff_t active_index_count = 1;
  ptrdiff_t rand_index = -1;
  ptrdiff_t active_sample_index = -1;
  const tph_poisson_real *active_sample = NULL;
  uint32_t attempt_count = 0;
  while (active_index_count > 0) {
    /* Randomly choose an active sample. A sample is considered active until failed attempts
     * have been made to generate a new sample within its annulus. */
    rand_index =
      (ptrdiff_t)(tph_poisson_xoshiro256p_next(&ctx.prng_state) % (uint64_t)active_index_count);
    active_sample_index = *((const ptrdiff_t *)ctx.active_indices.begin + rand_index);
    active_sample =
      (const tph_poisson_real *)internal->samples.begin + active_sample_index * ctx.ndims;
    attempt_count = 0;
    while (attempt_count < ctx.max_sample_attempts) {
      /* Randomly create a candidate sample inside the active sample's annulus. */
      tph_poisson_rand_annulus_sample(&ctx, active_sample, ctx.sample);
      /* Check if candidate sample is within bounds. */
      if (tph_poisson_inside(ctx.sample, ctx.bounds_min, ctx.bounds_max, ctx.ndims)) {
        tph_poisson_grid_index_bounds(&ctx, ctx.sample, ctx.min_grid_index, ctx.max_grid_index);
        if (!tph_poisson_existing_sample_within_radius(&ctx,
              &internal->samples,
              ctx.sample,
              active_sample_index,
              ctx.min_grid_index,
              ctx.max_grid_index)) {
          /* No existing samples where found to be too close to the
           * candidate sample, no further attempts necessary. */
          ret = tph_poisson_add_sample(&ctx, internal, ctx.sample);
          if (ret != TPH_POISSON_SUCCESS) {
            tph_poisson_context_destroy(&ctx, &internal->alloc);
            tph_poisson_destroy(sampling);
            return ret;
          }
          break;
        }
        /* else: The candidate sample is too close to an existing sample. */
      }
      /* else: The candidate sample is out-of-bounds. */
      ++attempt_count;
    }

    if (attempt_count == args->max_sample_attempts) {
      /* No valid sample was found on the disk of the active sample after
       * maximum number of attempts, remove it from the active list. */
      tph_poisson_assert(tph_poisson_vec_invariants(ptrdiff_t, &ctx.active_indices));
      tph_poisson_vec_erase_unordered(ptrdiff_t, &ctx.active_indices, rand_index, /*count=*/1);
      tph_poisson_assert(tph_poisson_vec_invariants(ptrdiff_t, &ctx.active_indices));
    }
    active_index_count = tph_poisson_vec_size(ptrdiff_t, &ctx.active_indices);
  }

  tph_poisson_assert(tph_poisson_vec_invariants(tph_poisson_real, &internal->samples));
  ret = tph_poisson_vec_shrink_to_fit(tph_poisson_real, &internal->samples, &internal->alloc);
  tph_poisson_assert(tph_poisson_vec_invariants(tph_poisson_real, &internal->samples));
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_context_destroy(&ctx, &internal->alloc);
    tph_poisson_destroy(sampling);
    return ret;
  }

  tph_poisson_assert(tph_poisson_vec_size(tph_poisson_real, &internal->samples) % ctx.ndims == 0);
  sampling->ndims = ctx.ndims;
  sampling->nsamples = tph_poisson_vec_size(tph_poisson_real, &internal->samples) / ctx.ndims;

  tph_poisson_context_destroy(&ctx, &internal->alloc);

  return TPH_POISSON_SUCCESS;
}

void tph_poisson_destroy(tph_poisson_sampling *sampling)
{
  if (sampling != NULL) {
    sampling->ndims = 0;
    sampling->nsamples = 0;
    if (sampling->internal != NULL) {
      tph_poisson_sampling_internal *internal = sampling->internal;
      tph_poisson_vec_free(&internal->samples, &internal->alloc);
      tph_poisson_free_fn free_fn = internal->alloc.free;
      void *alloc_ctx = internal->alloc.ctx;
      free_fn(internal->mem, internal->mem_size, alloc_ctx);

      /* Protects from destroy being called more than once causing a double-free error. */
      sampling->internal = NULL;
    }
  }
}

const tph_poisson_real *tph_poisson_get_samples(tph_poisson_sampling *sampling)
{
  /* Make sure that a 'destroyed' sampling does not return any samples. */
  return (sampling != NULL && sampling->internal != NULL)
           ? (const tph_poisson_real *)sampling->internal->samples.begin
           : NULL;
}

/* Clean up internal macros. */
#undef TPH_POISSON_INLINE
#undef tph_poisson_assert
#undef TPH_POISSON_MEMCPY
#undef TPH_POISSON_MEMSET
#undef TPH_POISSON_MALLOC
#undef TPH_POISSON_FREE

#undef tph_poisson_vec_invariants
#undef tph_poisson_vec_size
#undef tph_poisson_vec_append
#undef tph_poisson_vec_erase_unordered
#undef tph_poisson_vec_shrink_to_fit

#endif// TPH_POISSON_IMPLEMENTATION

/*

ABOUT:

    A single file implementation of Poisson disk sampling in arbitrary dimensions.

HISTORY:

    0.4     2024-08-XYZ  - ...

LICENSE:

    The MIT License (MIT)

    Copyright (c) 2024 Tommy Hinks

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

DISCLAIMER:

    This software is supplied "AS IS" without any warranties and support

USAGE:

    Generates a list of samples with the guarantees:
      (1) No two samples are closer to each other than some radius;
      (2) No sample is outside the region bounds.

    The algorithm tries to fit as many samples as possible into the region without violating the
    above requirements.

    The API consists of these functions:

    int tph_poisson_create(tph_poisson_sampling *sampling,
                           const tph_poisson_args *args,
                           tph_poisson_allocator *alloc);

    void tph_poisson_destroy(tph_poisson_sampling *sampling);

    const tph_poisson_real *tph_poisson_get_samples(tph_poisson_sampling *sampling);

    Example usage:

    #include <assert.h>
    #include <stddef.h>// ptrdiff_t
    #include <stdint.h>// UINT64_C, etc
    #include <stdio.h>// printf
    #include <stdlib.h>// EXIT_FAILURE, etc

    #if 0 // To use double.
    #define TPH_POISSON_REAL_TYPE double
    #include <math.h>
    #define TPH_POISSON_SQRT  sqrt
    #define TPH_POISSON_CEIL  ceil
    #define TPH_POISSON_FLOOR floor
    #endif
    #define TPH_POISSON_IMPLEMENTATION
    #include "thinks/tph_poisson.h"

    int main(int argc, char *argv[])
    {
      (void)argc;
      (void)argv;

      const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-100, (tph_poisson_real)-100 };
      const tph_poisson_real bounds_max[2] = { (tph_poisson_real)100, (tph_poisson_real)100 };
      tph_poisson_args args = { NULL };
      args.bounds_min = bounds_min;
      args.bounds_max = bounds_max;
      args.radius = (tph_poisson_real)10;
      args.ndims = INT32_C(2);
      args.max_sample_attempts = UINT32_C(30);
      args.seed = UINT64_C(1981);

      tph_poisson_sampling sampling = { NULL };
      tph_poisson_allocator *alloc = NULL;
      int ret = tph_poisson_create(&args, alloc, &sampling);
      if (ret != TPH_POISSON_SUCCESS) {
        // No need to destroy sampling here!
        printf("Failed creating Poisson sampling! Error code: %d", ret);
        return EXIT_FAILURE;
      }

      const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
      assert(samples);

      // Print sample positions.
      for (ptrdiff_t i = 0; i < sampling.nsamples; ++i) {
        printf("sample[%td] = ( %.3f, %.3f )\n",
          i,
          samples[i * sampling.ndims],
          samples[i * sampling.ndims + 1]);
      }

      // Free memory.
      tph_poisson_destroy(&sampling);

      return EXIT_SUCCESS;
    }

 */

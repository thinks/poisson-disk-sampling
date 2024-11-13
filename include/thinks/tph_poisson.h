/*
 * Copyright(c) 2024 Tommy Hinks
 * For LICENSE (MIT), USAGE, and HISTORY see the end of this file.
 */

#ifndef TPH_POISSON_H
#define TPH_POISSON_H

#define TPH_POISSON_MAJOR_VERSION 0
#define TPH_POISSON_MINOR_VERSION 4
#define TPH_POISSON_PATCH_VERSION 0

#include <stddef.h> /* size_t, ptrdiff_t, NULL */
#include <stdint.h> /* uint32_t, uintptr_t, etc */

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

typedef TPH_POISSON_REAL_TYPE tph_poisson_real;

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
extern const tph_poisson_real *tph_poisson_get_samples(const tph_poisson_sampling *sampling);

/* END PUBLIC API ----------------------------------------------------------- */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TPH_POISSON_H */

/* BEGIN IMPLEMENTATION ------------------------------------------------------*/

#ifdef TPH_POISSON_IMPLEMENTATION
#undef TPH_POISSON_IMPLEMENTATION

#include <stdalign.h> /* alignof */
#include <stdbool.h> /* bool, true, false */

#if defined(_MSC_VER) && !defined(__cplusplus)
#define TPH_POISSON_INLINE __inline
#else
#define TPH_POISSON_INLINE inline
#endif

#ifndef TPH_POISSON_ASSERT
#include <assert.h>
#define TPH_POISSON_ASSERT(_X_) assert((_X_))
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
 * that the address is a multiple of alignment.
 * @param ptr       Pointer to align.
 * @param alignment Requested alignment. Assumed to be a (non-zero) power of two.
 * @return An aligned pointer.
 */
static TPH_POISSON_INLINE void *tph_poisson_align(void *const ptr, const size_t alignment)
{
  TPH_POISSON_ASSERT((alignment > 0) & ((alignment & (alignment - 1)) == 0));
  return (void *)(((uintptr_t)ptr + (alignment - 1)) & ~(alignment - 1));
}

static tph_poisson_allocator tph_poisson_default_alloc = { tph_poisson_malloc,
  tph_poisson_free,
  /*.ctx=*/NULL };

/*
 * PSEUDO-RANDOM NUMBER GENERATION
 */

typedef struct tph_poisson_splitmix64_state_
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

/* clang-format off */
typedef struct tph_poisson_vec_
{
  ptrdiff_t mem_size; /** Size of memory buffer in bytes. */
  void *mem;          /** Memory buffer. */
  void *begin;        /** Pointer to first element, or NULL if empty. */
  void *end;          /** Pointer to first byte after last element, or NULL if empty. */
} tph_poisson_vec;
/* clang-format on */

/**
 * @brief Frees all memory associated with the vector.
 * @param vec   Vector.
 * @param alloc Allocator.
 */
static TPH_POISSON_INLINE void tph_poisson_vec_free(tph_poisson_vec *vec,
  const tph_poisson_allocator *alloc)
{
  TPH_POISSON_ASSERT(vec != NULL);
  TPH_POISSON_ASSERT(alloc != NULL);
  if (((int)(vec->mem != NULL) & (int)(vec->mem_size > 0)) == 1) {
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
  }
}

/**
 * @brief Returns the number of bytes in the vector.
 * @param vec Vector.
 * @return The number of bytes in the vector.
 */
static TPH_POISSON_INLINE ptrdiff_t tph_poisson_vec_size(const tph_poisson_vec *vec)
{
  TPH_POISSON_ASSERT((intptr_t)vec->end >= (intptr_t)vec->begin);
  return (ptrdiff_t)((intptr_t)vec->end - (intptr_t)vec->begin);
}

/**
 * @brief Increase the capacity of the vector (the total number of elements that the vector can hold
 * without requiring reallocation) to a value that's greater or equal to new_cap. If new_cap is
 * greater than the current capacity, new storage is allocated, otherwise the function does
 * nothing.
 * @param vec       Vector.
 * @param alloc     Allocator.
 * @param new_cap   Minimum number of elements that can be stored without requiring reallocation.
 * @param alignment Alignment of objects intended to be stored in the vector.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
static int tph_poisson_vec_reserve(tph_poisson_vec *vec,
  const tph_poisson_allocator *alloc,
  const ptrdiff_t new_cap,
  const ptrdiff_t alignment)
{
  TPH_POISSON_ASSERT(vec != NULL);
  TPH_POISSON_ASSERT(alloc != NULL);
  TPH_POISSON_ASSERT(new_cap > 0);
  TPH_POISSON_ASSERT(alignment > 0);

  /* NOTE: For zero-initialized vector cap is 0. */
  const ptrdiff_t cap = vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem);
  if (new_cap <= cap) { return TPH_POISSON_SUCCESS; }

  /* Allocate and align a new buffer with sufficient capacity. Take into account that
   * the memory returned by the allocator may not match the requested alignment. */
  const ptrdiff_t new_mem_size = new_cap + alignment;
  void *const new_mem = alloc->malloc(new_mem_size, alloc->ctx);
  if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
  void *const new_begin = tph_poisson_align(new_mem, (size_t)alignment);

  const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;

  /* Copy existing data (if any) to the new buffer. */
  if (size > 0) {
    TPH_POISSON_ASSERT(vec->begin != NULL);
    TPH_POISSON_MEMCPY(new_begin, vec->begin, (size_t)size);
  }

  /* Destroy the old buffer (if any). */
  if (vec->mem_size > 0) {
    TPH_POISSON_ASSERT(vec->mem != NULL);
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);
  }

  /* Configure vector to use the new buffer. */
  vec->mem = new_mem;
  vec->mem_size = new_mem_size;
  vec->begin = new_begin;
  vec->end = (void *)((intptr_t)new_begin + size);

  TPH_POISSON_ASSERT(vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem) >= new_cap);

  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Add elements to the end of the vector.
 * @param vec       Vector.
 * @param alloc     Allocator.
 * @param buf       Pointer to values to be added. Assumed to be non-null.
 * @param n         Number of bytes to copy from values. Assumed to be > 0.
 * @param alignment Alignment of objects intended to be stored in the vector.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
static int tph_poisson_vec_append(tph_poisson_vec *vec,
  const tph_poisson_allocator *alloc,
  const void *buf,
  const ptrdiff_t n,
  const ptrdiff_t alignment)
{
  TPH_POISSON_ASSERT(vec != NULL);
  TPH_POISSON_ASSERT(alloc != NULL);
  TPH_POISSON_ASSERT(buf != NULL);
  TPH_POISSON_ASSERT(n > 0);
  TPH_POISSON_ASSERT(alignment > 0);

  const ptrdiff_t cap = vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem);
  const ptrdiff_t req_cap = (intptr_t)vec->end - (intptr_t)vec->begin + n;
  if (req_cap > cap) {
    /* Current buffer does not have enough capacity. Try doubling the vector
     * capacity and check if it's enough to hold the new elements. If not,
     * set the new capacity to hold exactly the new elements. */
    ptrdiff_t new_cap = cap <= (PTRDIFF_MAX >> 1) ? (cap << 1) : cap;
    if (new_cap < req_cap) { new_cap = req_cap; }

    /* Allocate and align a new buffer with sufficient capacity. Take into
     * account that the memory returned by the allocator may not be aligned to
     * the type of element that will be stored. */
    new_cap += alignment;
    void *new_mem = alloc->malloc(new_cap, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *new_begin = tph_poisson_align(new_mem, (size_t)alignment);

    const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;

    /* Copy existing data (if any) to the new buffer. */
    if (size > 0) {
      TPH_POISSON_ASSERT(vec->begin != NULL);
      TPH_POISSON_MEMCPY(new_begin, vec->begin, (size_t)size);
    }

    /* Destroy the old buffer (if any). */
    if (vec->mem_size > 0) {
      TPH_POISSON_ASSERT(vec->mem != NULL);
      alloc->free(vec->mem, vec->mem_size, alloc->ctx);
    }

    /* Configure vector to use the new buffer. */
    vec->mem = new_mem;
    vec->mem_size = new_cap;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)new_begin + size);
  }
  TPH_POISSON_ASSERT(vec->mem_size - ((intptr_t)vec->begin - (intptr_t)vec->mem) >= req_cap);
  TPH_POISSON_ASSERT((intptr_t)vec->end % alignment == 0);

  TPH_POISSON_MEMCPY(vec->end, buf, (size_t)n);
  vec->end = (void *)((intptr_t)vec->end + n);
  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Erase an element from the vector using a "swap & pop" approach. Note that this may
 * re-order bytes.
 * @param vec   Vector.
 * @param pos   Position to be erased. Assuming 0 <= pos and pos < vector size.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
static void tph_poisson_vec_erase_swap(tph_poisson_vec *vec, const ptrdiff_t pos, const ptrdiff_t n)
{
  TPH_POISSON_ASSERT(vec != NULL);
  TPH_POISSON_ASSERT(pos >= 0);
  TPH_POISSON_ASSERT((intptr_t)vec->begin + pos < (intptr_t)vec->end);
  TPH_POISSON_ASSERT(n >= 1);
  TPH_POISSON_ASSERT((intptr_t)vec->begin + pos + n <= (intptr_t)vec->end);

  const intptr_t dst_begin = (intptr_t)vec->begin + pos;
  const intptr_t dst_end = dst_begin + n;
  intptr_t src_begin = (intptr_t)vec->end - n;
  if (src_begin < dst_end) { src_begin = dst_end; }
  const intptr_t m = (intptr_t)vec->end - src_begin;
  if (m > 0) { TPH_POISSON_MEMCPY((void *)dst_begin, (const void *)src_begin, (size_t)m); }
  /* else: when erasing up to the last element there is no need to copy anything,
   *       just "pop" the end of the vector. */

  /* "pop" the end of the buffer, decreasing the vector size by n. */
  vec->end = (void *)((intptr_t)vec->end - n);
}

/**
 * @brief Requests the removal of unused capacity.
 * @param vec   Vector.
 * @param alloc Allocator.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
static int tph_poisson_vec_shrink_to_fit(tph_poisson_vec *vec,
  const tph_poisson_allocator *alloc,
  const ptrdiff_t alignment)
{
  TPH_POISSON_ASSERT(vec != NULL);
  TPH_POISSON_ASSERT(alloc != NULL);
  TPH_POISSON_ASSERT(alignment > 0);

  const ptrdiff_t size = (intptr_t)vec->end - (intptr_t)vec->begin;
  if (size == 0) {
    /* Empty vector, no elements. Wipe capacity!
     * This includes the case of a zero-initialized vector. */
    if (vec->mem != NULL) {
      /* Existing vector is empty but has capacity. */
      TPH_POISSON_ASSERT(vec->mem_size > 0);
      alloc->free(vec->mem, vec->mem_size, alloc->ctx);
      TPH_POISSON_MEMSET((void *)vec, 0, sizeof(tph_poisson_vec));
    }
    TPH_POISSON_ASSERT(vec->mem == NULL);
    TPH_POISSON_ASSERT(vec->mem_size == 0);
    return TPH_POISSON_SUCCESS;
  }

  /* Check if allocating a new buffer (size + alignment) would be smaller than
   * the existing buffer. */
  TPH_POISSON_ASSERT(vec->mem_size > alignment);
  const ptrdiff_t new_mem_size = size + alignment;
  if (vec->mem_size > new_mem_size) {
    /* Allocate and align a new buffer with sufficient capacity. Take into
     * account that the memory returned by the allocator may not be aligned to
     * the type of element that will be stored. */
    void *const new_mem = alloc->malloc(new_mem_size, alloc->ctx);
    if (new_mem == NULL) { return TPH_POISSON_BAD_ALLOC; }
    void *const new_begin = tph_poisson_align(new_mem, (size_t)alignment);

    /* Copy existing data to the new buffer and destroy the old buffer. */
    TPH_POISSON_MEMCPY(new_begin, vec->begin, (size_t)size);
    alloc->free(vec->mem, vec->mem_size, alloc->ctx);

    /* Configure vector to use the new buffer. */
    vec->mem = new_mem;
    vec->mem_size = new_mem_size;
    vec->begin = new_begin;
    vec->end = (void *)((intptr_t)new_begin + size);
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
  ptrdiff_t grid_linear_size; /** Total number of grid cells. */
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
  TPH_POISSON_ASSERT(!alloc || (alloc && ((alloc->malloc != NULL) & (alloc->free != NULL))));

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
  ctx->grid_linear_size = 1;
  for (int32_t i = 0; i < ctx->ndims; ++i) {
    /* Not checking for overflow! */
    ctx->grid_linear_size *=
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
    ctx->grid_linear_size * (ptrdiff_t)sizeof(uint32_t) + (ptrdiff_t)alignof(uint32_t); 
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
    TPH_POISSON_ASSERT((uintptr_t)ptr < (uintptr_t)ctx->mem + (size_t)ctx->mem_size);     \
    ptr = ((var) = (type *)ptr) + (count);                                                \
    TPH_POISSON_ASSERT((uintptr_t)(var) + (size_t)((ptrdiff_t)sizeof(type) * (count) - 1) \
                       < (uintptr_t)ctx->mem + (size_t)ctx->mem_size);                    \
    TPH_POISSON_ASSERT(((uintptr_t)(var) & (alignof(type) - 1)) == 0);                    \
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
  TPH_POISSON_CTX_ALLOC(uint32_t, ctx->grid_linear_size, ctx->grid_cells);
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
    ctx->grid_cells, 0xFF, (size_t)(ctx->grid_linear_size * (ptrdiff_t)sizeof(uint32_t)));

  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Frees all memory allocated by the context.
 * @param ctx Context.
 */
static void tph_poisson_context_destroy(tph_poisson_context *ctx, tph_poisson_allocator *alloc)
{
  TPH_POISSON_ASSERT(ctx && alloc);
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
    TPH_POISSON_ASSERT(b_min[i] < b_max[i]);
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
  TPH_POISSON_ASSERT(tph_poisson_inside(sample, ctx->bounds_min, ctx->bounds_max, ctx->ndims));
  TPH_POISSON_ASSERT(
    tph_poisson_vec_size(&internal->samples) % ((ptrdiff_t)sizeof(tph_poisson_real) * ctx->ndims)
    == 0);
  const ptrdiff_t sample_index =
    tph_poisson_vec_size(&internal->samples) / ((ptrdiff_t)sizeof(tph_poisson_real) * ctx->ndims);
  if ((uint32_t)sample_index == 0xFFFFFFFF) {
    /* The sample index cannot be the same as the sentinel value of the grid. */
    return TPH_POISSON_OVERFLOW;
  }

  int ret = tph_poisson_vec_append(&internal->samples,
    &internal->alloc,
    sample,
    (ptrdiff_t)sizeof(tph_poisson_real) * ctx->ndims,
    (ptrdiff_t)alignof(tph_poisson_real));
  if (ret != TPH_POISSON_SUCCESS) { return ret; }
  ret = tph_poisson_vec_append(&ctx->active_indices,
    &internal->alloc,
    &sample_index,
    (ptrdiff_t)sizeof(ptrdiff_t),
    (ptrdiff_t)alignof(ptrdiff_t));
  if (ret != TPH_POISSON_SUCCESS) { return ret; }

  /* Compute linear grid index. */
  TPH_POISSON_ASSERT(ctx->grid_stride[0] == 1);
  ptrdiff_t xi = (ptrdiff_t)TPH_POISSON_FLOOR((sample[0] - ctx->bounds_min[0]) * ctx->grid_dx_rcp);
  TPH_POISSON_ASSERT((0 <= xi) & (xi < ctx->grid_size[0]));
  ptrdiff_t k = xi;
  for (int32_t i = 1; i < ctx->ndims; ++i) {
    xi = (ptrdiff_t)TPH_POISSON_FLOOR((sample[i] - ctx->bounds_min[i]) * ctx->grid_dx_rcp);
    TPH_POISSON_ASSERT((0 <= xi) & (xi < ctx->grid_size[i]));
    /* Not checking for overflow! */
    k += xi * ctx->grid_stride[i];
  }

  /* Record sample index in grid. Each grid cell can hold up to one sample,
   * and once a cell has been assigned a sample it should not be updated.
   * It is assumed here that the cell has its sentinel value before being
   * assigned a sample index. */
  TPH_POISSON_ASSERT(ctx->grid_cells[k] == 0xFFFFFFFF);
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
    TPH_POISSON_ASSERT(ctx->grid_size[i] > 0);
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
    TPH_POISSON_ASSERT((0 <= ctx->grid_index[0]) & (ctx->grid_index[0] < ctx->grid_size[0]));
    k = ctx->grid_index[0];
    for (i = 1; i < ndims; ++i) {
      /* Not checking for overflow! */
      TPH_POISSON_ASSERT((0 <= ctx->grid_index[i]) & (ctx->grid_index[i] < ctx->grid_size[i]));
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
      TPH_POISSON_ASSERT(min_grid_index[i] <= max_grid_index[i]);
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
    TPH_POISSON_ASSERT(ctx->bounds_max[i] > ctx->bounds_min[i]);
    sample[i] =
      ctx->bounds_min[i]
      + (tph_poisson_real)(tph_poisson_to_double(tph_poisson_xoshiro256p_next(&ctx->prng_state)))
          * (ctx->bounds_max[i] - ctx->bounds_min[i]);
    /* Clamp to avoid numerical issues. */
    /* clang-format off */
    sample[i] = sample[i] < ctx->bounds_min[i] ? ctx->bounds_min[i] 
      : (ctx->bounds_max[i] < sample[i] ? ctx->bounds_max[i] : sample[i]);
    /* clang-format on */
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

  /* Heuristically reserve some memory for samples to avoid reallocations while
   * growing the buffer. Estimate that 25% of the grid cells will end up
   * containing a sample, which is a fairly conservative guess. Prefering not
   * to over-allocate up front here, at the cost of having to reallocate later. */
  ret = tph_poisson_vec_reserve(&internal->samples,
    &internal->alloc,
    (ctx.grid_linear_size / 4) * ((ptrdiff_t)sizeof(tph_poisson_real) * ctx.ndims),
    (ptrdiff_t)alignof(tph_poisson_real));
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_context_destroy(&ctx, &internal->alloc);
    tph_poisson_destroy(sampling);
    return ret;
  }

  /* Reserve memory for active indices, could use some analysis to find a
   * better estimate here... */
  ret = tph_poisson_vec_reserve(&ctx.active_indices,
    &internal->alloc,
    100 * (ptrdiff_t)sizeof(ptrdiff_t),
    (ptrdiff_t)alignof(ptrdiff_t));
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_context_destroy(&ctx, &internal->alloc);
    tph_poisson_destroy(sampling);
    return ret;
  }

  /* Add first sample randomly within bounds. No need to check (non-existing) neighbors. */
  tph_poisson_rand_sample(&ctx, ctx.sample);
  ret = tph_poisson_add_sample(&ctx, internal, ctx.sample);
  TPH_POISSON_ASSERT(ret == TPH_POISSON_SUCCESS);
  (void)ret;

  TPH_POISSON_ASSERT(tph_poisson_vec_size(&ctx.active_indices) / (ptrdiff_t)sizeof(ptrdiff_t) == 1);
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
      tph_poisson_vec_erase_swap(&ctx.active_indices,
        rand_index * (ptrdiff_t)sizeof(ptrdiff_t),
        (ptrdiff_t)sizeof(ptrdiff_t));
    }
    active_index_count = tph_poisson_vec_size(&ctx.active_indices) / (ptrdiff_t)sizeof(ptrdiff_t);
  }

  ret = tph_poisson_vec_shrink_to_fit(
    &internal->samples, &internal->alloc, (ptrdiff_t)alignof(tph_poisson_real));
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_context_destroy(&ctx, &internal->alloc);
    tph_poisson_destroy(sampling);
    return ret;
  }

  const ptrdiff_t sample_size = (ptrdiff_t)sizeof(tph_poisson_real) * ctx.ndims;
  TPH_POISSON_ASSERT(tph_poisson_vec_size(&internal->samples) % sample_size == 0);
  sampling->ndims = ctx.ndims;
  sampling->nsamples = tph_poisson_vec_size(&internal->samples) / sample_size;

  tph_poisson_context_destroy(&ctx, &internal->alloc);

  return TPH_POISSON_SUCCESS;
}

void tph_poisson_destroy(tph_poisson_sampling *sampling)
{
  if (sampling != NULL) {
    tph_poisson_sampling_internal *internal = sampling->internal;
    if (internal != NULL) {
      tph_poisson_vec_free(&internal->samples, &internal->alloc);
      tph_poisson_free_fn free_fn = internal->alloc.free;
      void *alloc_ctx = internal->alloc.ctx;
      free_fn(internal->mem, internal->mem_size, alloc_ctx);
    }
    /* Protects from destroy being called more than once causing a double-free error. */
    TPH_POISSON_MEMSET(sampling, 0, sizeof(tph_poisson_sampling));
  }
}

const tph_poisson_real *tph_poisson_get_samples(const tph_poisson_sampling *sampling)
{
  /* Make sure that a 'destroyed' sampling does not return any samples. */
  if (sampling != NULL && sampling->internal != NULL) {
    return (const tph_poisson_real *)sampling->internal->samples.begin;
  }
  return NULL;
}

/* Clean up internal macros. */
#undef TPH_POISSON_INLINE
#undef TPH_POISSON_ASSERT
#undef TPH_POISSON_MEMCPY
#undef TPH_POISSON_MEMSET
#undef TPH_POISSON_MALLOC
#undef TPH_POISSON_FREE

#endif /* TPH_POISSON_IMPLEMENTATION */

/*

ABOUT:

    A single file implementation of Poisson disk sampling in arbitrary dimensions.

HISTORY:

    0.4.0    2024-11-13 - C interface and implementation, new build system.
    v0.3     2020-06-30 - C++ interface and implementation.

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

    int tph_poisson_create(const tph_poisson_args *args,
                           const tph_poisson_allocator *alloc,
                           tph_poisson_sampling *sampling);

    void tph_poisson_destroy(tph_poisson_sampling *sampling);

    const tph_poisson_real *tph_poisson_get_samples(const tph_poisson_sampling *sampling);

    Example usage:

    #include <assert.h>
    #include <stddef.h> // ptrdiff_t
    #include <stdint.h> // UINT64_C, etc
    #include <stdio.h> // printf
    #include <stdlib.h> // EXIT_FAILURE, etc
    #include <string.h> // memset

    #define TPH_POISSON_IMPLEMENTATION
    #include "thinks/tph_poisson.h"

    int main(int argc, char *argv[])
    {
      (void)argc;
      (void)argv;

      const tph_poisson_real bounds_min[2] = { (tph_poisson_real)-100, (tph_poisson_real)-100 };
      const tph_poisson_real bounds_max[2] = { (tph_poisson_real)100, (tph_poisson_real)100 };
      const tph_poisson_args args = { .bounds_min = bounds_min,
        .bounds_max = bounds_max,
        .radius = (tph_poisson_real)3,
        .ndims = INT32_C(2),
        .max_sample_attempts = UINT32_C(30),
        .seed = UINT64_C(1981) };

      tph_poisson_allocator *alloc = NULL;

      tph_poisson_sampling sampling;
      memset(&sampling, 0, sizeof(tph_poisson_sampling));

      int ret = tph_poisson_create(&args, alloc, &sampling);
      if (ret != TPH_POISSON_SUCCESS) {
        // No need to destroy sampling here!
        printf("Failed creating Poisson sampling! Error code: %d", ret);
        return EXIT_FAILURE;
      }

      const tph_poisson_real *samples = tph_poisson_get_samples(&sampling);
      if (samples == NULL) {
        // Shouldn't happen since we check the return value from tph_poisson_create!
        printf("Bad samples!\n");
        tph_poisson_destroy(&sampling);
        return EXIT_FAILURE;
      }

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

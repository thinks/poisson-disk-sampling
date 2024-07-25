#ifndef TPH_POISSON_H
#define TPH_POISSON_H

#define TPH_POISSON_MAJOR_VERSION 0
#define TPH_POISSON_MINOR_VERSION 4
#define TPH_POISSON_PATCH_VERSION 0

#include <stddef.h>// size_t, ptrdiff_t, NULL
#include <stdint.h>// uint32_t, etc

#ifdef __cplusplus
extern "C" {
#endif

#ifndef tph_poisson_assert
#include <assert.h>
#define tph_poisson_assert(_X_) assert((_X_))
#endif

#ifndef TPH_POISSON_REAL_TYPE
#define TPH_POISSON_REAL_TYPE float
#endif

#ifndef TPH_POISSON_SQRT
#include <math.h>
#define TPH_POISSON_SQRT(_X_) sqrtf(_X_)
#endif

#ifndef TPH_POISSON_CEIL
#include <math.h>
#define TPH_POISSON_CEIL(_X_) ceilf(_X_)
#endif

#ifndef TPH_POISSON_FLOOR
#include <math.h>
#define TPH_POISSON_FLOOR(_X_) floorf(_X_)
#endif

#ifndef TPH_POISSON_MEMCPY
#include <string.h>
#define TPH_POISSON_MEMCPY(dst, src, n) memcpy((dst), (src), (size_t)(n))
#endif

#ifndef TPH_POISSON_MEMSET
#include <string.h>
#define TPH_POISSON_MEMSET(s, c, n) memset((s), (c), (size_t)(n))
#endif

typedef TPH_POISSON_REAL_TYPE tph_poisson_real;

/* clang-format off */
typedef struct tph_poisson_args_      tph_poisson_args;
typedef struct tph_poisson_allocator_ tph_poisson_allocator;
typedef struct tph_poisson_sampling_  tph_poisson_sampling;
/* clang-format on */

#pragma pack(push, 1)

struct tph_poisson_allocator_
{
  void *(*malloc)(ptrdiff_t size, void *ctx);
  void *(*realloc)(void *ptr, ptrdiff_t old_size, ptrdiff_t new_size, void *ctx);
  void (*free)(void *ptr, ptrdiff_t size, void *ctx);
  void *ctx;
};

struct tph_poisson_args_
{
  tph_poisson_real radius;
  int32_t ndims;
  const tph_poisson_real *bounds_min;
  const tph_poisson_real *bounds_max;
  uint32_t max_sample_attempts;
  uint64_t seed;
};

struct tph_poisson_sampling_
{
  int32_t ndims;
  ptrdiff_t nsamples;
  tph_poisson_real *samples;
  tph_poisson_allocator *alloc;
};

#pragma pack(pop)

/* clang-format off */
#define TPH_POISSON_SUCCESS       0
#define TPH_POISSON_BAD_ALLOC     1
#define TPH_POISSON_INVALID_ARGS  2
#define TPH_POISSON_BAD_ALLOCATOR 3
#define TPH_POISSON_OVERFLOW      4
/* clang-format on */

/**
 * @brief ...
 */
extern int32_t tph_poisson_create(tph_poisson_sampling *sampling,
  const tph_poisson_args *args,
  tph_poisson_allocator *alloc);

/**
 * @brief ...
 */
extern void tph_poisson_destroy(tph_poisson_sampling *sampling);

#ifdef __cplusplus
}// extern "C"
#endif

#endif// TPH_POISSON_H

#ifdef TPH_POISSON_IMPLEMENTATION
#undef TPH_POISSON_IMPLEMENTATION

#include <stdlib.h>// malloc, realloc, free

#define TPH_POISSON_SIZEOF(type) ((ptrdiff_t)sizeof(type))

#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline __inline
#endif

/* Memory. */

static void *tph_poisson_malloc(ptrdiff_t size, void *ctx)
{
  (void)ctx;
  return size > 0 ? malloc((size_t)size) : (void *)NULL;
}

static void *tph_poisson_realloc(void *ptr, ptrdiff_t old_size, ptrdiff_t new_size, void *ctx)
{
  (void)ctx;
  (void)old_size;
  return new_size > 0 ? realloc(ptr, (size_t)new_size) : (void *)NULL;
}

static void tph_poisson_free(void *ptr, ptrdiff_t size, void *ctx)
{
  (void)size;
  (void)ctx;
  free(ptr);
}

static tph_poisson_allocator tph_poisson_default_alloc = { tph_poisson_malloc,
  tph_poisson_realloc,
  tph_poisson_free,
  /*ctx=*/(void *)NULL };

/* Pseudo-random number generation. */

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

/* Misc. */

/**
 * @brief Returns a floating point number in [0..1).
 * @param x Bit representation.
 * @return A number in [0..1).
 */
static inline double tph_poisson_to_double(const uint64_t x)
{
  /* Convert to double, as suggested at https://prng.di.unimi.it.
     This conversion prefers the high bits of x (usually, a good idea). */
  return (double)(x >> 11) * 0x1.0p-53;
}

/**
 * @brief Returns the provided value clamped to the range [x0, x1].
 * Assumes x0 <= x1.
 * @param x0 Lower bound.
 * @param x1 Upper bound.
 * @param x  Value.
 * @return Value clamped to the range [x0, x1].
 */
#define TPH_POISSON_CLAMP(x0, x1, x) ((x) < (x0) ? (x0) : ((x1) < (x) ? (x1) : (x)))

/* Vector. */

typedef struct tph_poisson_vec_header_
{
  ptrdiff_t capacity;
  ptrdiff_t size;
} tph_poisson_vec_header;

/**
 * @brief Declares a vector.
 * @param type The type of elements in the vector, e.g. int, float, etc.
 */
#define tph_poisson_vec(type) type *

/**
 * @brief Converts a header to a vector. Assumes that the header is not NULL.
 * @param hdr - Header.
 * @return The vector as a <void*>.
 * @internal
 */
static inline void *tph_poisson_hdr_to_vec(const tph_poisson_vec_header *hdr)
{
  tph_poisson_assert(hdr);
  return (void *)&(hdr[1]);
}

/**
 * @brief Converts a vector to a header. Assumes that the vector is not NULL.
 * @param vec - Vector.
 * @return A header pointer.
 * @internal
 */
static inline tph_poisson_vec_header *tph_poisson_vec_to_hdr(const void *vec)
{
  tph_poisson_assert(vec);
  return &((tph_poisson_vec_header *)vec)[-1];
}

/**
 * @brief Frees all memory associated with the vector.
 * @param vec - Vector.
 * @return void
 */
static void tph_poisson_vec_free(void *vec, tph_poisson_allocator *alloc)
{
  if (vec) {
    tph_poisson_vec_header *hdr = tph_poisson_vec_to_hdr(vec);
    alloc->free(
      (void *)hdr, TPH_POISSON_SIZEOF(tph_poisson_vec_header) + hdr->capacity, alloc->ctx);
  }
}

/**
 * @brief Returns the number of elements in the vector.
 * @param vec Vector.
 * @return The number of elements in the vector.
 */
#define tph_poisson_vec_size(vec) (_tph_poisson_vec_size((vec), TPH_POISSON_SIZEOF(*(vec))))
static inline ptrdiff_t _tph_poisson_vec_size(const void *vec, const ptrdiff_t sizeof_elem)
{
  return vec ? tph_poisson_vec_to_hdr(vec)->size / sizeof_elem : (ptrdiff_t)0;
}

/**
 * @brief Add elements to the end of the vector.
 * @param vec    Vector.
 * @param values Pointer to values to be added.
 * @param count  Number of elements to copy from values.
 * @param alloc  Allocator.
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
#define tph_poisson_vec_append(vec, values, count, alloc) \
  (_tph_poisson_vec_append(                               \
    (void **)&(vec), (values), (count) * TPH_POISSON_SIZEOF(*(values)), (alloc)))
static int _tph_poisson_vec_append(void **vec_ptr,
  const void *values,
  const ptrdiff_t nbytes,
  tph_poisson_allocator *alloc)
{
  if (*vec_ptr) {
    /* Existing vector. */
    tph_poisson_vec_header *hdr = tph_poisson_vec_to_hdr(*vec_ptr);
    const ptrdiff_t req_cap = hdr->size + nbytes;
    if (hdr->capacity < req_cap) {
      /* Try doubling the vector capacity and check if it's enough to hold
       * the new elements. */
      ptrdiff_t new_cap = hdr->capacity ? hdr->capacity : (ptrdiff_t)1;
      if (new_cap <= (PTRDIFF_MAX >> 1)) { new_cap <<= 1; }
      if (new_cap < req_cap) { new_cap = req_cap; }
      tph_poisson_vec_header *new_hdr = (tph_poisson_vec_header *)alloc->realloc((void *)hdr,
        /*old_size=*/TPH_POISSON_SIZEOF(tph_poisson_vec_header) + hdr->capacity,
        /*new_size=*/TPH_POISSON_SIZEOF(tph_poisson_vec_header) + new_cap,
        alloc->ctx);
      if (!new_hdr) { return TPH_POISSON_BAD_ALLOC; }
      hdr = new_hdr;
      hdr->capacity = new_cap;
      *vec_ptr = tph_poisson_hdr_to_vec(hdr);
    }
    TPH_POISSON_MEMCPY((char *)*vec_ptr + hdr->size, values, nbytes);
    hdr->size += nbytes;
  } else {
    /* Initialize a new vector. */
    tph_poisson_vec_header *hdr = (tph_poisson_vec_header *)(alloc->malloc(
      TPH_POISSON_SIZEOF(tph_poisson_vec_header) + nbytes, alloc->ctx));
    if (!hdr) { return TPH_POISSON_BAD_ALLOC; }
    hdr->capacity = nbytes;
    hdr->size = nbytes;
    *vec_ptr = tph_poisson_hdr_to_vec(hdr);
    TPH_POISSON_MEMCPY(*vec_ptr, values, nbytes);
  }
  return TPH_POISSON_SUCCESS;
}

/**
 * @brief ...
 * @param vec   Vector.
 * @param pos   Starting index of elements to be erased; pos <= 0 is always a no-op.
 * @param count Number of elements to erase; count <= 0 is always a no-op.
 */
#define tph_poisson_vec_erase_unordered(vec, pos, count) \
  (_tph_poisson_vec_erase_unordered(                     \
    (vec), (pos) * TPH_POISSON_SIZEOF(*(vec)), (count) * TPH_POISSON_SIZEOF(*(vec))))
static void _tph_poisson_vec_erase_unordered(void *vec, ptrdiff_t pos, ptrdiff_t count)
{
  if ((vec != NULL) & ((pos >= 0) & (count > 0))) {
    tph_poisson_vec_header *hdr = tph_poisson_vec_to_hdr(vec);
    if (pos + count >= hdr->size) {
      /* There are no elements behind the specified range.
       * No re-ordering is necessary, simply decrease the size of the vector. */
      hdr->size = pos;
    } else {
      ptrdiff_t n_src = count;
      ptrdiff_t remain = hdr->size - pos - count;
      if (n_src > remain) { n_src = remain; }
      TPH_POISSON_MEMCPY(&((char *)vec)[pos], &((char *)vec)[hdr->size - n_src], n_src);
      hdr->size -= count;
    }
  }
}

// Structs.

#pragma pack(push, 1)

typedef struct tph_poisson_context_
{
  tph_poisson_allocator *alloc;
  char *mem;
  ptrdiff_t mem_size;

  tph_poisson_real radius;
  int32_t ndims;
  uint32_t max_sample_attempts;
  tph_poisson_real *bounds_min;
  tph_poisson_real *bounds_max;
  tph_poisson_xoshiro256p_state prng_state;

  tph_poisson_vec(tph_poisson_real) samples_vec;
  tph_poisson_vec(ptrdiff_t) active_indices_vec;

  tph_poisson_real grid_dx; /** Uniform cell extent. */
  tph_poisson_real grid_dx_rcp; /** 1 / dx */
  /* ptrdiff_t linear_size; ?? */
  ptrdiff_t *grid_size; /** Number of grid cells in each dimension. */
  ptrdiff_t *grid_stride; /** strides ... */
  uint32_t *grid_cells; /** cells ... */

  /* Arrays of size ndims. Pre-allocated in the context to provide 'scratch' variables. */
  tph_poisson_real *sample;
  ptrdiff_t *grid_index;
  ptrdiff_t *min_grid_index;
  ptrdiff_t *max_grid_index;
} tph_poisson_context;

#pragma pack(pop)

/**
 * @brief
 * @param ctx
 */
static int tph_poisson_context_init(tph_poisson_context *ctx,
  const tph_poisson_real radius,
  const int32_t ndims,
  const tph_poisson_real *bounds_min,
  const tph_poisson_real *bounds_max,
  const uint32_t max_sample_attempts,
  const uint64_t seed,
  tph_poisson_allocator *alloc)
{
  ctx->alloc = alloc;
  if (!ctx->alloc) { ctx->alloc = &tph_poisson_default_alloc; }
  if (!((ctx->alloc->malloc != NULL) & (ctx->alloc->realloc != NULL)
        & (ctx->alloc->free != NULL))) {
    /* Allocator must provide functions, allocator context is optional (may be null). */
    return TPH_POISSON_BAD_ALLOCATOR;
  }

  /* clang-format off */
  if (!((radius > 0) & (ndims > 0) & (max_sample_attempts > 0) & 
        (bounds_min != NULL) & (bounds_max != NULL))) {
    return TPH_POISSON_INVALID_ARGS;
  }
  for (int32_t i = 0; i < ndims; ++i) {
    if (!(bounds_max[i] > bounds_min[i])) { return TPH_POISSON_INVALID_ARGS; }
  }
  /* clang-format on */

  ctx->radius = radius;
  ctx->ndims = ndims;
  ctx->max_sample_attempts = max_sample_attempts;

  /* Use a slightly smaller radius to avoid numerical issues. */
  ctx->grid_dx =
    ((tph_poisson_real)0.999 * ctx->radius) / TPH_POISSON_SQRT((tph_poisson_real)ctx->ndims);
  ctx->grid_dx_rcp = (tph_poisson_real)1 / ctx->grid_dx;

  /* Seed pseudo-random number generator. */
  tph_poisson_xoshiro256p_init(&ctx->prng_state, seed);

  /* Compute grid linear size so that we know how much memory to allocate for cells. */
  ptrdiff_t grid_linear_size = 1;
  for (int32_t i = 0; i < ctx->ndims; ++i) {
    /* Not checking for overflow! */
    grid_linear_size *=
      (ptrdiff_t)TPH_POISSON_CEIL((bounds_max[i] - bounds_min[i]) * ctx->grid_dx_rcp);
  }

  /* clang-format off */
  ctx->mem_size = ctx->ndims * (
      TPH_POISSON_SIZEOF(tph_poisson_real) + /* ctx.bounds_min */ 
      TPH_POISSON_SIZEOF(tph_poisson_real) + /* ctx.bounds_max */ 
      TPH_POISSON_SIZEOF(tph_poisson_real) + /* ctx.sample */
      TPH_POISSON_SIZEOF(ptrdiff_t)        + /* ctx.grid_index */
      TPH_POISSON_SIZEOF(ptrdiff_t)        + /* ctx.min_grid_index */
      TPH_POISSON_SIZEOF(ptrdiff_t)        + /* ctx.max_grid_index */
      TPH_POISSON_SIZEOF(ptrdiff_t)        + /* ctx.grid.size */
      TPH_POISSON_SIZEOF(ptrdiff_t)          /* ctx.grid.stride */
    ) + grid_linear_size * TPH_POISSON_SIZEOF(uint32_t) /* ctx.grid.cells */
    + 16; /* 16 bytes padding for alignment; */
  ctx->mem = (char*)ctx->alloc->malloc(ctx->mem_size, ctx->alloc->ctx);
  /* clang-format on */
  if (!ctx->mem) { return TPH_POISSON_BAD_ALLOC; }
  TPH_POISSON_MEMSET(ctx->mem, 0, ctx->mem_size);

  /* Could zero-initialize and align (perhaps to sizeof(void*)?) memory here,
   * but it doesn't seem necessary? */

asdasd

  /* Initialize context pointers.
   * Requires intermediate casts to void* to suppress alignment warnings (-Wcast-align). */
  /* clang-format off */
  ptrdiff_t mem_offset = 0;
  ctx->bounds_min      = (tph_poisson_real *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(tph_poisson_real);
  ctx->bounds_max      = (tph_poisson_real *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(tph_poisson_real);
  ctx->sample          = (tph_poisson_real *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(tph_poisson_real);
  ctx->grid_index      = (ptrdiff_t *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(ptrdiff_t);
  ctx->min_grid_index  = (ptrdiff_t *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(ptrdiff_t);
  ctx->max_grid_index  = (ptrdiff_t *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(ptrdiff_t);
  ctx->grid_size       = (ptrdiff_t *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(ptrdiff_t);
  ctx->grid_stride     = (ptrdiff_t *)(void*)&ctx->mem[mem_offset];
  mem_offset          += ctx->ndims * TPH_POISSON_SIZEOF(ptrdiff_t);
  ctx->grid_cells      = (uint32_t *)(void*)&ctx->mem[mem_offset];
  /* clang-format on */
  tph_poisson_assert(ctx->mem_size == mem_offset + grid_linear_size * TPH_POISSON_SIZEOF(uint32_t));

  /* Copy bounds into context memory buffer to improve locality. */
  TPH_POISSON_MEMCPY(
    ctx->bounds_min, bounds_min, ctx->ndims * TPH_POISSON_SIZEOF(tph_poisson_real));
  TPH_POISSON_MEMCPY(
    ctx->bounds_max, bounds_max, ctx->ndims * TPH_POISSON_SIZEOF(tph_poisson_real));

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
  TPH_POISSON_MEMSET(ctx->grid_cells, 0xFF, grid_linear_size * TPH_POISSON_SIZEOF(uint32_t));

  return TPH_POISSON_SUCCESS;
}

/**
 * @brief Frees all memory allocated by the context.
 * @param ctx Context.
 */
static void tph_poisson_context_destroy(tph_poisson_context *ctx)
{
  if (!ctx) { return; }
  ctx->alloc->free((void *)ctx->mem, ctx->mem_size, ctx->alloc->ctx);
  tph_poisson_vec_free(ctx->samples_vec, ctx->alloc);
  tph_poisson_vec_free(ctx->active_indices_vec, ctx->alloc);
}

#if 0
static void tph_poisson_context_print(const tph_poisson_context *ctx)
{
  printf("Context\n");
  printf("mem_size = %td [bytes]\n", ctx->mem_size);
  printf("radius = %f\n", ctx->radius);
  printf("ndims = %d\n", ctx->ndims);
  printf("max_sample_attempts = %d\n", ctx->max_sample_attempts);
  printf("bounds = { ");
  for (int32_t i = 0; i < ctx->ndims; ++i) {
    printf(i == ctx->ndims - 1 ? "(%.3f, %.3f)" : "(%.3f, %.3f), ",
      ctx->bounds_min[i],
      ctx->bounds_max[i]);
  }
  printf(" }\n");
  printf("samples = { ");
  for (ptrdiff_t j = 0; j < tph_poisson_vec_size(ctx->samples_vec) / ctx->ndims; ++j) {
    printf("(");
    for (int32_t i = 0; i < ctx->ndims; ++i) {
      printf(i == ctx->ndims - 1 ? "%.3f" : "%.3f, ", ctx->samples_vec[j * ctx->ndims + i]);
    }
    printf(j == tph_poisson_vec_size(ctx->samples_vec) / ctx->ndims - 1 ? ")" : "), ");
  }
  printf(" }\n");
  printf("active_indices = { ");
  for (ptrdiff_t j = 0; j < tph_poisson_vec_size(ctx->active_indices_vec); ++j) {
    printf(j == tph_poisson_vec_size(ctx->active_indices_vec) - 1 ? "%td" : "%td, ",
      ctx->active_indices_vec[j]);
  }
  printf(" }\n");
}
#endif

/**
 * @brief Returns non-zero if p is element-wise inclusively inside b_min and b_max; otherwise zero.
 * Assumes that b_min is element-wise less than b_max.
 * @param p     Position to test.
 * @param b_min Minimum bounds.
 * @param b_max Maximum bounds.
 * @param ndims Number of values in p, b_min, and b_max.
 * @return Non-zero if p is element-wise inclusively inside the bounded region; otherwise zero.
 */
static int tph_poisson_inside(const tph_poisson_real *p,
  const tph_poisson_real *b_min,
  const tph_poisson_real *b_max,
  int32_t ndims)
{
  assert(ndims > 0);
  while (ndims--) {
    tph_poisson_assert(b_min[ndims] < b_max[ndims]);
    if (!((b_min[ndims] <= p[ndims]) & (p[ndims] <= b_max[ndims]))) { return 0; }
  }
  return 1;
}

/**
 * @brief
 * @param sample
 * @param ctx
 * @return TPH_POISSON_SUCCESS, or a non-zero error code.
 */
static int tph_poisson_add_sample(const tph_poisson_real *sample, tph_poisson_context *ctx)
{
  tph_poisson_assert(tph_poisson_inside(sample, ctx->bounds_min, ctx->bounds_max, ctx->ndims));
  tph_poisson_assert(tph_poisson_vec_size(ctx->samples_vec) % ctx->ndims == 0);
  const ptrdiff_t sample_index = tph_poisson_vec_size(ctx->samples_vec) / ctx->ndims;
  if ((uint32_t)sample_index == 0xFFFFFFFF) {
    /* The sample index cannot be the same as the sentinel value of the grid. */
    return TPH_POISSON_OVERFLOW;
  }

  int ret = TPH_POISSON_SUCCESS;
  ret = tph_poisson_vec_append(ctx->samples_vec, sample, ctx->ndims, ctx->alloc);
  if (ret != TPH_POISSON_SUCCESS) { return ret; }
  ret = tph_poisson_vec_append(ctx->active_indices_vec, &sample_index, /*count=*/1, ctx->alloc);
  if (ret != TPH_POISSON_SUCCESS) { return ret; }

  /* Compute linear grid index. */
  tph_poisson_assert(ctx->grid_stride[0] == 1);
  ptrdiff_t xi = (ptrdiff_t)TPH_POISSON_FLOOR((sample[0] - ctx->bounds_min[0]) * ctx->grid_dx_rcp);
  tph_poisson_assert((0 <= xi) & (xi < ctx->grid_size[0]));
  ptrdiff_t k = xi;
  for (int32_t i = 1; i < ctx->ndims; ++i) {
    /* Not checking for overflow! */
    xi = (ptrdiff_t)TPH_POISSON_FLOOR((sample[i] - ctx->bounds_min[i]) * ctx->grid_dx_rcp);
    tph_poisson_assert((0 <= xi) & (xi < ctx->grid_size[i]));
    k += xi * ctx->grid_stride[i];
  }

  /*tph_poisson_context_print(ctx);*/

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
 * @param center     Center position.
 * @param ndims      Number of values in center and sample.
 * @param radius     Radius bounding the annulus.
 * @param prng_state Pseudo-random number generator state.
 * @param sample     Output sample position.
 */
static void tph_poisson_rand_annulus_sample(const tph_poisson_real *center,
  tph_poisson_context *ctx,
  tph_poisson_real *sample)
{
  int32_t i = 0;
  tph_poisson_real sqr_mag = 0;
  while (1) {
    /* Generate a random component in the range [-2, 2] for each dimension. */
    sqr_mag = 0;
    for (i = 0; i < ctx->ndims; ++i) {
      sample[i] = (tph_poisson_real)(-2
                                     + 4
                                         * tph_poisson_to_double(
                                           tph_poisson_xoshiro256p_next(&ctx->prng_state)));
      sqr_mag += sample[i] * sample[i];
    }

    /* The randomized offset is not guaranteed to be within the radial
     * distance that we need to guarantee. If we found an offset with
     * magnitude in the range (1, 2] we are done, otherwise generate a new
     * offset. Continue until a valid offset is found. */
    if (((tph_poisson_real)1 < sqr_mag) & (sqr_mag <= (tph_poisson_real)4)) {
      /* Found a valid offset.
       * Add the offset scaled by radius to the center coordinate to
       * produce the final sample. */
      for (i = 0; i < ctx->ndims; ++i) { sample[i] = center[i] + ctx->radius * sample[i]; }
      break;
    }
  }
}

/**
 * @brief ...
 */
static void tph_poisson_grid_index_bounds(const tph_poisson_real *sample,
  tph_poisson_context *ctx,
  ptrdiff_t *min_grid_index,
  ptrdiff_t *max_grid_index)
{
  ptrdiff_t xi = 0;
  ptrdiff_t xi_max = 0;
  for (int32_t i = 0; i < ctx->ndims; ++i) {
    tph_poisson_assert(ctx->grid_size[i] > 0);
    xi_max = ctx->grid_size[i] - 1;
    xi = (ptrdiff_t)TPH_POISSON_FLOOR(
      ((sample[i] - ctx->radius) - ctx->bounds_min[i]) * ctx->grid_dx_rcp);
    min_grid_index[i] = TPH_POISSON_CLAMP(0, xi_max, xi);
    xi = (ptrdiff_t)TPH_POISSON_FLOOR(
      ((sample[i] + ctx->radius) - ctx->bounds_min[i]) * ctx->grid_dx_rcp);
    max_grid_index[i] = TPH_POISSON_CLAMP(0, xi_max, xi);
  }
}

// Returns true if there exists another sample within the radius used to
// construct the grid, otherwise false.
static int tph_poisson_existing_sample_within_radius(const tph_poisson_real *sample,
  const ptrdiff_t active_sample_index,
  tph_poisson_context *ctx,
  const ptrdiff_t *min_grid_index,
  const ptrdiff_t *max_grid_index)
{
  const tph_poisson_real r_sqr = ctx->radius * ctx->radius;
  tph_poisson_real di = 0;
  tph_poisson_real d_sqr = -1;
  uint32_t cell = 0xFFFFFFFF;
  const tph_poisson_real *cell_sample = NULL;
  int32_t i = -1;
  ptrdiff_t k = -1;
  TPH_POISSON_MEMCPY(ctx->grid_index, min_grid_index, ctx->ndims * TPH_POISSON_SIZEOF(ptrdiff_t));
  do {
    /* Compute linear grid index. */
    tph_poisson_assert((0 <= ctx->grid_index[0]) & (ctx->grid_index[0] < ctx->grid_size[0]));
    k = ctx->grid_index[0];
    for (i = 1; i < ctx->ndims; ++i) {
      /* Not checking for overflow! */
      tph_poisson_assert((0 <= ctx->grid_index[i]) & (ctx->grid_index[i] < ctx->grid_size[i]));
      k += ctx->grid_index[i] * ctx->grid_stride[i];
    }

    cell = ctx->grid_cells[k];
    if ((cell != 0xFFFFFFFF) & (cell != (uint32_t)active_sample_index)) {
      /* Compute (squared) distance to the existing sample and then check if the existing sample is
       * closer than radius to the provided sample. */
      cell_sample = &ctx->samples_vec[(ptrdiff_t)cell * ctx->ndims];
      di = sample[0] - cell_sample[0];
      d_sqr = di * di;
      for (i = 1; i < ctx->ndims; ++i) {
        di = sample[i] - cell_sample[i];
        d_sqr += di * di;
      }
      if (d_sqr < r_sqr) { return 1; }
    }

    /* Iterate over grid index range. Enumerate every grid index
     * between min_grid_index and max_grid_index (inclusive) exactly once.
     * Assumes that min_index is element-wise less than or equal to max_index. */
    for (i = 0; i < ctx->ndims; ++i) {
      assert(min_grid_index[i] <= max_grid_index[i]);
      ctx->grid_index[i]++;
      if (ctx->grid_index[i] <= max_grid_index[i]) { break; }
      ctx->grid_index[i] = min_grid_index[i];
    }
    /* If the above loop ran to completion, without triggering the break, the
     * grid_index has been set to its original value (min_grid_index). Since this
     * was the starting value for grid_index we exit the outer loop when this happens. */
  } while (i != ctx->ndims);

  /* No existing samples were found to be closer to the sample than the provided radius. */
  return 0;
}

int tph_poisson_create(tph_poisson_sampling *s,
  const tph_poisson_args *args,
  tph_poisson_allocator *alloc)
{
  if (!((s != NULL) & (args != NULL))) { return TPH_POISSON_INVALID_ARGS; }

  /* Free memory possibly allocated in a previous call. */
  tph_poisson_destroy(s);

  /* Initialize context. Validates arguments and allocates buffers. */
  tph_poisson_context ctx = { 0 };
  int ret = tph_poisson_context_init(&ctx,
    args->radius,
    args->ndims,
    args->bounds_min,
    args->bounds_max,
    args->max_sample_attempts,
    args->seed,
    alloc);
  if (ret != TPH_POISSON_SUCCESS) {
    /* No need to destroy context here. */
    return ret;
  }

  /* Add first sample randomly within bounds. No need to check (non-existing) neighbors. */
  for (int32_t i = 0; i < ctx.ndims; ++i) {
    ctx.sample[i] =
      ctx.bounds_min[i]
      + (tph_poisson_real)(tph_poisson_to_double(tph_poisson_xoshiro256p_next(&ctx.prng_state)))
          * (ctx.bounds_max[i] - ctx.bounds_min[i]);
    ctx.sample[i] = TPH_POISSON_CLAMP(ctx.bounds_min[i], ctx.bounds_max[i], ctx.sample[i]);
  }
  ret = tph_poisson_add_sample(ctx.sample, &ctx);
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_context_destroy(&ctx);
    return ret;
  }

  tph_poisson_assert(tph_poisson_vec_size(ctx.active_indices_vec) == 1);
  ptrdiff_t active_index_count = 1;
  ptrdiff_t active_index = -1;
  const tph_poisson_real *active_sample = NULL;
  uint32_t attempt_count = 0;
  while (active_index_count > 0) {
    /* Randomly choose an active sample. A sample is considered active
       until failed attempts have been made to generate a new sample within
       its annulus. */
    active_index =
      (ptrdiff_t)(tph_poisson_xoshiro256p_next(&ctx.prng_state) % (uint64_t)active_index_count);
    active_sample = &ctx.samples_vec[ctx.active_indices_vec[active_index] * ctx.ndims];
    attempt_count = 0;
    while (attempt_count < ctx.max_sample_attempts) {
      /* Randomly create a candidate sample inside the active sample's annulus. */
      tph_poisson_rand_annulus_sample(active_sample, &ctx, ctx.sample);
      /* Check if candidate sample is within bounds. */
      if (tph_poisson_inside(ctx.sample, ctx.bounds_min, ctx.bounds_max, ctx.ndims)) {
        tph_poisson_grid_index_bounds(ctx.sample, &ctx, ctx.min_grid_index, ctx.max_grid_index);
        if (!tph_poisson_existing_sample_within_radius(ctx.sample,
              ctx.active_indices_vec[active_index],
              &ctx,
              ctx.min_grid_index,
              ctx.max_grid_index)) {
          /* No existing samples where found to be too close to the
           * candidate sample, no further attempts necessary. */
          ret = tph_poisson_add_sample(ctx.sample, &ctx);
          if (ret != TPH_POISSON_SUCCESS) {
            tph_poisson_context_destroy(&ctx);
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
      tph_poisson_vec_erase_unordered(ctx.active_indices_vec, active_index, /*count=*/1);
    }
    active_index_count = tph_poisson_vec_size(ctx.active_indices_vec);
  }

  /* Transfer samples to output. */
  const ptrdiff_t nsamples = tph_poisson_vec_size(ctx.samples_vec) / ctx.ndims;
  tph_poisson_assert(nsamples >= 1);
  tph_poisson_real *samples = (tph_poisson_real *)ctx.alloc->malloc(
    ctx.ndims * nsamples * TPH_POISSON_SIZEOF(tph_poisson_real), ctx.alloc->ctx);
  if (!samples) {
    tph_poisson_context_destroy(&ctx);
    return TPH_POISSON_BAD_ALLOC;
  }
  TPH_POISSON_MEMCPY(
    samples, ctx.samples_vec, ctx.ndims * nsamples * TPH_POISSON_SIZEOF(tph_poisson_real));
  s->ndims = ctx.ndims;
  s->nsamples = nsamples;
  s->samples = samples;
  s->alloc = ctx.alloc;

  tph_poisson_context_destroy(&ctx);

  return TPH_POISSON_SUCCESS;
}

void tph_poisson_destroy(tph_poisson_sampling *s)
{
  /* Sanity-check that the sampling was initialized by a call to tph_poisson_create. If the sampling
   * was default-initialized do nothing. */
  if (s && ((s->alloc != NULL) & (s->samples != NULL) & (s->ndims > 0) & (s->nsamples > 0))) {
    tph_poisson_assert(s->alloc->free);
    s->alloc->free(
      s->samples, s->nsamples * s->ndims * TPH_POISSON_SIZEOF(*s->samples), s->alloc->ctx);
  }
}

#undef TPH_POISSON_SIZEOF
#undef TPH_POISSON_CLAMP
#undef tph_poisson_vec
#undef tph_poisson_vec_empty
#undef tph_poisson_vec_size
#undef tph_poisson_vec_append
#undef tph_poisson_vec_erase_unordered

#endif// TPH_POISSON_IMPLEMENTATION

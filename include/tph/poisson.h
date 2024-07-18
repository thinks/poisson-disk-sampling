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
  int32_t ndims;
  const tph_real *bounds_min;
  const tph_real *bounds_max;
  uint32_t max_sample_attempts;
  uint64_t seed;
};

struct tph_poisson_sampling_
{
  tph_poisson_context_internal *internal;
  int32_t dims;
  ptrdiff_t numpoints;
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

#include <assert.h>// assert
#include <stdlib.h>// malloc, realloc, free

// Structs

#pragma pack(push, 1)

typedef struct tph_poisson_grid_
{
  tph_real dx;
  tph_real dx_inv;
  int32_t ndims;
  const tph_real *bounds_min;
  const tph_real *bounds_max;
  ptrdiff_t *size;
  uint32_t *cells;

  // ptrdiff_t *strides; // TODO!


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

  ptrdiff_t *grid_index;
  ptrdiff_t *min_grid_index;
  ptrdiff_t *max_grid_index;

  tph_poisson_xoshiro256p_state prng_state;
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
  int32_t ndims,
  const tph_real *bounds_min,
  const tph_real *bounds_max,
  const tph_poisson_context_internal *internal)
{
  const tph_real dx = ((tph_real)0.999 * radius) / TPH_SQRT((tph_real)ndims);
  const tph_real dx_inv = (tph_real)1 / dx;

  ptrdiff_t *size = (ptrdiff_t *)internal->alloc(internal->mem_ctx, ndims * sizeof(int32_t));
  if (size == NULL) { return TPH_POISSON_BAD_ALLOC; }

  ptrdiff_t linear_size = 1;
  for (int32_t i = 0; i < ndims; ++i) {
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
  /* Initialize cells with value 0xFFFFFFFF, indicating no sample there.
   * Cell values are later set to indices of samples. */
  memset(cells, 0xFF, linear_size * sizeof(uint32_t));

  grid->dx = dx;
  grid->dx_inv = dx_inv;
  grid->ndims = ndims;
  grid->bounds_min = bounds_min;
  grid->bounds_max = bounds_max;
  grid->size = size;
  grid->cells = cells;

  return TPH_POISSON_SUCCESS;
}

/* NOTE: Could return a value less than zero to indicate error... */
static ptrdiff_t tph_poisson_grid_linear_index(const tph_poisson_grid *grid,
  const ptrdiff_t *grid_index)
{
  assert((0 <= grid_index[0]) & (grid_index[0] < grid->size[0]));
  ptrdiff_t k = grid_index[0];
  ptrdiff_t d = 1;
  const int32_t ndims = grid->ndims;
  for (int32_t i = 1; i < ndims; ++i) {
    assert((0 <= grid_index[i]) & (grid_index[i] < grid->size[i]));
    /* Not checking for overflow! */
    d *= grid->size[i - 1];
    k += grid_index[i] * d;
  }
  return k;
}

/**
 * @brief Returns non-zero if p is element-wise inclusively inside b_min and b_max; otherwise zero.
 * Assumes that b_min is element-wise less than b_max.
 * @param p     Position to test. 
 * @param b_min Minimum bounds.
 * @param b_max Maximum bounds.
 * @param ndims Number of values in p, b_min, and b_max.
 * @return Non-zero if p is element-wise inclusively inside the bounded region; otherwise zero.
 */
static int tph_poisson_inside(const tph_real *p,
  const tph_real *b_min,
  const tph_real *b_max,
  ptrdiff_t ndims)
{
  assert(ndims > 0);
  while (ndims--) {
    assert(b_min[ndims] < b_max[ndims]);
    if (!((b_min[ndims] <= p[ndims]) & (p[ndims] <= b_max[ndims]))) { return 0; }
  }
  return 1;
}

static uint64_t tph_poisson_splitmix64(uint64_t *state)
{
  uint64_t result = (*state += 0x9E3779B97f4A7C15);
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
 * @param seed Seed value, can be zero.
 */
static void tph_poisson_xoshiro256p_init(tph_poisson_xoshiro256p_state *state, const uint64_t seed)
{
  /* As suggested at https://prng.di.unimi.it, use SplitMix64 to initialize the state of a
   * generator starting from a 64-bit seed. It has been shown that initialization must be
   * performed with a generator radically different in nature from the one used to avoid
   * correlation on similar seeds. */
  uint64_t sm_state = seed;
  state->s[0] = tph_poisson_splitmix64(&sm_state);
  state->s[1] = tph_poisson_splitmix64(&sm_state);
  state->s[2] = tph_poisson_splitmix64(&sm_state);
  state->s[3] = tph_poisson_splitmix64(&sm_state);
}

/**
 * @brief Returns a pseudo-random number. Assumes that the value of the passed in state is not all
 * zeros.
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

  /* s[3] = rotl(s[3], 45) */
  s[3] = (s[3] << 45) | (s[3] >> 49);
  return result;
}

/**
 * @brief Returns a floating point number in [0..1).
 * @param x Bit representation.
 * @return A number in [0..1).
 */
static inline double tph_poisson_to_double(const uint64_t x)
{
  /* Convert to double, as suggested at https://prng.di.unimi.it
     This conversion prefers the high bits of x (usually, a good idea). */
  return (x >> 11) * 0x1.0p-53;
}

// Assumes min_value <= max_value.
static inline tph_real
  tph_poisson_clamped(const tph_real min_value, const tph_real max_value, const tph_real value)
{
  assert(min_value <= max_value);
  return value < min_value ? min_value : (value > max_value ? max_value : value);
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

static int tph_valid_args(const tph_poisson_args *args)
{
  if (args == NULL) { return 0; }
  int valid = 1;
  valid &= (args->radius > 0.F);
  valid &= (args->max_sample_attempts > 0);
  valid &= (args->ndims > 0);
  for (int32_t i = 0; i < args->ndims; ++i) {
    valid &= (args->bounds_max[i] > args->bounds_min[i]);
  }
  return valid;
}

static int tph_poisson_add_sample(const tph_real *pos,
  tph_real *samples_vec,
  uint32_t *active_indices_vec,
  tph_poisson_grid *grid/*,
  ptrdiff_t *grid_index*/)
{
  /* assert(inside...) */
  const int32_t ndims = grid->ndims;
  const ptrdiff_t sample_index = tph_poisson_vec_size(samples_vec);
  int ret = TPH_POISSON_SUCCESS;
  ret = tph_poisson_vec_append(samples_vec, pos, ndims);
  if (ret != TPH_POISSON_SUCCESS) { return ret; }
  ret = tph_poisson_vec_push_back(active_indices_vec, sample_index);
  if (ret != TPH_POISSON_SUCCESS) { return ret; }


  ptrdiff_t k = (ptrdiff_t)TPH_FLOOR((pos[0] - grid->bounds_min[0]) * grid->dx_inv);
  assert((0 <= k) & (k < grid->size[0]));
  ptrdiff_t d = 1;
  for (int32_t i = 1; i < ndims; ++i) {
    // assert((0 <= grid_index[i]) & (grid_index[i] < grid->size[i]));
    /* Not checking for overflow! */
    d *= grid->size[i - 1];
    k += (ptrdiff_t)TPH_FLOOR((pos[i] - grid->bounds_min[i]) * grid->dx_inv) * d;
  }

  assert((0 <= k) & (k < grid->linear_size));
  assert(grid->cells[k] == 0xFFFFFFFF);
  grid->cells[k] = (uint32_t)sample_index;
  assert(grid->cells[k] != 0xFFFFFFFF);
  return ret;
}

/**
 * @brief Generate a pseudo-random sample position that is guaranteed be at a distance [radius, 2
 * * radius] from the provided center position.
 * @param center     Center position.
 * @param ndims      Number of values in center and sample.
 * @param radius     Radius bounding the annulus.
 * @param prng_state Pseudo-random number generator state.
 * @param sample     Output sample position.
 */
static void tph_poisson_rand_annulus_sample(const tph_real *center,
  const int32_t ndims,
  const tph_real radius,
  tph_poisson_xoshiro256p_state *prng_state,
  tph_real *sample)
{
  int32_t i = 0;
  tph_real sqr_mag = 0;
  while (1) {
    /* Generate a random component in the range [-2, 2] for each dimension. */
    sqr_mag = 0;
    for (i = 0; i < ndims; ++i) {
      sample[i] =
        (tph_real)(-2 + 4 * tph_poisson_to_double(tph_poisson_xoshiro256p_next(prng_state)));
      sqr_mag += sample[i] * sample[i];
    }

    /* The randomized offset is not guaranteed to be within the radial
     * distance that we need to guarantee. If we found an offset with
     * magnitude in the range (1, 2] we are done, otherwise generate a new
     * offset. Continue until a valid offset is found. */
    if (((tph_real)1 < sqr_mag) & (sqr_mag <= (tph_real)4)) {
      /* Found a valid offset.
       * Add the offset scaled by radius to the center coordinate to
       * produce the final sample. */
      for (i = 0; i < ndims; ++i) { sample[i] = center[i] + radius * sample[i]; }
      break;
    }
  }
}

static void tph_poisson_grid_index_bounds(const tph_real *sample,
  const tph_poisson_grid *grid,
  ptrdiff_t *min_grid_index,
  ptrdiff_t *max_grid_index)
{
  const int32_t ndims = grid->ndims;
  const tph_real dx_inv = grid->dx_inv;
  const tph_real r = grid->radius;
  const ptrdiff_t *grid_size = grid->size;
  const tph_real *grid_min = grid->bounds_min;
  for (int32_t i = 0; i < ndims; ++i) {
    assert(grid_size[i] > 0);
    min_grid_index[i] = tph_poisson_clamped(
      0, grid_size[i] - 1, (ptrdiff_t)TPH_FLOOR(((sample[i] - r) - grid_min[i]) * dx_inv));
    max_grid_index[i] = tph_poisson_clamped(
      0, grid_size[i] - 1, (ptrdiff_t)TPH_FLOOR(((sample[i] + r) - grid_min[i]) * dx_inv));
  }
}


// Returns true if there exists another sample within the radius used to
// construct the grid, otherwise false.
static int tph_poisson_existing_sample_within_radius(const tph_real *sample,
  const ptrdiff_t active_sample_index,
  const tph_real *samples_vec,
  const tph_poisson_grid *grid,
  ptrdiff_t *grid_index,
  const ptrdiff_t *min_grid_index,
  const ptrdiff_t *max_grid_index)
{
  const int32_t ndims = grid->ndims;
  const tph_real r_sqr = grid->radius * grid->radius;
  ptrdiff_t lin_index = -1;
  uint32_t cell = 0xFFFFFFFF;
  const tph_real *cell_sample = NULL;
  tph_real d_sqr = -1;
  int32_t i = -1;
  assert(ndims > 0);
  memcpy(grid_index, min_grid_index, ndims * sizeof(ptrdiff_t));
  do {
    cell = grid->cells[tph_poisson_grid_linear_index(grid, grid_index)];
    if ((cell != 0xFFFFFFFF) & (cell != (uint32_t)active_sample_index)) {
      /* Compute (squared) distance to the existing sample. */
      cell_sample = &samples_vec[cell * ndims];
      d_sqr = (sample[0] - cell_sample[0]) * (sample[0] - cell_sample[0]);
      for (i = 1; i < ndims; ++i) {
        d_sqr += (sample[i] - cell_sample[i]) * (sample[i] - cell_sample[i]);
      }

      /* Check if the existing sample is closer than radius to the provided sample. */
      if (d_sqr < r_sqr) { return 1; }
    }

    /* Iterate over grid index range. Enumerate every grid index
     * between min_grid_index and max_grid_index (inclusive) exactly once.
     * Assumes that min_index is element-wise less than or equal to max_index. */
    for (i = 0; i < ndims; ++i) {
      assert(min_grid_index[i] <= max_grid_index[i]);
      grid_index[i]++;
      if (grid_index[i] <= max_grid_index[i]) { break; }
      grid_index[i] = min_grid_index[i];
    }
    /* If the above loop ran to completion, without triggering the break, the
     * grid_index has been set to its original value (min_grid_index). Since this
     * was the starting value for grid_index we exit the outer loop when this happens. */
  } while (i != ndims);

  /* No existing samples were found to be closer to the sample than the provided radius. */
  return 0;
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
  if (!internal) { return TPH_POISSON_BAD_ALLOC; }
  internal->mem_ctx = user_alloc_ctx;
  internal->alloc = alloc_fn;
  internal->free = free_fn;

  /* Acceleration grid. */
  tph_poisson_grid grid;
  int ret = tph_poisson_grid_create(
    &grid, args->radius, args->ndims, args->bounds_min, args->bounds_max, internal);
  if (ret != TPH_POISSON_SUCCESS) {
    tph_poisson_free(s);
    return ret;
  }
  internal->grid_index = (ptrdiff_t *)alloc_fn(internal->mem_ctx, args->ndims * sizeof(ptrdiff_t));
  internal->min_grid_index =
    (ptrdiff_t *)alloc_fn(internal->mem_ctx, args->ndims * sizeof(ptrdiff_t));
  internal->max_grid_index =
    (ptrdiff_t *)alloc_fn(internal->mem_ctx, args->ndims * sizeof(ptrdiff_t));
  if (!(internal->grid_index && internal->min_grid_index && internal->max_grid_index)) {
    return TPH_POISSON_BAD_ALLOC;
  }


  /* Seed pseudo-random number generator. */
  internal->prng_state = {};
  tph_poisson_xoshiro256p_init(&internal->prng_state, args->seed);

  // cvector_vector_type(tph_real) samples = NULL;
  // cvector_vector_type(uint32_t) active_indices = NULL;

  /* Add first sample randomly within bounds.
     No need to check (non-existing) neighbors. */
  tph_real *pos = (tph_real *)alloc_fn(internal->mem_ctx, args->ndims * sizeof(tph_real));
  for (int32_t i = 0; i < args->ndims; ++i) {
    pos[i] = tph_poisson_clamped(args->bounds_min[i],
      args->bounds_max[i],
      args->bounds_min[i]
        + (tph_real)(tph_poisson_to_double(tph_poisson_xoshiro256p_next(&prng_state)))
            * (args->bounds_max[i] - args->bounds_min[i]));
  }
  assert(tph_poisson_inside(pos, args->bounds_min, args->bounds_max, args->ndims));

  ret = tph_poisson_add_sample(pos, samples_vec, active_indices_vec, grid, grid_index);
  if (ret != TPH_POISSON_SUCCESS) { return ret; }

  while (!tph_poisson_vec_empty(active_indices_vec)) {
    /* Randomly choose an active sample. A sample is considered active
       until failed attempts have been made to generate a new sample within
       its annulus. */
    const ptrdiff_t active_index =
      (ptrdiff_t)(tph_poisson_xoshiro256p_next(&prng_state)
                  % (uint64_t)tph_poisson_vec_size(active_indices_vec));
    const ptrdiff_t sample_index = active_indices_vec[active_index];
    const tph_real *sample_pos = &samples_vec[sample_index * args->ndims];
    // const auto active_sample = pds::RandActiveSample(active_indices, samples, &local_seed);
    uint32_t attempt_count = 0;
    while (attempt_count < args->max_sample_attempts) {
      /* Randomly create a candidate sample inside the active sample's annulus. */
      tph_poisson_rand_annulus_sample(sample_pos, args->ndims, args->radius, &prng_state, pos);
      /* Check if candidate sample is within bounds. */
      if (tph_poisson_inside(pos, args->bounds_min, args->bounds_max, args->ndims)) {
        tph_poisson_grid_index_bounds(pos, &grid, min_grid_index, max_grid_index);
        if (!tph_poisson_existing_sample_within_radius(
              pos, active_index, samples_vec, &grid, grid_index, min_grid_index, max_grid_index)) {
          /* No existing samples where found to be too close to the
           * candidate sample, no further attempts necessary. */
          ret = tph_poisson_add_sample(pos, samples_vec, active_indices_vec, grid, grid_index);
          if (ret != TPH_POISSON_SUCCESS) { return ret; }
          break;
        }
        /* else: The candidate sample is too close to an existing sample,
         * move on to next attempt. */
      }
      /* else: The candidate sample is out-of-bounds. */
      ++attempt_count;
    }

    if (attempt_count == args->max_sample_attempts) {
      /* No valid sample was found on the disk of the active sample after
       * maximum number of attempts, remove it from the active list. */
      tph_poisson_vec_erase_unordered(active_indices_vec, active_index);
    }
  }

#if 1// TMP TEST!!
  internal->pos =
    (tph_real *)internal->alloc(internal->mem_ctx, 5 * args->ndims * sizeof(tph_real));
  if (internal->pos == NULL) { return TPH_POISSON_BAD_ALLOC; }
  for (int32_t i = 0; i < 5; ++i) {
    /* Generate random position within bounds. */
    for (int32_t i = 0; i < args->ndims; ++i) {
      pos[i] = tph_poisson_clamped(args->bounds_min[i],
        args->bounds_max[i],
        args->bounds_min[i]
          + (tph_real)(tph_poisson_to_double(tph_poisson_xoshiro256p_next(&prng_state)))
              * (args->bounds_max[i] - args->bounds_min[i]));
    }
    memcpy(&internal->pos[i * args->ndims], pos, args->ndims * sizeof(tph_real));
  }
#endif

  tph_poisson_grid_free(&grid, internal);

  // cvector_free(samples);
  // cvector_free(active_indices);

  s->internal = internal;
  s->dims = args->ndims;
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

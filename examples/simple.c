#include <inttypes.h>// PRIu32, etc
#include <stdio.h>// printf
#include <string.h>// memset

#define TPH_POISSON_IMPLEMENTATION
#define TPH_POISSON_REAL_TYPE float /*double*/
#include <thinks/tph_poisson.h>

int main(int argc, char *argv[])
{
  const tph_poisson_real bounds_min[] = { -100.F, -100.F };
  const tph_poisson_real bounds_max[] = { 100.F, 100.F };
  tph_poisson_args args = {};
  args.radius = 10.F;
  args.ndims = 2;
  args.bounds_min = bounds_min;
  args.bounds_max = bounds_max;
  args.max_sample_attempts = 30;
  args.seed = 0;

  tph_poisson_sampling sampling = {};
  int ret = tph_poisson_create(&sampling, &args, NULL);
  if (ret != TPH_POISSON_SUCCESS) {
    printf("Error!");
    return 1;
  }

  for (ptrdiff_t i = 0; i < sampling.nsamples; ++i) {
    printf("p[%td] = ( %.3f, %.3f )\n",
      i,
      sampling.samples[i * sampling.ndims],
      sampling.samples[i * sampling.ndims + 1]);
  }

  tph_poisson_destroy(&sampling);

  return 0;
}

#if 0
typedef struct tph_poisson_context_
{
  tph_poisson_allocator *alloc;
  char *mem;
  ptrdiff_t mem_size;

  tph_real radius;
  int32_t ndims;
  uint32_t max_sample_attempts;
  tph_real *bounds_min;
  tph_real *bounds_max;
  tph_poisson_xoshiro256p_state prng_state;

  tph_poisson_vec(tph_real) samples_vec;
  tph_poisson_vec(ptrdiff_t) active_indices_vec;

  tph_real grid_dx; /** Uniform cell extent. */
  tph_real grid_dx_rcp; /** 1 / dx */
  /* ptrdiff_t linear_size; ?? */
  ptrdiff_t *grid_size; /** Number of grid cells in each dimension. */
  ptrdiff_t *grid_stride; /** strides ... */
  uint32_t *grid_cells; /** cells ... */

  /* Arrays of size ndims. Pre-allocated in the context to provide 'scratch' variables. */
  tph_real *sample;
  ptrdiff_t *grid_index;
  ptrdiff_t *min_grid_index;
  ptrdiff_t *max_grid_index;
} tph_poisson_context;
#endif
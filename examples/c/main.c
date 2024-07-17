#include <inttypes.h>// PRIu32, etc
#include <stdio.h>// printf
#include <string.h>// memset

#define TPH_POISSON_IMPLEMENTATION
#define TPH_REAL_TYPE float /*double*/
#include <tph/poisson.h>

int main(int argc, char *argv[])
{
  const tph_real bounds_min[] = { -100.F, -100.F };
  const tph_real bounds_max[] = { 100.F, 100.F };

  // clang-format off
  const tph_poisson_args args = { 
    .radius = 10.F,
    .ndims = 2,
    .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .max_sample_attempts = 30,
    .seed = 0 };
  // clang-format on

  tph_poisson_sampling sampling = {};
  tph_poisson_generate(&args, &sampling);
  const tph_poisson_points points = tph_poisson_get_points(&sampling);
  if (points.pos == NULL) {
    printf("NULL points");
    return 1;
  }

  for (ptrdiff_t i = 0; i < sampling.numpoints; ++i) {
    printf("p[%td] = ( %.3f, %.3f )\n",
      i,
      points.pos[i * sampling.dims],
      points.pos[i * sampling.dims + 1]);
  }

  return 0;
}

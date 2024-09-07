#define TPH_POISSON_IMPLEMENTATION
#include <tph/poisson.h>

#include <inttypes.h>
#include <stdio.h>// printf

int main(int argc, char *argv[])
{
  tph_real bbox_min[2] = { -100.F, -100.F };
  tph_real bbox_max[2] = { 100.F, 100.F };
  tph_poisson_args args = {
        .radius = 10.F,
        .dims = 2,
        .bbox = {
            .min = bbox_min,
            .max = bbox_max,
        },
        .max_sample_attempts = 30,
        .seed = 0
    };

  tph_poisson_sampling sampling;
  tph_poisson_generate(&args, &sampling);

  const tph_poisson_points *points = tph_poisson_get_points(&sampling);

  for (uint32_t i = 0; i < sampling.numpoints; ++i) {
    tph_real pos[2];
    pos[0] = points->pos[i * sampling.dims];
    pos[1] = points->pos[i * sampling.dims + 1];
    printf("p[%" PRIu32 "] = ( %.3f, %.3f )", i, pos[0], pos[1]);
  }

  tph_poisson_free(&sampling);

  return 0;
}
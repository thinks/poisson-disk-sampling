#include <array>//std::array
#include <cinttypes>// PRIu32, etc
#include <cstdio>// std::printf
#include <cstring>// std::memset
#include <memory>// std::unique_ptr

#define TPH_POISSON_IMPLEMENTATION
#define TPH_REAL_TYPE float /*double*/
#include <tph/poisson.h>

int main(int argc, char *argv[])
{
  std::vector

  constexpr std::array<tph_real, 2> bounds_min{ -100.F, -100.F };
  constexpr std::array<tph_real, 2> bounds_max{ 100.F, 100.F };

  // clang-format off
  const tph_poisson_args args{ 
    /*.radius=*/10.F,
    /*.dims=*/2,
    /*.bounds_min=*/bounds_min.data(),
    /*.bounds_max=*/bounds_max.data(),
    /*.max_sample_attempts=*/30,
    /*.seed=*/0 };
  auto sampling = std::unique_ptr<tph_poisson_sampling, decltype(&tph_poisson_free)>{
    new tph_poisson_sampling{ 
      /*.internal=*/nullptr,
      /*.dims=*/0,
      /*.num_points=*/0 },
    tph_poisson_free
  };
  // clang-format on

  tph_poisson_generate(&args, sampling.get());
  const tph_poisson_points points = tph_poisson_get_points(sampling.get());
  if (points.pos == NULL) {
    std::printf("NULL");
    return 1;
  }

  for (uint32_t i = 0; i < sampling->numpoints; ++i) {
    std::printf("p[%" PRIu32 "] = ( %.3f, %.3f )\n",
      i,
      points.pos[i * sampling->dims],
      points.pos[i * sampling->dims + 1]);
  }

  return 0;
}

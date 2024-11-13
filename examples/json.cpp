#include <array>
#include <cstdint>// UINT32_C, etc
#include <cstdlib>// std::abort, EXIT_FAILURE
#include <fstream>// std::ofstream
#include <functional>// std::function
#include <iomanip>// std::setw
#include <memory>// std::unique_ptr
#include <type_traits>// std::is_same_v
#include <vector>

#include <nlohmann/json.hpp>

#define TPH_POISSON_IMPLEMENTATION
#include <thinks/tph_poisson.h>

static_assert(std::is_same_v<tph_poisson_real, float>);

int main(int /*argc*/, char * /*argv*/[])
{
  constexpr std::array<tph_poisson_real, 2> bounds_min{ -10, -10 };
  constexpr std::array<tph_poisson_real, 2> bounds_max{ 10, 10 };
  tph_poisson_args args = {};
  args.ndims = 2;
  args.radius = static_cast<tph_poisson_real>(3);
  args.bounds_min = bounds_min.data();
  args.bounds_max = bounds_max.data();
  args.max_sample_attempts = UINT32_C(30);
  args.seed = UINT64_C(0);

  using unique_poisson_ptr =
    std::unique_ptr<tph_poisson_sampling, std::function<void(tph_poisson_sampling *)>>;
  auto sampling = unique_poisson_ptr{ new tph_poisson_sampling{}, [](tph_poisson_sampling *s) {
                                       tph_poisson_destroy(s);
                                       delete s;
                                     } };

  if (tph_poisson_create(&args, /*alloc=*/nullptr, sampling.get()) != TPH_POISSON_SUCCESS) {
    return EXIT_FAILURE;
  }

  const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
  if (samples == nullptr) { return EXIT_FAILURE; }

  try {
    std::vector<std::array<tph_poisson_real, 2>> points;
    for (ptrdiff_t i = 0; i < sampling->nsamples; ++i) {
      points.push_back(std::array<tph_poisson_real, 2>{ {
        samples[i * sampling->ndims],
        samples[i * sampling->ndims + 1],
      } });
    }

    using json = nlohmann::json;
    json j;
    j["bounds_min"] = bounds_min;
    j["bounds_max"] = bounds_max;
    j["seed"] = args.seed;
    j["max_sample_attempts"] = args.max_sample_attempts;
    j["radius"] = args.radius;
    j["ndims"] = args.ndims;
    j["points"] = points;

    std::ofstream ofs{ "./tph_poisson.json" };
    if (!ofs) { return EXIT_FAILURE; }
    ofs << std::setw(2) << j;
    ofs.close();
  } catch (...) {
    std::printf("Unknown exception\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

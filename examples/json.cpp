#include <array>
#include <cstdint>// UINT32_C, etc
#include <cstdlib>// std::abort, EXIT_FAILURE
#include <fstream>
#include <iomanip>// std::setw
#include <memory>// std::unique_ptr
#include <vector>

#include <nlohmann/json.hpp>

#define TPH_POISSON_IMPLEMENTATION
#include <thinks/tph_poisson.h>

int main(int /*argc*/, char * /*argv*/[])
{
  using unique_poisson_ptr = std::unique_ptr<tph_poisson_sampling, decltype(&tph_poisson_destroy)>;
  auto make_unique_poisson = []() {
    return unique_poisson_ptr(new tph_poisson_sampling{}, tph_poisson_destroy);
  };

  constexpr std::array<tph_poisson_real, 2> bounds_min{ -10, -10 };
  constexpr std::array<tph_poisson_real, 2> bounds_max{ 10, 10 };
  tph_poisson_args args = {};
  args.ndims = 2;
  args.radius = 3;
  args.bounds_min = bounds_min.data();
  args.bounds_max = bounds_max.data();
  args.max_sample_attempts = UINT32_C(30);
  args.seed = UINT64_C(0);
  unique_poisson_ptr sampling = make_unique_poisson();
  if (tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr) != TPH_POISSON_SUCCESS) {
    std::abort();
  }

  const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
  if (samples == nullptr) { std::abort(); }

  using json = nlohmann::json;
  json j;
  j["bounds_min"] = bounds_min;
  j["bounds_max"] = bounds_max;
  j["radius"] = args.radius;
  j["ndims"] = args.ndims;
  j["samples"] =
    std::vector<tph_poisson_real>{ samples, samples + sampling->ndims * sampling->nsamples };

  std::ofstream ofs{ "./tph_poisson.json" };
  if (!ofs) { return EXIT_FAILURE; }
  ofs << std::setw(2) << j;
  ofs.close();

  return EXIT_SUCCESS;
}

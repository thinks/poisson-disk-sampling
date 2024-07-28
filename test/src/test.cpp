#include <algorithm>// std::all_of
#include <array>
#include <cmath>
#include <cstdint>
#include <future>
#include <limits>
#include <thread>
#include <vector>

#include "tph_poisson_f.h"
using Real = TPH_POISSON_REAL_TYPE;

#if 0
template<int32_t NDIMS> class PoissonSampling
{
public:
  PoissonSampling() : _sampling{} {}
  ~PoissonSampling() { tph_poisson_destroy(&_sampling); }

  // Disable copy/assign.
  PoissonSampling(const PoissonSampling &) = delete;
  PoissonSampling &operator=(const PoissonSampling &) = delete;

  [[nodiscard]] auto Create(const Real radius,
    const std::array<Real, NDIMS> &bounds_min,
    const std::array<Real, NDIMS> &bounds_max,
    const std::uint32_t max_sample_attempts = 30,
    const std::uint64_t seed = 0) noexcept -> int
  {
    tph_poisson_args args = {};
    args.ndims = NDIMS;
    args.radius = radius;
    args.bounds_min = bounds_min.data();
    args.bounds_max = bounds_max.data();
    args.max_sample_attempts = max_sample_attempts;
    args.seed = seed;
    _sampling = {};
    return tph_poisson_create(&_sampling, &args, /*alloc=*/nullptr);
  }

  auto Sample(ptrdiff_t i, std::array<Real, NDIMS> *pos) noexcept -> void
  {
    if (i < _sampling.nsamples) {
      for (int32_t j = 0; j < NDIMS; ++j) { *pos[j] = _sampling.samples[i * NDIMS + j]; }
    }
  }

private:
  tph_poisson_sampling _sampling;
};
#endif

static void
  require_fail(const char *expr, const char *file, unsigned int line, const char *function)
{
  std::printf(
    "Requirement failed: '%s' on line %u in file %s in function %s\n", expr, line, file, function);
  std::abort();
}

// clang-format off
#ifdef _MSC_VER
  #define TPH_PRETTY_FUNCTION __FUNCSIG__
#else 
  #define TPH_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif
#define REQUIRE(expr) \
  (static_cast<bool>(expr) ? void(0) : require_fail(#expr, __FILE__, __LINE__, TPH_PRETTY_FUNCTION))
// clang-format on

using unique_poisson_ptr = std::unique_ptr<tph_poisson_sampling, decltype(&tph_poisson_destroy)>;
static auto make_unique_poisson() -> unique_poisson_ptr
{
  return unique_poisson_ptr(new tph_poisson_sampling{}, tph_poisson_destroy);
}

// Brute-force (with some tricks) verification that the distance between each
// possible sample pair meets the Poisson requirement, i.e. is greater than some
// radius.
static void TestRadius()
{
  auto valid_radius = [](int32_t ndims, const Real *bounds_min, const Real *bounds_max) {
    tph_poisson_args args = {};
    args.ndims = ndims;
    args.radius = 2;
    args.bounds_min = bounds_min;
    args.bounds_max = bounds_max;
    args.seed = UINT64_C(1981);
    args.max_sample_attempts = UINT32_C(30);
    unique_poisson_ptr sampling = make_unique_poisson();
    if (tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr) != TPH_POISSON_SUCCESS) {
      return false;
    }

    // Setup threading.
    // Avoid spawning more threads than there are samples (although very
    // unlikely).
    const ptrdiff_t thread_count =
      std::thread::hardware_concurrency() > 0
        ? std::min(static_cast<ptrdiff_t>(std::thread::hardware_concurrency()), sampling->nsamples)
        : static_cast<ptrdiff_t>(1);

    // Launch threads.
    std::vector<std::future<bool>> futures;
    for (ptrdiff_t i = 0; i < thread_count; ++i) {
      futures.emplace_back(std::async(std::launch::async, [&]() {
        // We know that distance is symmetrical, such that
        // dist(s[j], s[k]) == dist(s[k], s[j]). Therefore
        // we need only compute the upper half of the matrix (excluding the diagonal).
        //
        // Load balance threads such that "short" (small j) and "long" (large j)
        // columns are divided evenly among threads.
        const int32_t ndims = sampling->ndims;
        const Real r_sqr = args.radius * args.radius;
        for (ptrdiff_t j = i; j < sampling->nsamples; j += thread_count) {
          const Real *sj = &sampling->samples[j * ndims];
          const ptrdiff_t k_max = j;
          for (ptrdiff_t k = 0; k < k_max; ++k) {
            const Real *sk = &sampling->samples[k * ndims];
            Real dist_sqr = 0;
            for (int32_t m = 0; m < ndims; ++m) { dist_sqr += (sj[m] - sk[m]) * (sj[m] - sk[m]); }
            if (!(dist_sqr > r_sqr)) { return false; }
          }
        }
        return true;
      }));
    }

    // Check results.
    for (auto &&f : futures) { f.wait(); }
    return std::all_of(
      std::begin(futures), std::end(futures), [](std::future<bool> &f) { return f.get(); });

    return true;
  };

  constexpr std::array<Real, 2> bounds_min2{ -100, -100 };
  constexpr std::array<Real, 2> bounds_max2{ 100, 100 };
  REQUIRE(valid_radius(/*ndims=*/2, bounds_min2.data(), bounds_max2.data()));

  constexpr std::array<Real, 3> bounds_min3{ -20, -20, -20 };
  constexpr std::array<Real, 3> bounds_max3{ 20, 20, 20 };
  REQUIRE(valid_radius(/*ndims=*/3, bounds_min3.data(), bounds_max3.data()));

  constexpr std::array<Real, 4> bounds_min4{ -10, -10, -10, -10 };
  constexpr std::array<Real, 4> bounds_max4{ 10, 10, 10, 10 };
  REQUIRE(valid_radius(/*ndims=*/4, bounds_min4.data(), bounds_max4.data()));
}

// Verify that all samples are within the specified bounds.
static void TestBounds()
{
  auto valid_bounds = [](int32_t ndims, const Real *bounds_min, const Real *bounds_max) {
    tph_poisson_args args = {};
    args.ndims = ndims;
    args.radius = 2;
    args.bounds_min = bounds_min;
    args.bounds_max = bounds_max;
    args.seed = UINT64_C(1981);
    args.max_sample_attempts = UINT32_C(30);
    unique_poisson_ptr sampling = make_unique_poisson();
    if (tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr) != TPH_POISSON_SUCCESS) {
      return false;
    }
    for (ptrdiff_t i = 0; i < sampling->nsamples; ++i) {
      const Real *p = &sampling->samples[i * sampling->ndims];
      for (int32_t j = 0; j < sampling->ndims; ++j) {
        if (!((bounds_min[j] <= p[j]) & (p[j] <= bounds_max[j]))) { return false; }
      }
    }
    return true;
  };

  constexpr std::array<Real, 2> bounds_min2{ -100, -100 };
  constexpr std::array<Real, 2> bounds_max2{ 100, 100 };
  REQUIRE(valid_bounds(/*ndims=*/2, bounds_min2.data(), bounds_max2.data()));

  constexpr std::array<Real, 3> bounds_min3{ -20, -20, -20 };
  constexpr std::array<Real, 3> bounds_max3{ 20, 20, 20 };
  REQUIRE(valid_bounds(/*ndims=*/3, bounds_min3.data(), bounds_max3.data()));

  constexpr std::array<Real, 4> bounds_min4{ -10, -10, -10, -10 };
  constexpr std::array<Real, 4> bounds_max4{ 10, 10, 10, 10 };
  REQUIRE(valid_bounds(/*ndims=*/4, bounds_min4.data(), bounds_max4.data()));
}

// Verify that we get a denser sampling, i.e. more samples,
// when we increase the max sample attempts parameter (with
// all other parameters constant).
static void TestVaryingMaxSampleAttempts()
{
  constexpr int32_t ndims = 2;
  constexpr std::array<Real, ndims> bounds_min{ -10, -10 };
  constexpr std::array<Real, ndims> bounds_max{ 10, 10 };

  tph_poisson_args args_10 = {};
  args_10.ndims = ndims;
  args_10.radius = static_cast<Real>(0.5);
  args_10.bounds_min = bounds_min.data();
  args_10.bounds_max = bounds_max.data();
  args_10.seed = UINT64_C(1981);
  args_10.max_sample_attempts = UINT32_C(10);
  tph_poisson_args args_40 = args_10;
  args_40.max_sample_attempts = UINT32_C(40);

  unique_poisson_ptr sampling_10 = make_unique_poisson();
  unique_poisson_ptr sampling_40 = make_unique_poisson();
  REQUIRE(
    TPH_POISSON_SUCCESS == tph_poisson_create(sampling_10.get(), &args_10, /*alloc=*/nullptr));
  REQUIRE(
    TPH_POISSON_SUCCESS == tph_poisson_create(sampling_40.get(), &args_40, /*alloc=*/nullptr));

  REQUIRE(sampling_10->nsamples < sampling_40->nsamples);
}

// Verify that different seeds give different sample distributions (with
// all other parameters constant).
static void TestVaryingSeed()
{
  constexpr int32_t ndims = 2;
  constexpr std::array<Real, ndims> bounds_min{ -10, -10 };
  constexpr std::array<Real, ndims> bounds_max{ 10, 10 };

  tph_poisson_args args_1981 = {};
  args_1981.ndims = ndims;
  args_1981.radius = static_cast<Real>(0.5);
  args_1981.bounds_min = bounds_min.data();
  args_1981.bounds_max = bounds_max.data();
  args_1981.max_sample_attempts = UINT32_C(20);
  args_1981.seed = UINT64_C(1981);
  tph_poisson_args args_1337 = args_1981;
  args_1337.seed = UINT64_C(1337);

  unique_poisson_ptr sampling_1981 = make_unique_poisson();
  unique_poisson_ptr sampling_1337 = make_unique_poisson();
  REQUIRE(
    TPH_POISSON_SUCCESS == tph_poisson_create(sampling_1981.get(), &args_1981, /*alloc=*/nullptr));
  REQUIRE(
    TPH_POISSON_SUCCESS == tph_poisson_create(sampling_1337.get(), &args_1337, /*alloc=*/nullptr));

  // For each sample in the first point set compute the smallest
  // distance checking every sample in the second point set.
  // Then, if the smallest distance is larger than some threshold
  // we say that the sample from the first point set is distinct
  // from every sample in the second point set. Thus the two
  // distributions must be different.
  auto distinct_sample_found = false;
  for (ptrdiff_t i = 0; i < sampling_1981->nsamples; ++i) {
    const Real *p = &sampling_1981->samples[i * ndims];
    Real min_sqr_dist = std::numeric_limits<Real>::max();
    for (ptrdiff_t j = 0; j < sampling_1337->nsamples; ++j) {
      const Real *q = &sampling_1337->samples[j * ndims];
      Real sqr_dist = 0;
      for (int32_t k = 0; k < ndims; ++k) { sqr_dist += (p[k] - q[k]) * (p[k] - q[k]); }
      min_sqr_dist = std::min(min_sqr_dist, sqr_dist);
    }
    if (std::sqrt(min_sqr_dist) > static_cast<Real>(0.1)) {
      distinct_sample_found = true;
      break;
    }
  }

  REQUIRE(distinct_sample_found);
}

static void TestInvalidArgs()
{
  constexpr int32_t ndims = 2;
  constexpr std::array<Real, ndims> bounds_min{ -10, -10 };
  constexpr std::array<Real, ndims> bounds_max{ 10, 10 };
  tph_poisson_args valid_args = {};
  valid_args.radius = 1;
  valid_args.ndims = ndims;
  valid_args.bounds_min = bounds_min.data();
  valid_args.bounds_max = bounds_max.data();
  valid_args.max_sample_attempts = UINT32_C(30);
  valid_args.seed = UINT64_C(333);
  unique_poisson_ptr sampling = make_unique_poisson();
  REQUIRE(
    TPH_POISSON_SUCCESS == tph_poisson_create(sampling.get(), &valid_args, /*alloc=*/nullptr));
  REQUIRE(sampling->ndims == ndims);
  REQUIRE(sampling->nsamples > 0);
  REQUIRE(sampling->samples != nullptr);
  REQUIRE(sampling->alloc != nullptr);
  sampling = make_unique_poisson();
  REQUIRE(sampling->ndims == 0);
  REQUIRE(sampling->nsamples == 0);
  REQUIRE(sampling->samples == nullptr);
  REQUIRE(sampling->alloc == nullptr);

  // radius <= 0
  [&]() {
    tph_poisson_args args = valid_args;
    args.radius = 0;
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    args.radius = -1;
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));
  }();

  // ndims <= 0
  [&]() {
    tph_poisson_args args = valid_args;
    args.ndims = 0;
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    args.ndims = -1;
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));
  }();

  // max_sample_attempts == 0
  [&]() {
    tph_poisson_args args = valid_args;
    args.max_sample_attempts = 0;
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));
  }();

  // bounds_min >= bounds_max
  [&]() {
    tph_poisson_args args = valid_args;
    args.bounds_min = nullptr;
    args.bounds_max = nullptr;
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    constexpr std::array<Real, ndims> invalid_bounds_min0{ 10, 10 };
    constexpr std::array<Real, ndims> invalid_bounds_max0{ 10, 10 };
    args.bounds_min = invalid_bounds_min0.data();
    args.bounds_max = invalid_bounds_max0.data();
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    constexpr std::array<Real, ndims> invalid_bounds_min1{ 10, -10 };
    constexpr std::array<Real, ndims> invalid_bounds_max1{ 10, 10 };
    args.bounds_min = invalid_bounds_min1.data();
    args.bounds_max = invalid_bounds_max1.data();
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    constexpr std::array<Real, ndims> invalid_bounds_min2{ -10, 10 };
    constexpr std::array<Real, ndims> invalid_bounds_max2{ 10, 10 };
    args.bounds_min = invalid_bounds_min2.data();
    args.bounds_max = invalid_bounds_max2.data();
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    constexpr std::array<Real, ndims> invalid_bounds_min3{ 10, 10 };
    constexpr std::array<Real, ndims> invalid_bounds_max3{ -10, -10 };
    args.bounds_min = invalid_bounds_min3.data();
    args.bounds_max = invalid_bounds_max3.data();
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    constexpr std::array<Real, ndims> invalid_bounds_min4{ 10, -10 };
    constexpr std::array<Real, ndims> invalid_bounds_max4{ -10, 10 };
    args.bounds_min = invalid_bounds_min4.data();
    args.bounds_max = invalid_bounds_max4.data();
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));

    constexpr std::array<Real, ndims> invalid_bounds_min5{ -10, 10 };
    constexpr std::array<Real, ndims> invalid_bounds_max5{ 10, -10 };
    args.bounds_min = invalid_bounds_min5.data();
    args.bounds_max = invalid_bounds_max5.data();
    REQUIRE(
      TPH_POISSON_INVALID_ARGS == tph_poisson_create(sampling.get(), &args, /*alloc=*/nullptr));
  }();

  // Verify that no output was written by failed attempts.
  REQUIRE(sampling->ndims == 0);
  REQUIRE(sampling->nsamples == 0);
  REQUIRE(sampling->samples == nullptr);
  REQUIRE(sampling->alloc == nullptr);
}

int main(int argc, char *argv[])
{
  std::printf("TestRadius...\n");
  TestRadius();

  std::printf("TestBounds...\n");
  TestBounds();

  std::printf("TestVaryingMaxSampleAttempts...\n");
  TestVaryingMaxSampleAttempts();

  std::printf("TestVaryingSeed...\n");
  TestVaryingSeed();

  std::printf("TestInvalidArgs...\n");
  TestInvalidArgs();
  
  return EXIT_SUCCESS;
}

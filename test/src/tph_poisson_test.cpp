#include <algorithm>// std::all_of
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>// std::memcmp
#include <functional>// std::function
#include <future>
#include <limits>
#include <thread>
#include <vector>

// #include <rpmalloc/rpmalloc.h>

#define TPH_POISSON_VEC_TEST 1
#ifdef TPH_POISSON_TEST_USE_DOUBLE
#include "tph_poisson_d.h"
#else
#include "tph_poisson_f.h"
#endif
using Real = TPH_POISSON_REAL_TYPE;

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

using unique_poisson_ptr =
  std::unique_ptr<tph_poisson_sampling, std::function<void(tph_poisson_sampling *)>>;
static auto make_unique_poisson() -> unique_poisson_ptr
{
  return unique_poisson_ptr(new tph_poisson_sampling{}, [](tph_poisson_sampling *s) {
    tph_poisson_destroy(s);
    delete s;
  });
}

static auto make_rpalloc() -> tph_poisson_allocator
{
  tph_poisson_allocator a = {};
  a.malloc = [](ptrdiff_t size, void *ctx) {
    static_cast<void>(ctx);
    // return rpmalloc(static_cast<size_t>(size));
    return malloc(static_cast<size_t>(size));
  };
  a.free = [](void *ptr, ptrdiff_t size, void *ctx) {
    static_cast<void>(size);
    static_cast<void>(ctx);
    // return rpfree(ptr);
    free(ptr);
  };
  a.ctx = nullptr;
  return a;
}

// Brute-force (with some tricks) verification that the distance between each
// possible sample pair meets the Poisson requirement, i.e. is greater than some
// radius.
static void TestRadius()
{
  const auto valid_radius = [](const std::vector<Real> bounds_min,
                              const std::vector<Real> bounds_max,
                              tph_poisson_allocator *alloc) {
    if (bounds_min.size() != bounds_max.size()) { return false; }
    tph_poisson_args args = {};
    args.ndims = static_cast<int32_t>(bounds_min.size());
    args.bounds_min = bounds_min.data();
    args.bounds_max = bounds_max.data();
    args.radius = 2;
    args.seed = UINT64_C(1981);
    args.max_sample_attempts = UINT32_C(30);
    unique_poisson_ptr sampling = make_unique_poisson();
    if (tph_poisson_create(&args, alloc, sampling.get()) != TPH_POISSON_SUCCESS) { return false; }
    const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
    if (samples == nullptr) { return false; }

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
      futures.emplace_back(std::async(std::launch::async,
        [i,
          samples,
          thread_count,
          ndims = sampling->ndims,
          nsamples = sampling->nsamples,
          r = args.radius]() {
          // We know that distance is symmetrical, such that
          // dist(s[j], s[k]) == dist(s[k], s[j]). Therefore
          // we need only compute the upper half of the matrix (excluding the diagonal).
          //
          // Load balance threads such that "short" (small j) and "long" (large j)
          // columns are divided evenly among threads.
          const Real r_sqr = r * r;
          for (ptrdiff_t j = i; j < nsamples; j += thread_count) {
            const Real *sj = &samples[j * ndims];
            const ptrdiff_t k_max = j;
            for (ptrdiff_t k = 0; k < k_max; ++k) {
              const Real *sk = &samples[k * ndims];
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
  };

  tph_poisson_allocator rpalloc = make_rpalloc();
  for (auto *alloc : std::vector<tph_poisson_allocator *>{ nullptr, &rpalloc }) {
    REQUIRE(valid_radius(/*bounds_min=*/{ -100, -100 }, /*bounds_max=*/{ 100, 100 }, alloc));
    REQUIRE(valid_radius({ -20, -20, -20 }, { 20, 20, 20 }, alloc));
    REQUIRE(valid_radius({ -10, -10, -10, -10 }, { 10, 10, 10, 10 }, alloc));
  }
}

// Verify that all samples are within the specified bounds.
static void TestBounds()
{
  const auto valid_bounds = [](const std::vector<Real> bounds_min,
                              const std::vector<Real> bounds_max,
                              tph_poisson_allocator *alloc) {
    if (bounds_min.size() != bounds_max.size()) { return false; }
    tph_poisson_args args = {};
    args.ndims = static_cast<int32_t>(bounds_min.size());
    args.bounds_min = bounds_min.data();
    args.bounds_max = bounds_max.data();
    args.radius = 2;
    args.seed = UINT64_C(1981);
    args.max_sample_attempts = UINT32_C(30);
    unique_poisson_ptr sampling = make_unique_poisson();
    if (tph_poisson_create(&args, alloc, sampling.get()) != TPH_POISSON_SUCCESS) { return false; }
    const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
    if (samples == nullptr) { return false; }
    for (ptrdiff_t i = 0; i < sampling->nsamples; ++i) {
      const Real *p = &samples[i * sampling->ndims];
      for (int32_t j = 0; j < sampling->ndims; ++j) {
        const size_t jj = static_cast<size_t>(j);
        // clang-format off
        if ((static_cast<int>(p[jj] >= bounds_min[jj]) & 
             static_cast<int>(p[jj] <= bounds_max[jj])) == 0) {
          return false;
        }
        // clang-format on
      }
    }
    return true;
  };

  tph_poisson_allocator rpalloc = make_rpalloc();
  for (auto *alloc : std::vector<tph_poisson_allocator *>{ nullptr, &rpalloc }) {
    REQUIRE(valid_bounds(/*bounds_min=*/{ -100, -100 }, /*bounds_max=*/{ 100, 100 }, alloc));
    REQUIRE(valid_bounds({ -20, -20, -20 }, { 20, 20, 20 }, alloc));
    REQUIRE(valid_bounds({ -10, -10, -10, -10 }, { 10, 10, 10, 10 }, alloc));
  }
}

// Verify that we get a denser sampling, i.e. more samples,
// when we increase the max sample attempts parameter (with
// all other parameters constant).
static void TestVaryingMaxSampleAttempts()
{
  constexpr int32_t ndims = INT32_C(2);
  constexpr std::array<Real, ndims> bounds_min{ -10, -10 };
  constexpr std::array<Real, ndims> bounds_max{ 10, 10 };

  tph_poisson_allocator rpalloc = make_rpalloc();
  for (auto *alloc : std::vector<tph_poisson_allocator *>{ nullptr, &rpalloc }) {
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
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&args_10, alloc, sampling_10.get()));
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&args_40, alloc, sampling_40.get()));

    REQUIRE(sampling_10->nsamples < sampling_40->nsamples);
  }
}

// Verify that different seeds give different sample distributions (with
// all other parameters constant).
static void TestVaryingSeed()
{
  constexpr int32_t ndims = INT32_C(2);
  constexpr std::array<Real, ndims> bounds_min{ -10, -10 };
  constexpr std::array<Real, ndims> bounds_max{ 10, 10 };

  tph_poisson_allocator rpalloc = make_rpalloc();
  for (auto *alloc : std::vector<tph_poisson_allocator *>{ nullptr, &rpalloc }) {
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
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&args_1981, alloc, sampling_1981.get()));
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&args_1337, alloc, sampling_1337.get()));

    const tph_poisson_real *samples_1981 = tph_poisson_get_samples(sampling_1981.get());
    const tph_poisson_real *samples_1337 = tph_poisson_get_samples(sampling_1337.get());
    REQUIRE(samples_1981 != nullptr);
    REQUIRE(samples_1337 != nullptr);

    // For each sample in the first point set compute the smallest
    // distance checking every sample in the second point set.
    // Then, if the smallest distance is larger than some threshold
    // we say that the sample from the first point set is distinct
    // from every sample in the second point set. Thus the two
    // distributions must be different.
    auto distinct_sample_found = false;
    for (ptrdiff_t i = 0; i < sampling_1981->nsamples; ++i) {
      const Real *p = &samples_1981[i * sampling_1981->ndims];
      Real min_sqr_dist = std::numeric_limits<Real>::max();
      for (ptrdiff_t j = 0; j < sampling_1337->nsamples; ++j) {
        const Real *q = &samples_1337[j * sampling_1337->ndims];
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
}

static void TestInvalidArgs()
{
// Work-around for MSVC compiler not being able to handle constexpr
// capture in lambdas.
// constexpr int32_t ndims = 2;
#define NDIMS INT32_C(2)

  constexpr std::array<Real, NDIMS> bounds_min{ -10, -10 };
  constexpr std::array<Real, NDIMS> bounds_max{ 10, 10 };

  tph_poisson_allocator rpalloc = make_rpalloc();
  for (auto *alloc : std::vector<tph_poisson_allocator *>{ nullptr, &rpalloc }) {
    tph_poisson_args valid_args = {};
    valid_args.radius = 1;
    valid_args.ndims = NDIMS;
    valid_args.bounds_min = bounds_min.data();
    valid_args.bounds_max = bounds_max.data();
    valid_args.max_sample_attempts = UINT32_C(30);
    valid_args.seed = UINT64_C(333);
    // Verify valid arguments.
    unique_poisson_ptr sampling = make_unique_poisson();
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&valid_args, alloc, sampling.get()));
    REQUIRE(sampling->ndims == NDIMS);
    REQUIRE(sampling->nsamples > 0);
    REQUIRE(tph_poisson_get_samples(sampling.get()) != nullptr);
    // Reset sampling.
    sampling = make_unique_poisson();
    REQUIRE(sampling->ndims == 0);
    REQUIRE(sampling->nsamples == 0);

    // sampling == NULL
    [&]() {
      tph_poisson_args args = valid_args;
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, /*sampling=*/nullptr));
    }();

    // Incomplete (custom) allocator.
    if (alloc != nullptr) {
      [&]() {
        tph_poisson_args args = valid_args;
        tph_poisson_allocator incomplete_alloc = *alloc;
        incomplete_alloc.malloc = nullptr;
        REQUIRE(
          TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, &incomplete_alloc, sampling.get()));
      }();
      [&]() {
        tph_poisson_args args = valid_args;
        tph_poisson_allocator incomplete_alloc = *alloc;
        incomplete_alloc.free = nullptr;
        REQUIRE(
          TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, &incomplete_alloc, sampling.get()));
      }();
    }

    // args == NULL
    [&]() {
      REQUIRE(
        TPH_POISSON_INVALID_ARGS == tph_poisson_create(/*args=*/nullptr, alloc, sampling.get()));
    }();

    // radius <= 0
    [&]() {
      tph_poisson_args args = valid_args;
      args.radius = 0;
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      args.radius = -1;
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));
    }();

    // ndims <= 0
    [&]() {
      tph_poisson_args args = valid_args;
      args.ndims = 0;
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      args.ndims = -1;
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));
    }();

    // max_sample_attempts == 0
    [&]() {
      tph_poisson_args args = valid_args;
      args.max_sample_attempts = 0;
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));
    }();

    // bounds_min >= bounds_max
    [&]() {
      tph_poisson_args args = valid_args;
      args.bounds_min = nullptr;
      args.bounds_max = nullptr;
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min0{ 10, 10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max0{ 10, 10 };
      args.bounds_min = invalid_bounds_min0.data();
      args.bounds_max = invalid_bounds_max0.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min1{ 10, -10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max1{ 10, 10 };
      args.bounds_min = invalid_bounds_min1.data();
      args.bounds_max = invalid_bounds_max1.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min2{ -10, 10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max2{ 10, 10 };
      args.bounds_min = invalid_bounds_min2.data();
      args.bounds_max = invalid_bounds_max2.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min3{ 10, 10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max3{ -10, -10 };
      args.bounds_min = invalid_bounds_min3.data();
      args.bounds_max = invalid_bounds_max3.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min4{ 10, -10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max4{ -10, 10 };
      args.bounds_min = invalid_bounds_min4.data();
      args.bounds_max = invalid_bounds_max4.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min5{ -10, 10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max5{ 10, -10 };
      args.bounds_min = invalid_bounds_min5.data();
      args.bounds_max = invalid_bounds_max5.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      // clang-format off
      constexpr std::array<Real, NDIMS> invalid_bounds_min6{ 
        -10, std::numeric_limits<Real>::quiet_NaN() };
      constexpr std::array<Real, NDIMS> invalid_bounds_max6{ 10, 10 };
      args.bounds_min = invalid_bounds_min6.data();
      args.bounds_max = invalid_bounds_max6.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min7{ 
        std::numeric_limits<Real>::quiet_NaN(), -10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max7{ 10, 10 };
      args.bounds_min = invalid_bounds_min7.data();
      args.bounds_max = invalid_bounds_max7.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min8{ -10, -10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max8{ 
        std::numeric_limits<Real>::quiet_NaN(), 10 };
      args.bounds_min = invalid_bounds_min8.data();
      args.bounds_max = invalid_bounds_max8.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));

      constexpr std::array<Real, NDIMS> invalid_bounds_min9{ -10, -10 };
      constexpr std::array<Real, NDIMS> invalid_bounds_max9{ 
        10, std::numeric_limits<Real>::quiet_NaN()};
      args.bounds_min = invalid_bounds_min9.data();
      args.bounds_max = invalid_bounds_max9.data();
      REQUIRE(TPH_POISSON_INVALID_ARGS == tph_poisson_create(&args, alloc, sampling.get()));
      // clang-format on
    }();

    // Verify that no output was written by failed attempts.
    REQUIRE(sampling->ndims == 0);
    REQUIRE(sampling->nsamples == 0);
    REQUIRE(tph_poisson_get_samples(sampling.get()) == nullptr);
  }
#undef NDIMS
}

static void TestDestroy()
{
  constexpr int32_t ndims = 2;
  constexpr std::array<Real, ndims> bounds_min{ -10, -10 };
  constexpr std::array<Real, ndims> bounds_max{ 10, 10 };

  tph_poisson_allocator rpalloc = make_rpalloc();
  for (auto *alloc : std::vector<tph_poisson_allocator *>{ nullptr, &rpalloc }) {
    tph_poisson_args valid_args = {};
    valid_args.radius = 1;
    valid_args.ndims = ndims;
    valid_args.bounds_min = bounds_min.data();
    valid_args.bounds_max = bounds_max.data();
    valid_args.max_sample_attempts = UINT32_C(30);
    valid_args.seed = UINT64_C(333);
    tph_poisson_sampling sampling = {};
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&valid_args, alloc, &sampling));
    REQUIRE(tph_poisson_get_samples(&sampling) != nullptr);

    tph_poisson_destroy(&sampling);
    REQUIRE(sampling.internal == nullptr);

    // No samples after destroy.
    REQUIRE(tph_poisson_get_samples(&sampling) == nullptr);

    // Double create without intermediate destroy.
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&valid_args, alloc, &sampling));
    REQUIRE(sampling.internal != nullptr);
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&valid_args, alloc, &sampling));
    REQUIRE(sampling.internal != nullptr);

    // Double destroy (no crash).
    tph_poisson_destroy(&sampling);
    REQUIRE(sampling.internal == nullptr);
    tph_poisson_destroy(&sampling);
    REQUIRE(sampling.internal == nullptr);
  }
}

static void TestUserAlloc()
{
  const auto create_sampling = [](tph_poisson_allocator *alloc) {
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
    REQUIRE(TPH_POISSON_SUCCESS == tph_poisson_create(&valid_args, alloc, sampling.get()));
    return sampling;
  };

  // Verify that we get exactly (bit-wise) the same results with default (libc malloc) and user
  // allocator (rpmalloc).
  tph_poisson_allocator rpalloc = make_rpalloc();
  unique_poisson_ptr sampling = create_sampling(/*alloc=*/nullptr);
  unique_poisson_ptr sampling_alloc = create_sampling(&rpalloc);
  REQUIRE(sampling->ndims == sampling_alloc->ndims);
  REQUIRE(sampling->nsamples == sampling_alloc->nsamples);
  const tph_poisson_real *samples = tph_poisson_get_samples(sampling.get());
  const tph_poisson_real *samples_alloc = tph_poisson_get_samples(sampling_alloc.get());
  REQUIRE(samples != nullptr);
  REQUIRE(samples_alloc != nullptr);
  REQUIRE(std::memcmp(reinterpret_cast<const void *>(samples),
            reinterpret_cast<const void *>(samples_alloc),
            static_cast<size_t>(sampling->nsamples) * static_cast<size_t>(sampling->ndims)
              * sizeof(tph_poisson_real))
          == 0);
}

static void TestBadAlloc()
{
  const auto create_sampling = [](tph_poisson_allocator *alloc) {
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
    return tph_poisson_create(&valid_args, alloc, sampling.get());
  };

  // Verify that default allocator is fine.
  REQUIRE(TPH_POISSON_SUCCESS == create_sampling(/*alloc=*/nullptr));

  const auto add_sample_fail = [&](const ptrdiff_t max_count) {
    struct AllocCtx
    {
      ptrdiff_t count = 0;
      ptrdiff_t max_count = 0;
    };
    tph_poisson_allocator alloc = {};
    alloc.malloc = [](ptrdiff_t size, void *ctx) {
      AllocCtx *alloc_ctx = reinterpret_cast<AllocCtx *>(ctx);
      return alloc_ctx->count++ < alloc_ctx->max_count ? std::malloc(static_cast<size_t>(size))
                                                       : nullptr;
    };
    alloc.free = [](void *ptr, ptrdiff_t size, void *ctx) {
      static_cast<void>(size);
      static_cast<void>(ctx);
      std::free(ptr);
    };
    AllocCtx alloc_ctx;
    alloc_ctx.max_count = max_count;
    alloc.ctx = &alloc_ctx;
    REQUIRE(TPH_POISSON_BAD_ALLOC == create_sampling(&alloc));
  };

  // NOTE: The constant 3 is very specifically set to cause the bad_alloc
  //       when adding the first sample to improve code coverage.
  add_sample_fail(/*max_count=*/3);
  // NOTE: The constant 5 is very specifically set to cause the bad_alloc
  //       when adding the second (or later) sample to improve code coverage.
  add_sample_fail(/*max_count=*/5);
}

static void TestVec()
{
  tph_poisson_vec v;
  REQUIRE(false);

}

int main(int argc, char *argv[])
{
  static_cast<void>(argc);
  static_cast<void>(argv);

  // rpmalloc_initialize();

  std::printf("TestInvalidArgs...\n");
  TestInvalidArgs();

  std::printf("TestRadius...\n");
  TestRadius();

  std::printf("TestBounds...\n");
  TestBounds();

  std::printf("TestVaryingMaxSampleAttempts...\n");
  TestVaryingMaxSampleAttempts();

  std::printf("TestVaryingSeed...\n");
  TestVaryingSeed();

  std::printf("TestDestroy...\n");
  TestDestroy();

  std::printf("TestUserAlloc...\n");
  TestUserAlloc();

  std::printf("TestBadAlloc...\n");
  TestBadAlloc();

  std::printf("TestVec...\n");
  TestVec();

  // rpmalloc_finalize();

  return EXIT_SUCCESS;
}

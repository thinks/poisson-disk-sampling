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

  struct AllocCtx
  {
    ptrdiff_t nsamples = 0;
    ptrdiff_t max_samples = 0;
  };

#if 0
  const auto add_sample_fail = [&](AllocCtx *alloc_ctx) {
    tph_poisson_allocator alloc = {};
    alloc.malloc = [](ptrdiff_t size, void *ctx) -> void * {
      AllocCtx *a_ctx = reinterpret_cast<AllocCtx *>(ctx);
#if 1
      if (size == (2 * sizeof(Real) + alignof(Real))) {
        ++a_ctx->nsamples;
        if (a_ctx->nsamples >= a_ctx->max_samples) { return nullptr; }
      }
#else
      std::printf("[0x%" PRIXPTR "] malloc: %td [bytes]\n", (uintptr_t)a_ctx, size);
#endif
      return std::malloc(static_cast<size_t>(size));

      // AllocCtx *a_ctx = reinterpret_cast<AllocCtx *>(ctx);
      // return a_ctx->count++ < a_ctx->max_count ? std::malloc(static_cast<size_t>(size)) :
      // nullptr;
    };
    alloc.free = [](void *ptr, ptrdiff_t size, void *ctx) {
      static_cast<void>(size);
      static_cast<void>(ctx);
      std::free(ptr);
    };
    alloc.ctx = alloc_ctx;
    REQUIRE(TPH_POISSON_BAD_ALLOC == create_sampling(&alloc));
  };
#endif

  // NOTE: The constant 3 is very specifically set to cause the bad_alloc
  //       when adding the first sample to improve code coverage.
  // AllocCtx alloc_ctx{ /*nsamples=*/0, /*.max_samples=*/1 };
  // add_sample_fail(&alloc_ctx);
  // NOTE: The constant 5 is very specifically set to cause the bad_alloc
  //       when adding the second (or later) sample to improve code coverage.
  // AllocCtx alloc_ctx2{ /*nsamples=*/0, /*.max_samples=*/5 };
  // add_sample_fail(&alloc_ctx2);
}


int main(int /*argc*/, char * /*argv*/[])
{
  std::printf("TestUserAlloc...\n");
  TestUserAlloc();

  std::printf("TestBadAlloc...\n");
  TestBadAlloc();

  return EXIT_SUCCESS;
}

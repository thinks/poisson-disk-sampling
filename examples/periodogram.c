#include <math.h> /* sqrt, ceil, floor, round */
#include <string.h> /* memset, memcpy */

#include <fftw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define TPH_POISSON_REAL_TYPE double
#define TPH_POISSON_SQRT sqrt
#define TPH_POISSON_CEIL ceil
#define TPH_POISSON_FLOOR floor
#define TPH_POISSON_IMPLEMENTATION
#include "thinks/tph_poisson.h"

static_assert(sizeof(tph_poisson_real) == sizeof(double), "");

static void fft_shift(const int n0, const int n1, double *inout)
{
  /* Simple and slow implementation, not optimized. */

  const int s0 = n0 / 2;
  const int s1 = n1 / 2;

  /* Shift rows. */
  double x = 0.0; /* swap */
  int jj = 0;
  for (int j = 0; j < n1; ++j) {
    jj = n0 * j;
    for (int i = 0; i < s0; ++i) {
      x = inout[i + jj];
      inout[i + jj] = inout[i + s0 + jj];
      inout[i + s0 + jj] = x;
    }
  }

  /* Shift columns. */
  for (int i = 0; i < n0; ++i) {
    for (int j = 0; j < s1; ++j) {
      jj = n0 * j;
      x = inout[i + jj];
      inout[i + jj] = inout[i + n0 * (s1 + j)];
      inout[i + n0 * (s1 + j)] = x;
    }
  }
}

static void sampling_image(const tph_poisson_args *args,
  const tph_poisson_sampling *s,
  const int n0,
  const int n1,
  fftw_complex *img)
{
  /* Reset, start from zero image. Imaginary part will remain zero. */
  const int sz = n0 * n1;
  for (int i = 0; i < sz; ++i) {
    img[i][0] = 0.0;
    img[i][1] = 0.0;
  }

  const double *p = tph_poisson_get_samples(s);
  if (p == NULL) { abort(); }

  const ptrdiff_t nsamples = s->nsamples;
  const ptrdiff_t ndims = s->ndims;
  const double x_min = args->bounds_min[0];
  const double y_min = args->bounds_min[1];
  const double x_max = args->bounds_max[0];
  const double y_max = args->bounds_max[1];
  for (ptrdiff_t i = 0; i < nsamples; ++i) {
    int ix = (int)floor(((p[i * ndims] - x_min) / (x_max - x_min)) * (double)n0);
    ix = ix < 0 ? 0 : ((n0 - 1) < ix ? (n0 - 1) : ix);

    int iy = (int)floor(((p[i * ndims + 1] - y_min) / (y_max - y_min)) * (double)n1);
    iy = iy < 0 ? 0 : ((n1 - 1) < iy ? (n1 - 1) : iy);

    img[ix + n0 * iy][0] = 1.0;
  }

  /* Subtract average. */
  double avg = 0.0;
  for (int i = 0; i < sz; ++i) { avg += img[i][0]; }
  avg /= (double)sz;
  for (int i = 0; i < sz; ++i) { img[i][0] -= avg; }
}

static void
  accum_periodogram(const double scale, const int n0, const int n1, fftw_complex *in, double *out)
{
  const int sz = n0 * n1;
  for (int i = 0; i < sz; ++i) { out[i] += scale * (in[i][0] * in[i][0] + in[i][1] * in[i][1]); }
}

static bool write_png(const char *filename, const int n0, const int n1, const double *data)
{
  static const int comp = 1; /* Greyscale. */

  const ptrdiff_t sz = (ptrdiff_t)n0 * (ptrdiff_t)n1;
  double min_val = data[0];
  double max_val = data[0];
  for (ptrdiff_t i = 1; i < sz; ++i) {
    if (data[i] < min_val) { min_val = data[i]; }
    if (data[i] > max_val) { max_val = data[i]; }
  }

  const size_t buf_size = (size_t)sz * sizeof(uint8_t);
  uint8_t *buf = (uint8_t *)malloc(buf_size);
  memset(buf, 0, buf_size);

  for (ptrdiff_t i = 0; i < sz; ++i) {
    const int iv = (int)round(((data[i] - min_val) / (max_val - min_val)) * 255.0);
    buf[i] = (uint8_t)(iv < 0 ? 0 : (255 < iv ? 255 : iv));
  }

  const int ret = stbi_write_png(filename, n0, n1, comp, buf, n0);
  free(buf);

  return ret != 0;
}

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  /* Periodogram settings. */
  const int image_count = 100;
  const int n0 = 2048;
  const int n1 = 2048;

  /* clang-format off */
  const tph_poisson_real bounds_min[2] = { 
    (tph_poisson_real)0, (tph_poisson_real)0 };
  const tph_poisson_real bounds_max[2] = { 
    (tph_poisson_real)128, (tph_poisson_real)128 };
  /* clang-format on */

  /* Configure tph_poisson arguments. Set varying seed later. */
  tph_poisson_args args = { .bounds_min = bounds_min,
    .bounds_max = bounds_max,
    .radius = (tph_poisson_real)1,
    .ndims = INT32_C(2),
    .max_sample_attempts = UINT32_C(30),
    .seed = UINT64_C(1981) };

  /* Initialize empty tph_poisson sampling. */
  tph_poisson_sampling sampling;
  memset(&sampling, 0, sizeof(tph_poisson_sampling));

  /* Initlialize buffers used to accumulate the average periodogram. */
  double *periodogram = (double *)malloc((size_t)n0 * (size_t)n1 * sizeof(double));
  for (int i = 0; i < (n0 * n1); ++i) { periodogram[i] = 0.0; }

  /* Initialize FFT buffers and plan. */
  fftw_complex *in = fftw_alloc_complex((size_t)n0 * (size_t)n1);
  fftw_complex *out = fftw_alloc_complex((size_t)n0 * (size_t)n1);
  fftw_plan plan = fftw_plan_dft_2d(n0, n1, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

  const double scale = 1. / (double)image_count;
  for (int i = 0; i < image_count; ++i) {
    /* Vary the seed for each image. */
    args.seed = (uint64_t)i;

    /* Populate sampling with points. Using default allocator (libc malloc). */
    if (tph_poisson_create(&args, /*alloc=*/NULL, &sampling) != TPH_POISSON_SUCCESS) { abort(); }

    /* Construct FFT input from sampling. */
    sampling_image(&args, &sampling, n0, n1, in);

    /* Perform FFT. */
    fftw_execute(plan);

    /* Accumulate (scaled) results. */
    accum_periodogram(scale, n0, n1, out, periodogram);
  }

  /* Shift DC bin to the center of the image and write png file. */
  fft_shift(n0, n1, periodogram);
  if (!write_png("./tph_poisson_periodogram.png", n0, n1, periodogram)) { abort(); }

  /* Free resources. */
  fftw_destroy_plan(plan);
  free(in);
  free(out);
  free(periodogram);
  tph_poisson_destroy(&sampling);

  return EXIT_SUCCESS;
}

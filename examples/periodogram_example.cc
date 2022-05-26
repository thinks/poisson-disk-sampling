// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <iostream>

#include "simple_fft/fft.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define STBI_MSC_SECURE_CRT
#include "stb_image_write.h"

#include "tph/poisson_disk_sampling.hpp"

[[nodiscard]] static constexpr auto Reinterval(const double in_val,
                                               const double old_min,
                                               const double old_max,
                                               const double new_min,
                                               const double new_max) noexcept -> double {
  return (old_max - old_min) == 0.0
             ? new_max
             : (((in_val - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;
}

template <typename PixelT>
class Image {
public:
  Image(const std::size_t width, const std::size_t height)
      : _width(width), _height(height), _pixels(_width * _height, PixelT{0}) {}

  // NOLINTNEXTLINE
  [[nodiscard]] constexpr auto width() const -> std::size_t { return _width; }

  // NOLINTNEXTLINE
  [[nodiscard]] constexpr auto height() const -> std::size_t { return _height; }

  // NOLINTNEXTLINE
  [[nodiscard]] auto data() const -> const PixelT* { return _pixels.data(); }

  // NOLINTNEXTLINE
  [[nodiscard]] auto data() -> PixelT* { return _pixels.data(); }

  [[nodiscard]] auto operator()(const std::size_t i, const std::size_t j) noexcept -> PixelT& {
    return _pixels[_linearIndex(i, j)];
  }

  [[nodiscard]] auto operator()(const std::size_t i, const std::size_t j) const -> const PixelT& {
    return _pixels[_linearIndex(i, j)];
  }

private:
  [[nodiscard]]
  // NOLINTNEXTLINE
  constexpr std::size_t
  _linearIndex(const std::size_t i, const std::size_t j) const noexcept {
    return i + _width * j;
  }

  std::size_t _width;
  std::size_t _height;
  std::vector<PixelT> _pixels;
};

//[[nodiscard]] 
static auto AddEq(const Image<double>& rhs, Image<double>* lhs) noexcept
    -> Image<double>& {
  if (!(rhs.width() == lhs->width() && rhs.height() == lhs->height())) {
    std::cerr << "AddEq width/height mismatch";
    std::abort();
  }

  const auto w = rhs.width();
  const auto h = rhs.height();
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      (*lhs)(i, j) += rhs(i, j);
    }
  }
  return *lhs;
}

[[nodiscard]] static auto Scaled(const Image<double>& img, const double scalar) noexcept
    -> Image<double> {
  Image<double> out = img;
  const auto w = out.width();
  const auto h = out.height();
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      out(i, j) *= scalar;
    }
  }
  return out;
}

[[nodiscard]] static auto SubtractAverage(const Image<double>& img) noexcept -> Image<double> {
  auto out = img;
  const auto w = out.width();
  const auto h = out.height();
  double avg = 0.0;
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      avg += out(i, j);
    }
  }
  avg /= static_cast<double>(w * h);

  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      out(i, j) -= avg;
    }
  }

  return out;
}

[[nodiscard]] static auto Fft2d(const Image<double>& img_in) noexcept
    -> Image<std::complex<double>> {
  // Create a complex image with imaginary components set to zero.
  auto c_img = Image<std::complex<double>>(img_in.width(), img_in.height());
  const auto w = c_img.width();
  const auto h = c_img.height();
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      c_img(i, j) = {img_in(i, j), 0.0};
    }
  }

  const char* error_description = nullptr;
  if (!simple_fft::FFT(c_img, c_img.width(), c_img.height(), error_description)) {
    std::cerr << "FFT error: " << error_description;
    std::abort();
  }

  return c_img;
}

[[nodiscard]] static auto FftShift2d(const Image<std::complex<double>>& fft_img) noexcept
    -> Image<std::complex<double>> {
  if (!(fft_img.width() % 2 == 0 && fft_img.height() % 2 == 0)) {
    std::cerr << "FFT shift error: odd dimension";
    std::abort();
  }

  auto shift_img = fft_img;
  const auto w = fft_img.width();
  const auto h = fft_img.height();
  const auto half_w = w / 2;
  const auto half_h = h / 2;

  // Simple and slow implementation, not optimized.
  //
  // Shift rows.
  for (auto j = 0U; j < h; ++j) {
    for (auto i = 0U; i < half_w; ++i) {
      std::swap(shift_img(i, j), shift_img(half_w + i, j));
    }
  }

  // Shift cols.
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < half_h; ++j) {
      std::swap(shift_img(i, j), shift_img(i, half_h + j));
    }
  }

  return shift_img;
}

// The periodogram is the squared magnitude of the FFT bins.
[[nodiscard]] static auto Periodogram(const Image<std::complex<double>>& fft_img) noexcept
    -> Image<double> {
  auto img = Image<double>(fft_img.width(), fft_img.height());
  const auto w = img.width();
  const auto h = img.height();
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      const auto p = std::abs(fft_img(i, j));
      img(i, j) = p * p;
    }
  }
  return img;
}

[[nodiscard]] static auto CreateSampleImage(const std::size_t width,
                                            const std::size_t height,
                                            const std::array<double, 2>& sample_min,
                                            const std::array<double, 2>& sample_max,
                                            const std::vector<double>& samples) noexcept
    -> Image<double> {
  Image<double> img(width, height);
  const auto sample_count = samples.size() / 2U;
  for (std::decay_t<decltype(sample_count)> k = 0; k < sample_count; ++k) {
    const auto i = static_cast<std::size_t>(Reinterval(
        samples[2 * k], sample_min[0], sample_max[0], 0.0, static_cast<double>(width - 1)));
    const auto j = static_cast<std::size_t>(Reinterval(
        samples[2 * k + 1], sample_min[1], sample_max[1], 0.0, static_cast<double>(height - 1)));
    img(i, j) = 1.0;
  }
  return img;
}

[[nodiscard]] static auto To8bits(const Image<double>& img) noexcept -> Image<std::uint8_t> {
  double min_pixel = std::numeric_limits<double>::max();
  double max_pixel = std::numeric_limits<double>::lowest();
  const auto w = img.width();
  const auto h = img.height();
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      min_pixel = std::min(min_pixel, img(i, j));
      max_pixel = std::max(max_pixel, img(i, j));
    }
  }

  constexpr auto kMinMappedPixel = 0.0;
  constexpr auto kMaxMappedPixel = 255.999;
  Image<std::uint8_t> out(w, h);
  for (auto i = 0U; i < w; ++i) {
    for (auto j = 0U; j < h; ++j) {
      out(i, j) = static_cast<std::uint8_t>(
          Reinterval(img(i, j), min_pixel, max_pixel, kMinMappedPixel, kMaxMappedPixel));
    }
  }
  return out;
}

static void WriteImage(const std::string& filename, const Image<double>& img) {
  constexpr auto kComp = 1; // Greyscale.

  const auto w = static_cast<int>(img.width());
  const auto h = static_cast<int>(img.height());

  // HACK - clang-tidy will analyze this call and find errors in stb.
  //        If we ignore this line it doesn't seem to analyze the stb code,
  //        even though it is inlined in this file.
  //
  // NOLINTNEXTLINE
  if (stbi_write_png(filename.c_str(), w, h, kComp, To8bits(img).data(), w) == 0) {
    std::cerr << "failed writing image";
    std::abort();
  }
}

int main(int /*argc*/, char* /*argv*/[]) { // NOLINT
  try {
    constexpr auto kImageCount = 100U;
    constexpr auto kPixelSize = 2048U;

    constexpr auto kRadius = 1.0;
    constexpr std::array<double, 2> kXMin = {0.0, 0.0};
    constexpr std::array<double, 2> kXMax = {128.0, 128.0};
    constexpr auto kMaxSampleAttempts = std::uint32_t{30};

    Image<double> avg_periodogram_img(kPixelSize, kPixelSize);
    for (auto i = 0U; i < kImageCount; ++i) {
      AddEq(Scaled(Periodogram(FftShift2d(Fft2d(SubtractAverage(
                       CreateSampleImage(avg_periodogram_img.width(),
                                         avg_periodogram_img.height(),
                                         kXMin,
                                         kXMax,
                                         tph::PoissonDiskSampling(kRadius,
                                                                  kXMin,
                                                                  kXMax,
                                                                  kMaxSampleAttempts,
                                                                  /*seed=*/i)))))),
                   1.0 / kImageCount),
            &avg_periodogram_img);
    }
    WriteImage("avg_periodogram.png", avg_periodogram_img);
  } catch (std::exception& e) {
    std::cerr << "std::exception: " << e.what();
    std::abort();
  } catch (...) {
    std::cerr << "unknown exception";
    std::abort();
  }

  return EXIT_SUCCESS;
}

// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <cmath>
#include <complex>
#include <iostream>

#include "simple_fft/fft.h"
#include "thinks/pnm_io/pnm_io.h"
#include "thinks/poisson_disk_sampling/poisson_disk_sampling.h"


template <typename PixelT>
class Image {
 public:
  Image(const std::size_t width, const std::size_t height)
      : _width(width), _height(height), _pixels(_width * _height, PixelT{0}) {}

  std::size_t width() const { return _width; }
  std::size_t height() const { return _height; }
  const PixelT* data() const { return _pixels.data(); }
  PixelT* data() { return _pixels.data(); }

  std::size_t row_stride() const { return 1; }
  std::size_t col_stride() const { return _width; }

  PixelT& operator()(const std::size_t i, const std::size_t j) {
    return _pixels[_linearIndex(i, j)];
  }

  const PixelT& operator()(const std::size_t i, const std::size_t j) const {
    return _pixels[_linearIndex(i, j)];
  }

 private:
  std::size_t _linearIndex(const std::size_t i, const std::size_t j) const {
    return i + _width * j;
  }

  std::size_t _width;
  std::size_t _height;
  std::vector<PixelT> _pixels;
};

static Image<double>& AddEq(const Image<double>& rhs,
                                          Image<double>* lhs) {
  const auto min_width = std::min(rhs.width(), lhs->width());
  const auto min_height = std::min(rhs.height(), lhs->height());
  for (auto i = 0u; i < min_width; ++i) {
    for (auto j = 0u; j < min_height; ++j) {
      (*lhs)(i, j) += rhs(i, j);
    }
  }
  return *lhs;
}

[[nodiscard]] static Image<double> Scaled(const Image<double>& img,
                                          const double scalar) {
  Image<double> scaled_img(img.width(), img.height());
  for (auto i = 0u; i < scaled_img.width(); ++i) {
    for (auto j = 0u; j < scaled_img.height(); ++j) {
      scaled_img(i, j) = scalar * img(i, j);
    }
  }
  return scaled_img;
}

[[nodiscard]] constexpr auto reinterval(
    const double in_val, const double old_min, const double old_max,
    const double new_min, const double new_max) noexcept -> double {
  return (old_max - old_min) == 0.0
             ? new_max
             : (((in_val - old_min) * (new_max - new_min)) /
                (old_max - old_min)) +
                   new_min;
}

Image<double> CreateCosImage(const std::size_t width, const std::size_t height,
                             const double p, const double q) {
  constexpr auto kPi = 3.14159265359;

  auto out = Image<double>(width, height);
  const auto m = out.width();
  const auto n = out.height();
  const auto m_inv = 1.0 / m;
  const auto n_inv = 1.0 / n;

  for (std::size_t s = 0; s < m; ++s) {
    for (std::size_t t = 0; t < n; ++t) {
      out(s, t) = std::cos(2.0 * kPi * ((p * s) * m_inv + (q * t) * n_inv));
    }
  }

  return out;
}

Image<double> AverageOfImages(const std::vector<Image<double>>& imgs) {
  const auto img_count = imgs.size();

  Image<double> avg_img(imgs[0].width(), imgs[0].height());

  for (auto i = 0u; i < avg_img.width(); ++i) {
    for (auto j = 0u; j < avg_img.height(); ++j) {
      double sum = 0.0;
      for (auto k = 0u; k < img_count; ++k) {
        sum += imgs[k](i, j);
      }
      avg_img(i, j) = sum / img_count;
    }
  }

  return avg_img;
}

Image<double> SubtractAverage(const Image<double>& in) {
  auto out = in;
  const auto m = out.width();
  const auto n = out.height();

  double avg = 0.0;
  for (std::size_t s = 0; s < m; ++s) {
    for (std::size_t t = 0; t < n; ++t) {
      avg += out(s, t);
    }
  }
  avg /= m * n;

  for (std::size_t s = 0; s < m; ++s) {
    for (std::size_t t = 0; t < n; ++t) {
      out(s, t) -= avg;
    }
  }

  return out;
}

Image<std::complex<double>> Fft2d(const Image<double>& img_in) {
  // Create a complex image with imaginary components set to zero.
  auto c_img = Image<std::complex<double>>(img_in.width(), img_in.height());
  for (auto i = 0u; i < c_img.width(); ++i)
    for (auto j = 0u; j < c_img.height(); ++j) {
      c_img(i, j) = {img_in(i, j), 0.0};
    }

  const char* error_description = nullptr;
  if (!simple_fft::FFT(c_img, c_img.width(), c_img.height(),
                       error_description)) {
    std::cerr << "FFT error: " << error_description << '\n';
    std::abort();
  }

  return c_img;
}

Image<std::complex<double>> FftShift2d(const Image<std::complex<double>>& fft_img) {
  if(!(fft_img.width() % 2 == 0 && fft_img.height() % 2 == 0)) {
    std::cerr << "FFT shift error: odd dimension\n";
    std::abort();
  }

  auto shift_img = fft_img;
  const auto half_width = fft_img.width() / 2;
  const auto half_height = fft_img.height() / 2;

  // Shift rows.
  for (auto j = 0u; j < fft_img.height(); ++j) {
    for (auto i = 0u; i < half_width; ++i) {
      std::swap(shift_img(i, j), shift_img(half_width + i, j));
    }
  }

  // Shift cols.
  for (auto i = 0u; i < fft_img.width(); ++i) {
    for (auto j = 0u; j < half_height; ++j) {
      std::swap(shift_img(i, j), shift_img(i, half_height + j));
    }
  }

  return shift_img;
}

// The periodogram is the squared magnitude of the FFT bins.
Image<double> Periodogram(const Image<std::complex<double>>& fft_img) {
  auto img = Image<double>(fft_img.width(), fft_img.height());
  for (auto i = 0u; i < img.width(); ++i) {
    for (auto j = 0u; j < img.height(); ++j) {
      const auto p = std::abs(fft_img(i, j));
      img(i, j) = p * p;
    }
  }
  return img;
}

Image<double> CreateSampleImage(
    const std::size_t width, const std::size_t height,
    const std::array<double, 2>& sample_min,
    const std::array<double, 2>& sample_max,
    const std::vector<std::array<double, 2>>& samples) {
  Image<double> out(width, height);

  for (const auto& sample : samples) {
    const auto i = static_cast<std::size_t>(
        reinterval(sample[0], sample_min[0], sample_max[0], 0.0,
                   static_cast<double>(width - 1)));
    const auto j = static_cast<std::size_t>(
        reinterval(sample[1], sample_min[1], sample_max[1], 0.0,
                   static_cast<double>(height - 1)));
    out(i, j) = 1.0;
  }

  return out;
}

Image<std::uint8_t> To8bits(const Image<double>& in) {
  double min = std::numeric_limits<double>::max();
  double max = std::numeric_limits<double>::lowest();

  const std::size_t m = in.width();
  const std::size_t n = in.height();
  for (std::size_t s = 0; s < m; ++s) {
    for (std::size_t t = 0; t < n; ++t) {
      min = std::min(min, in(s, t));
      max = std::max(max, in(s, t));
    }
  }

  Image<std::uint8_t> out(in.width(), in.height());
  for (std::size_t s = 0; s < m; ++s) {
    for (std::size_t t = 0; t < n; ++t) {
      out(s, t) =
          static_cast<std::uint8_t>(reinterval(in(s, t), min, max, 0.0, 255.0));
    }
  }
  return out;
}

void WriteImage(const std::string& filename, const Image<double>& img) {
  const Image<std::uint8_t> tmp = To8bits(img);
  thinks::WritePgmImage(filename, tmp.width(), tmp.height(), tmp.data());
}

int main(int /*argc*/, char* /*argv*/[]) {  // NOLINT
  try {
#if 1
    {
      constexpr auto kRadius = 2.0;
      constexpr std::array<double, 2> kXMin = {0.0, 0.0};
      constexpr std::array<double, 2> kXMax = {128.0, 128.0};
      constexpr auto kMaxSampleAttempts = std::uint32_t{30};
      constexpr auto kSeed = std::uint32_t{0};
      const auto samples = thinks::PoissonDiskSampling(
          kRadius, kXMin, kXMax, kMaxSampleAttempts, kSeed);

      const auto sample_img =
          SubtractAverage(CreateSampleImage(256, 256, kXMin, kXMax, samples));
      WriteImage("samples.pgm", sample_img);
    }
    {
      constexpr auto kNumImages = 100u;

      constexpr auto kRadius = 1.0;
      constexpr std::array<double, 2> kXMin = {0.0, 0.0};
      constexpr std::array<double, 2> kXMax = {128.0, 128.0};
      constexpr auto kMaxSampleAttempts = std::uint32_t{30};

      Image<double> avg_periodogram_img(1024, 1024);
      for (auto i = 0u; i < kNumImages; ++i) {
        auto img = CreateSampleImage(
                       avg_periodogram_img.width(), avg_periodogram_img.height(), kXMin, kXMax,
                       thinks::PoissonDiskSampling(kRadius, kXMin, kXMax,
                                                   kMaxSampleAttempts,
                                                   /* seed */ i));
        img = SubtractAverage(img);
        auto c_img = Fft2d(img);
        c_img = FftShift2d(c_img);
        img = Periodogram(c_img);
        AddEq(Scaled(img, 1.0 / kNumImages), &avg_periodogram_img);

#if 0
        AddEq(
            Scaled(Periodogram(FftShift2d(Fft2d(SubtractAverage(CreateSampleImage(
                       avg_periodogram_img.width(), avg_periodogram_img.height(), kXMin, kXMax,
                       thinks::PoissonDiskSampling(kRadius, kXMin, kXMax,
                                                   kMaxSampleAttempts,
                                                   /* seed */ i)))))),
                   1.0 / kNumImages),
            &avg_periodogram_img);
#endif
      }

      WriteImage("avg_periodogram.pgm", avg_periodogram_img);
    }
#endif
  } catch (const char* e) {
    std::cerr << e << '\n';
    std::abort();
  }

  return EXIT_SUCCESS;
}

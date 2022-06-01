#pragma once

#include <tph/poisson_disk_sampling.hpp>

TPH_NODISCARD constexpr auto MyTest() noexcept -> bool {
  return true;    
  // return tph::PoissonDiskSampling(
  //            /*radius=*/10.0F, std::array<float, 2>{-1.0F, -1.0F}, std::array<float, 2>{1.0F, 1.0F})
  //            .front() == 42;
}

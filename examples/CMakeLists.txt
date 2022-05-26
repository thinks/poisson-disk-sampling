# Copyright (C) Tommy Hinks <tommy.hinks@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

include(InstallNlohmannJson)
include(InstallSimpleFft)
include(InstallStb)

##
## json_example
## 
add_executable(json_example json_example.cc)
target_link_libraries(json_example
  PRIVATE
     nlohmann_json::nlohmann_json
     tph_poisson_disk_sampling::tph_poisson_disk_sampling
)

##
## periodogram_example
##
add_executable(periodogram_example periodogram_example.cc)
target_link_libraries(periodogram_example
  PRIVATE
    d1vanov::simple_fft
    nothings::stb
    tph_poisson_disk_sampling::tph_poisson_disk_sampling
)

##
## simple_example
##
add_executable(simple_example simple_example.cc)
target_link_libraries(simple_example
  PRIVATE
    tph_poisson_disk_sampling::tph_poisson_disk_sampling
)

# thinks_cc_executable(
#   NAME
#     vec_traits_in_namespace_example
#   SRCS
#     "vec_traits_in_namespace_example.cc"
#   COPTS
#     ${THINKS_DEFAULT_COPTS}
#   DEPS
#     thinks::poisson_disk_sampling
# )

# thinks_cc_executable(
#   NAME
#     vec_traits_passed_in_example
#   SRCS
#     "vec_traits_passed_in_example.cc"
#   COPTS
#     ${THINKS_DEFAULT_COPTS}
#   DEPS
#     thinks::poisson_disk_sampling
# )

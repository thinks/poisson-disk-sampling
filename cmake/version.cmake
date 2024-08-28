# ---- Version ----

set(tph_poisson_MAJOR_VERSION "")
set(tph_poisson_MINOR_VERSION "")
set(tph_poisson_PATCH_VERSION "")

set(_tph_poisson_hdr "include/thinks/tph_poisson.h")
file(STRINGS
     ${_tph_poisson_hdr}
     _ver_tmp
     REGEX "#define TPH_POISSON_MAJOR_VERSION .*$")
string(REGEX MATCHALL "[0-9]+" tph_poisson_MAJOR_VERSION ${_ver_tmp})
file(STRINGS
     ${_tph_poisson_hdr}
     _ver_tmp
     REGEX "#define TPH_POISSON_MINOR_VERSION .*$")
string(REGEX MATCHALL "[0-9]+" tph_poisson_MINOR_VERSION ${_ver_tmp})
file(STRINGS
     ${_tph_poisson_hdr}
     _ver_tmp
     REGEX "#define TPH_POISSON_PATCH_VERSION .*$")
string(REGEX MATCHALL "[0-9]+" tph_poisson_PATCH_VERSION ${_ver_tmp})

unset(_ver_tmp)
unset(_tph_poisson_hdr)

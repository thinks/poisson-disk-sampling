#define TPH_POISSON_IMPLEMENTATION
/* #define tph_poisson_assert ... */
/* #define TPH_POISSON_MEMCPY ... */
/* #define TPH_POISSON_MEMSET ... */
/* #define TPH_POISSON_MALLOC ... */
/* #define TPH_POISSON_FREE ... */
#include "tph_poisson_d.h"

/* Silence clang-tidy warning [misc-include-cleaner] */
typedef void *(*dummy)(tph_poisson_real);

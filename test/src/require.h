#include <assert.h> /* assert */
#include <stdio.h> /* printf */
#include <stdlib.h> /* abort */

static inline void
  require_fail(const char *expr, const char *file, unsigned int line, const char *function)
{
  printf(
    "Requirement failed: '%s' on line %u in file %s in function %s\n", expr, line, file, function);
  abort();
}

/* clang-format off */
#ifdef _MSC_VER
  #define TPH_PRETTY_FUNCTION __FUNCSIG__
#else 
  #define TPH_PRETTY_FUNCTION __func__
#endif
#define REQUIRE(expr) \
  ((bool)(expr) ? (void)0 : require_fail(#expr, __FILE__, __LINE__, TPH_PRETTY_FUNCTION))
/* clang-format on */

/* Dummy malloc/free functions. Used to set up incomplete allocators, should never be called.
 * Having the function definitions here means that these functions don't show up as missing
 * code coverage. */

static inline void *dummy_malloc(ptrdiff_t size, void *ctx)
{
  (void)size;
  (void)ctx;

  assert(false && "called dummy_malloc");
  static int a[2];
  return (void *)a;
}

static inline void dummy_free(void *ptr, ptrdiff_t size, void *ctx)
{
  (void)ptr;
  (void)size;
  (void)ctx;

  // Do nothing!
  assert(false && "called dummy_free");
}

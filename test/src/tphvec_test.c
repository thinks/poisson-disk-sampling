#include "tphvec.h"

static void *tph_alloc_fn(size_t n, void *ctx)
{
  (void)ctx;
  return malloc(n);
}

static void *tph_realloc_fn(void *p, size_t n, void *ctx)
{
  (void)ctx;
  return realloc(p, n);
}

static void tph_free_fn(void *p, void *ctx)
{
  (void)ctx;
  free(p);
}

static void print_vecf(float *vec)
{
  printf("cap = %" PRIu32 " | size = %" PRIu32 " | alloc = %p\n",
    (uint32_t)tphvec_capacity(vec),
    (uint32_t)tphvec_size(vec),
    (void *)tphvec_get_allocator(vec));
  const size_t sz = tphvec_size(vec);
  printf("v = { ");
  for (size_t i = 0; i < tphvec_size(vec); ++i) {
    printf(i != tphvec_size(vec) - 1 ? "%.3f, " : "%.3f", vec[i]);
  }
  printf(" }\n");
}

int main(int argc, char *argv[])
{
  // printf("cap: %zu, next_cap: %zu\n", 0, _tphvec_next_capacity(0, 0));
  // printf("cap: %zu, next_cap: %zu\n", 5, _tphvec_next_capacity(5, 13));
  // printf("cap: %zu, next_cap: %zu\n", 5, _tphvec_next_capacity(5, SIZE_MAX - 10));

  tphvec(float) v = NULL;
  tphvec_reserve(v, 10);
  print_vecf(v);
  printf("\n");

  float vals[] = { 41.F, 42.F, 43.F };
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  tphvec_append(v, vals, 3);
  print_vecf(v);
  printf("\n");

  tphvec_shrink_to_fit(v);
  print_vecf(v);
  printf("\n");

  tphvec_free(v);

  {
    tphvec(float) v2 = NULL;
    float value = 333.F;
    tphvec_resize(v2, 13, value);
    print_vecf(v2);
    printf("\n");
  }
  {
    tphvec(float) v2 = NULL;
    float value = 333.F;
    tphvec_resize(v2, 1, value);
    print_vecf(v2);
    printf("\n");
    float value2 = 44.F;
    tphvec_resize(v2, 4, value2);
    print_vecf(v2);
    printf("\n");
  }
  {
    tphvec(float) v2 = NULL;
    float value = 333.F;
    tphvec_resize(v2, 0, value);
    print_vecf(v2);
    printf("\n");
  }


#if 0
  float *v2 = NULL;
  tphvec_resize(v2, 16, 42.F);
  tphvec_free(v2);
  print_vecf(v2);
  printf("\n");
#endif
  return 0;
}
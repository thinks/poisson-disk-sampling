#include <inttypes.h>
#include <stdio.h>


#define TPH_POISSON_IMPLEMENTATION
#include <thinks/tph_poisson.h>

static void print_vecf(tph_poisson_vec *v)
{
  printf("mem = %" PRIxPTR " | mem_size = %" PRIu32 " | size = %" PRIu32 "\n",
    (uintptr_t)v->mem,
    (uint32_t)v->mem_size,
    (uint32_t)(tph_poisson_vec_size(float, v)));
  printf("v = { ");
  const tph_poisson_real *iter = (const tph_poisson_real *)v->begin;
  const tph_poisson_real *const iend = (const tph_poisson_real *)v->end;
  for (; iter != iend; ++iter) { printf(iter + 1 != iend ? "%.3f, " : "%.3f", *iter); }
  printf(" }\n");
}

int main(int argc, char *argv[])
{

  tph_poisson_vec v = { NULL };
  assert(tph_poisson_vec_invariants(float, &v));
  print_vecf(&v);

  float a[] = { 2, 3, 4, 5, 6, 7 };
  float b[] = { 8, 9, 10, 11, 12, 13 };
  tph_poisson_vec_append(float, &v, &tph_poisson_default_alloc, a, 6);
  assert(tph_poisson_vec_invariants(float, &v));
  print_vecf(&v);
  tph_poisson_vec_append(float, &v, &tph_poisson_default_alloc, b, 6);
  assert(tph_poisson_vec_invariants(float, &v));
  print_vecf(&v);

  tph_poisson_vec_erase_unordered(float, &v, 8, 3);
  assert(tph_poisson_vec_invariants(float, &v));
  print_vecf(&v);

  tph_poisson_vec_erase_unordered(float, &v, 7, 2);
  assert(tph_poisson_vec_invariants(float, &v));
  print_vecf(&v);

  tph_poisson_vec_shrink_to_fit(float, &v, &tph_poisson_default_alloc);
  assert(tph_poisson_vec_invariants(float, &v));
  print_vecf(&v);

  tph_poisson_vec_free(&v, &tph_poisson_default_alloc);
  assert(tph_poisson_vec_invariants(float, &v));
  print_vecf(&v);

  return 0;
}

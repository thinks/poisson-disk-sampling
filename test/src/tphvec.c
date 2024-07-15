#include "tphvec.h"

typedef struct
{
  tphvec_allocator_t *allocator;
  ptrdiff_t capacity;
  ptrdiff_t size;
} tphvec_header_t;


/**
 * @brief _tphvec_hdr_to_vec - For internal use, converts a header pointer to
 * a vector. Assumes that the header pointer is not NULL.
 * @param hdr - A pointer to a header.
 * @return The vector as a <void*>.
 * @internal
 */
static inline void *_tphvec_hdr_to_vec(const tphvec_header_t *hdr)
{
  tphvec_assert(hdr);
  return (void *)&(hdr[1]);
}

/**
 * @brief _tphvec_vec_to_hdr - For internal use, converts a vector to a
 * header pointer. Assumes that the vector is not NULL.
 * @param vec - A vector.
 * @return A header pointer.
 * @internal
 */
static inline tphvec_header_t *_tphvec_vec_to_hdr(const void *vec)
{
  tphvec_assert(vec);
  return &((tphvec_header_t *)vec)[-1];
}


static inline ptrdiff_t _tphvec_next_capacity(ptrdiff_t cap, ptrdiff_t req_cap)
{
  cap = cap ? cap : (ptrdiff_t)1;
  while (cap < req_cap) { cap = cap <= (PTRDIFF_MAX >> 1) ? cap << 1 : SIZE_MAX; }
  return cap;
}


void tphvec_free(void *vec)
{
  if (vec) {
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(vec);
    tphvec_allocator_t *a = hdr->allocator;
    if (a) {
      a->free((void *)hdr, sizeof(tphvec_header_t) + hdr->capacity, a->ctx);
    } else {
      free((void *)hdr);
    }
  }
}


tphvec_allocator_t *tphvec_get_allocator(const void *vec)
{
  return vec ? _tphvec_vec_to_hdr(vec)->allocator : (tphvec_allocator_t *)NULL;
}


int _tphvec_set_allocator(void **vec_ptr, tphvec_allocator_t *allocator)
{
  /* BITWISE AND/OR??? & */
  if (!allocator || (allocator && !(allocator->malloc && allocator->realloc && allocator->free))) {
    return TPHVEC_INVALID_ARG;
  }

  if (*vec_ptr) {
    /* Existing vector, move memory into the provided allocator. Free the memory
       allocated by the existing allocator (if any). */
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    if (allocator == hdr->allocator) { return TPHVEC_SUCCESS; }
    tphvec_header_t *new_hdr = (tphvec_header_t *)(allocator->malloc(
      sizeof(tphvec_header_t) + hdr->capacity, allocator->ctx));
    if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
    memcpy(new_hdr, hdr, sizeof(tphvec_header_t) + hdr->capacity);
    new_hdr->allocator = allocator;
    *vec_ptr = _tphvec_hdr_to_vec(new_hdr);
    if (hdr->allocator) {
      hdr->allocator->free(hdr, sizeof(tphvec_header_t) + hdr->capacity, hdr->allocator->ctx);
    } else {
      free(hdr);
    }
  } else {
    /* Must initialize a new vector to be able to store the allocator in it. */
    tphvec_header_t *hdr =
      (tphvec_header_t *)(allocator->malloc(sizeof(tphvec_header_t), allocator->ctx));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }
    hdr->capacity = 0;
    hdr->size = 0;
    hdr->allocator = allocator;
    *vec_ptr = _tphvec_hdr_to_vec(hdr);
  }
  return TPHVEC_SUCCESS;
}


int _tphvec_empty(const void *vec)
{
  /* Could be strict here and check if size in bytes is < sizeof(type)... */
  return vec ? _tphvec_vec_to_hdr(vec)->size == 0 : (int)1;
}


ptrdiff_t _tphvec_size(const void *vec, size_t sizeof_el)
{
  return vec ? _tphvec_vec_to_hdr(vec)->size / sizeof_el : (ptrdiff_t)0;
}


int _tphvec_reserve(void **vec_ptr, ptrdiff_t new_cap)
{
  if (*vec_ptr) {
    /* Existing vector. */
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    if (hdr->capacity >= new_cap) { return TPHVEC_SUCCESS; }
    tphvec_allocator_t *a = hdr->allocator;
    tphvec_header_t *new_hdr =
      (tphvec_header_t *)(a ? a->realloc(hdr,
                                sizeof(tphvec_header_t) + hdr->capacity,
                                sizeof(tphvec_header_t) + new_cap,
                                a->ctx)
                            : realloc(hdr, sizeof(tphvec_header_t) + new_cap));
    if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
    new_hdr->capacity = new_cap;
    *vec_ptr = _tphvec_hdr_to_vec(new_hdr);
  } else {
    /* Initialize new vector. */
    tphvec_header_t *hdr = (tphvec_header_t *)(malloc(sizeof(tphvec_header_t) + new_cap));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }
    hdr->capacity = new_cap;
    hdr->size = 0;
    hdr->allocator = NULL;
    *vec_ptr = _tphvec_hdr_to_vec(hdr);
  }
  return TPHVEC_SUCCESS;
}


ptrdiff_t _tphvec_capacity(const void *vec, size_t sizeof_el)
{
  return vec ? _tphvec_vec_to_hdr(vec)->capacity / sizeof_el : (ptrdiff_t)0;
}


int _tphvec_shrink_to_fit(void **vec_ptr)
{
  if (*vec_ptr) {
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    tphvec_assert(hdr->size <= hdr->capacity);
    if (hdr->size == hdr->capacity) { return TPHVEC_SUCCESS; }
    tphvec_allocator_t *a = hdr->allocator;
    tphvec_header_t *new_hdr =
      (tphvec_header_t *)(a ? a->realloc(
                                hdr, hdr->capacity, sizeof(tphvec_header_t) + hdr->size, a->ctx)
                            : realloc(hdr, sizeof(tphvec_header_t) + hdr->size));
    if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
    new_hdr->capacity = new_hdr->size;
    *vec_ptr = _tphvec_hdr_to_vec(new_hdr);
  }
  tphvec_assert(tphvec_capacity(*vec_ptr) == tphvec_size(*vec_ptr));
  return TPHVEC_SUCCESS;
}


void _tphvec_clear(void *vec)
{
  if (vec) { _tphvec_vec_to_hdr(vec)->size = 0; }
}


int _tphvec_append(void **vec_ptr, void *values, ptrdiff_t nbytes)
{
  if (*vec_ptr) {
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    const ptrdiff_t req_cap = hdr->size + nbytes;
    if (hdr->capacity < req_cap) {
      const ptrdiff_t new_cap = _tphvec_next_capacity(hdr->capacity, req_cap);
      tphvec_allocator_t *a = hdr->allocator;
      tphvec_header_t *new_hdr =
        (tphvec_header_t *)(a ? a->realloc(
                                  hdr, hdr->capacity, sizeof(tphvec_header_t) + new_cap, a->ctx)
                              : realloc(hdr, sizeof(tphvec_header_t) + new_cap));
      if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
      hdr = new_hdr;
      hdr->capacity = new_cap;
      *vec_ptr = _tphvec_hdr_to_vec(hdr);
    }
    memcpy(*vec_ptr + hdr->size, values, nbytes);
    hdr->size += nbytes;
  } else {
    tphvec_header_t *hdr = (tphvec_header_t *)(malloc(sizeof(tphvec_header_t) + nbytes));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }
    hdr->capacity = nbytes;
    hdr->size = nbytes;
    *vec_ptr = _tphvec_hdr_to_vec(hdr);
    memcpy(*vec_ptr, values, nbytes);
  }
  return TPHVEC_SUCCESS;
}


int _tphvec_resize(void **vec_ptr, ptrdiff_t count, void *value, ptrdiff_t sizeof_value)
{
  if (*vec_ptr) {
    /* Existing vector. */
    tphvec_header_t *hdr = _tphvec_vec_to_hdr(*vec_ptr);
    if (count < hdr->size) {
      /* Shrink vector, no new elements added! */
      hdr->size = count;
    } else if (count > hdr->size) {
      if (hdr->capacity < count) {
        /* Grow capacity. */
        tphvec_allocator_t *a = hdr->allocator;
        tphvec_header_t *new_hdr =
          (tphvec_header_t *)(a ? a->realloc(hdr,
                                    sizeof(tphvec_header_t) + hdr->capacity,
                                    sizeof(tphvec_header_t) + count,
                                    a->ctx)
                                : realloc(hdr, sizeof(tphvec_header_t) + count));
        if (!new_hdr) { return TPHVEC_BAD_ALLOC; }
        hdr = new_hdr;
        hdr->capacity = count;
      }
      /* Initialize the first new element of the vector by copying the
         provided value. The following elements are copied in increasingly
         large chunks in order to minimize the number of function calls. */
      memcpy(*vec_ptr + hdr->size, value, sizeof_value);
      void *src = *vec_ptr + hdr->size;
      hdr->size += sizeof_value;
      ptrdiff_t n = sizeof_value;
      while (hdr->size < count) {
        memcpy(*vec_ptr + hdr->size, src, n);
        hdr->size += n;
        n = ((hdr->size << 1) > count ? count - hdr->size : hdr->size);
        /* CHECK OVERFLOW??? */
      }
    }
    /* else: hdr->size == count, nothing to do! */
  } else {
    /* Initialize a new vector. */
    tphvec_header_t *hdr = (tphvec_header_t *)(malloc(sizeof(tphvec_header_t) + count));
    if (!hdr) { return TPHVEC_BAD_ALLOC; }
    hdr->capacity = count;
    hdr->allocator = NULL;
    *vec_ptr = _tphvec_hdr_to_vec(hdr);
    if (count < sizeof_value) {
      /* Not enough capacity to initialize an element. */
      hdr->size = 0;
    } else {
      /* Initialize the first element of the vector by copying the
         provided value. The following elements are copied in increasingly
         large chunks in order to minimize the number of calls to memcpy. */
      memcpy(*vec_ptr, value, sizeof_value);
      hdr->size = sizeof_value;
      ptrdiff_t n = sizeof_value;
      while (hdr->size < count) {
        memcpy(*vec_ptr + hdr->size, *vec_ptr, n);
        hdr->size += n;
        n = ((hdr->size << 1) > count ? count - hdr->size : hdr->size);
        /* CHECK OVERFLOW??? */
      }
    }
  }
  return TPHVEC_SUCCESS;
}

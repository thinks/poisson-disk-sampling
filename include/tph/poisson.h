#ifndef TPH_POISSON_H
#define TPH_POISSON_H

#define TPH_MAJOR_VERSION 0
#define TPH_MINOR_VERSION 4
#define TPH_PATCH_VERSION 0

#include <stddef.h>// size_t, NULL
#include <stdint.h>// uint32_t, etc

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TPH_REAL_TYPE
#define TPH_REAL_TYPE float
#endif

typedef TPH_REAL_TYPE tph_real;

// clang-format off
typedef struct tph_poisson_points_           tph_poisson_points;
typedef struct tph_poisson_args_             tph_poisson_args;
typedef struct tph_poisson_sampling_         tph_poisson_sampling;
typedef struct tph_poisson_context_internal_ tph_poisson_context_internal;
// clang-format on

#pragma pack(push, 1)

struct tph_poisson_points_
{
  const tph_real *pos = NULL;
};

struct tph_poisson_args_
{
  tph_real radius = 0.F;
  uint32_t dims = 0;
  struct
  {
    tph_real *min = NULL;
    tph_real *max = NULL;
  } bbox;
  uint32_t max_sample_attempts = 30;
  uint32_t seed = 0;
};

struct tph_poisson_sampling_
{
  tph_poisson_context_internal *internal = NULL;
  uint32_t dims = 0;
  uint32_t numpoints = 0;
};

#pragma pack(pop)

/**
 * Uses malloc.
 */
extern int32_t tph_poisson_generate(const tph_poisson_args *args,
  tph_poisson_sampling *sampling);

typedef void *(*FTPHAllocFn)(void *userctx, size_t size);
typedef void (*FTPHFreeFn)(void *userctx, void *p);

extern int32_t tph_poisson_generate_useralloc(const tph_poisson_args *args,
  void *userallocctx,
  FTPHAllocFn allocfn,
  FTPHFreeFn freefn,
  tph_poisson_sampling *sampling);

extern void tph_poisson_free(tph_poisson_sampling *sampling);

extern const tph_poisson_points *tph_poisson_get_points(
  const tph_poisson_sampling *sampling);

#ifdef __cplusplus
}// extern "C"
#endif

#endif// TPH_POISSON_DISK_SAMPLING_H

#ifdef TPH_POISSON_DISK_SAMPLING_IMPLEMENTATION
#undef TPH_POISSON_DISK_SAMPLING_IMPLEMENTATION

#include <stdlib.h>// malloc, free

// Structs

#pragma pack(push, 1)

struct tph_poisson_context_internal_
{
  void *mem;
  jcv_edge *edges;
  jcv_halfedge *beachline_start;
  jcv_halfedge *beachline_end;
  jcv_halfedge *last_inserted;
  jcv_priorityqueue *eventqueue;

  jcv_site *sites;
  jcv_site *bottomsite;
  int numsites;
  int currentsite;
  int _padding;

  jcv_memoryblock *memblocks;
  jcv_edge *edgepool;
  jcv_halfedge *halfedgepool;
  void **eventmem;
  jcv_clipper clipper;

  void *memctx;// Given by the user
  FTPHAllocFn alloc;
  FTPHFreeFn free;

  jcv_rect rect;
};

#pragma pack(pop)

void tph_poisson_free(tph_poisson_sampling *s) 
{
  tph_poisson_context_internal *internal = s->internal;
  FTPHFreeFn freefn = internal->free;
  void *memctx = internal->memctx;
  freefn(memctx, internal->mem);

#if 0
  FJCVFreeFn freefn = internal->free;
  while (internal->memblocks) {
    jcv_memoryblock *p = internal->memblocks;
    internal->memblocks = internal->memblocks->next;
    freefn(memctx, p);
  }

  freefn(memctx, internal->mem);
#endif
}

static void *tph_alloc_fn(void *memctx, size_t size)
{
  (void)memctx;
  return malloc(size);
}

static void tph_free_fn(void *memctx, void *p)
{
  (void)memctx;
  free(p);
}

void tph_poisson_disk_sampling_generate(const tph_args *args, tph_sampling *s)
{
  tph_poisson_disk_sampling_generate_useralloc(args, NULL, tph_alloc_fn, tph_free_fn, s);
}

void tph_diagram_generate_useralloc(const tph_args *args,
  void *userallocctx,
  FTPHAllocFn allocfn,
  FTPHFreeFn freefn,
  tph_sampling *s)
{
  if (s->internal != NULL) { tph_poisson_free(s); }

  jcv_context_internal *internal = jcv_alloc_internal(num_points, userallocctx, allocfn, freefn);

  internal->beachline_start = jcv_halfedge_new(internal, 0, 0);
  internal->beachline_end = jcv_halfedge_new(internal, 0, 0);

  internal->beachline_start->left = 0;
  internal->beachline_start->right = internal->beachline_end;
  internal->beachline_end->left = internal->beachline_start;
  internal->beachline_end->right = 0;

  internal->last_inserted = 0;

  int max_num_events = num_points * 2;// beachline can have max 2*n-5 parabolas
  jcv_pq_create(internal->eventqueue, max_num_events, (void **)internal->eventmem);

  internal->numsites = num_points;
  jcv_site *sites = internal->sites;

  for (int i = 0; i < num_points; ++i) {
    sites[i].p = points[i];
    sites[i].edges = 0;
    sites[i].index = i;
  }

  qsort(sites, (size_t)num_points, sizeof(jcv_site), jcv_point_cmp);

  jcv_clipper box_clipper;
  if (clipper == 0) {
    box_clipper.test_fn = jcv_boxshape_test;
    box_clipper.clip_fn = jcv_boxshape_clip;
    box_clipper.fill_fn = jcv_boxshape_fillgaps;
    clipper = &box_clipper;
  }
  internal->clipper = *clipper;

  jcv_rect tmp_rect;
  tmp_rect.min.x = tmp_rect.min.y = JCV_FLT_MAX;
  tmp_rect.max.x = tmp_rect.max.y = -JCV_FLT_MAX;
  jcv_prune_duplicates(internal, &tmp_rect);

  // Prune using the test second
  if (internal->clipper.test_fn) {
    // e.g. used by the box clipper in the test_fn
    internal->clipper.min = rect ? rect->min : tmp_rect.min;
    internal->clipper.max = rect ? rect->max : tmp_rect.max;

    jcv_prune_not_in_shape(internal, &tmp_rect);

    // The pruning might have made the bounding box smaller
    if (!rect) {
      // In the case of all sites being all on a horizontal or vertical line, the
      // rect area will be zero, and the diagram generation will most likely fail
      jcv_rect_round(&tmp_rect);
      jcv_rect_inflate(&tmp_rect, 10);

      internal->clipper.min = tmp_rect.min;
      internal->clipper.max = tmp_rect.max;
    }
  }

  internal->rect = rect ? *rect : tmp_rect;

  d->min = internal->rect.min;
  d->max = internal->rect.max;
  d->numsites = internal->numsites;
  d->internal = internal;

  internal->bottomsite = jcv_nextsite(internal);

  jcv_priorityqueue *pq = internal->eventqueue;
  jcv_site *site = jcv_nextsite(internal);

  int finished = 0;
  while (!finished) {
    jcv_point lowest_pq_point;
    if (!jcv_pq_empty(pq)) {
      jcv_halfedge *he = (jcv_halfedge *)jcv_pq_top(pq);
      lowest_pq_point.x = he->vertex.x;
      lowest_pq_point.y = he->y;
    }

    if (site != 0 && (jcv_pq_empty(pq) || jcv_point_less(&site->p, &lowest_pq_point))) {
      jcv_site_event(internal, site);
      site = jcv_nextsite(internal);
    } else if (!jcv_pq_empty(pq)) {
      jcv_circle_event(internal);
    } else {
      finished = 1;
    }
  }

  for (jcv_halfedge *he = internal->beachline_start->right; he != internal->beachline_end;
       he = he->right) {
    jcv_finishline(internal, he->edge);
  }

  jcv_fillgaps(d);
}


#endif// TPH_POISSON_DISK_SAMPLING_IMPLEMENTATION
//
// gc-bench.c
// benchmarking creation and copying of a b-tree
// (see core/gc.c for the lightweight garbage collector,
// based on ideas from Qish by Basile Starynkevitch.)
//
// (c) 2008 why the lucky stiff, the freelance professor
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include "potion.h"
#include "internal.h"
#include "gc.h"
#include "CuTest.h"

Potion *P;

static const int tree_stretch = 20;
static const int tree_long_lived = 18;
static const int array_size = 2000000;
static const int min_tree = 4;
static const int max_tree = 20;

unsigned
current_time(void)
{
  struct timeval t;
  struct timezone tz;

  if (gettimeofday (&t, &tz) == -1)
    return 0;
  return (t.tv_sec * 1000 + t.tv_usec / 1000);
}

//
// TODO: use a b-tree class rather than tuples
//
PN gc_make_tree(int depth) {
  PN l, r, x;
  if (depth <= 0)
    return potion_tuple_with_size(P, 2);

  l = gc_make_tree(depth - 1);
  r = gc_make_tree(depth - 1);
  x = potion_tuple_with_size(P, 2);
  PN_TUPLE_AT(x, 0) = l;
  PN_TUPLE_AT(x, 1) = r;
  return x;
}

PN gc_populate_tree(PN node, int depth) {
  if (depth <= 0)
    return;

  depth--;
  PN_TUPLE_AT(node, 0) = potion_tuple_with_size(P, 2);
  potion_gc_update(P->mem, node);
#ifdef HOLES
  n = potion_tuple_with_size(P, 2);
#endif
  PN_TUPLE_AT(node, 1) = potion_tuple_with_size(P, 2);
  potion_gc_update(P->mem, node);
#ifdef HOLES
  n = potion_tuple_with_size(P, 2);
#endif
  gc_populate_tree(PN_TUPLE_AT(node, 0), depth);
  gc_populate_tree(PN_TUPLE_AT(node, 1), depth);
}

int gc_tree_depth(PN node, int side, int depth) {
  PN n = PN_TUPLE_AT(node, side);
  if (n == PN_NIL) return depth;
  return gc_tree_depth(n, side, depth + 1);
}

int tree_size(int i) {
  return ((1 << (i + 1)) - 1);
}

int main(void) {
  POTION_INIT_STACK(sp);
  PN temp, long_lived, ary;
  int i, j, count;

  P = potion_create(sp);

  printf("Stretching memory with a binary tree of depth %d\n",
    tree_stretch);
  temp = gc_make_tree(tree_stretch);
  temp = 0;

  printf("Creating a long-lived binary tree of depth %d\n",
    tree_long_lived);
  long_lived = potion_tuple_with_size(P, 2);
  gc_populate_tree(long_lived, tree_long_lived);

  printf("Creating a long-lived array of %d doubles\n",
    array_size);
  ary = potion_tuple_with_size(P, array_size);
  for (i = 0; i < array_size / 2; ++i)
    PN_TUPLE_AT(ary, i) = PN_NUM(1.0 / i);

  for (i = min_tree; i <= max_tree; i += 2) {
    long start, finish;
    int iter = 2 * tree_size(tree_stretch) / tree_size(i);
    printf ("Creating %d trees of depth %d\n", iter, i);

    start = current_time();
    for (j = 0; j < iter; ++j) {
      temp = potion_tuple_with_size(P, 2);
      gc_populate_tree(temp, i);
    }
    finish = current_time();
    printf("\tTop down construction took %d msec\n",
      finish - start);

    start = current_time();
    for (j = 0; j < iter; ++j) {
      temp = gc_make_tree(i);
      temp = 0;
    }
    finish = current_time();
    printf("\tBottom up construction took %d msec\n",
      finish - start);
  }
  
  if (long_lived == 0 || PN_TUPLE_AT(ary, 1000) != PN_NUM(1.0 / 1000))
    printf("Wait, problem.\n");

  printf ("Total %d minor and %d full garbage collections\n"
	  "   (min.birth.size=%dK, max.size=%dK, gc.thresh=%dK)\n",
	  P->mem->minors, P->mem->majors,
	  POTION_BIRTH_SIZE >> 10, POTION_MAX_BIRTH_SIZE >> 10,
	  POTION_GC_THRESHOLD >> 10);
  return 0;
}

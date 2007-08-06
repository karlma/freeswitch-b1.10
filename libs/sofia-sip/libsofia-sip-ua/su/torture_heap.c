/*
 * This file is part of the Sofia-SIP package
 *
 * Copyright (C) 2007 Nokia Corporation.
 *
 * Contact: Pekka Pessi <pekka.pessi@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/**
 * @file torture_heap.c
 * @brief Test heap
 *
 * @author Pekka Pessi <Pekka.Pessi@nokia.com>
 */

#include "config.h"

#include <sofia-sip/heap.h>

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct {
  unsigned key, value;
  size_t index;
} type1, *type2;

static type1 const null = { 0, 0, 0 };

static inline
int less1(type1 a, type1 b)
{
  return a.key < b.key;
}

static inline
void set1(type1 *heap, size_t index, type1 e)
{
  assert(index > 0);
  e.index = index;
  heap[index] = e;
}

#define alloc(a, o, size) realloc((o), (size))

static inline
int less2(type2 a, type2 b)
{
  return a->key < b->key;
}

static inline
void set2(type2 *heap, size_t index, type2 e)
{
  assert(index > 0);
  e->index = index;
  heap[index] = e;
}

#define scope static

/* Define heap having structs as its elements */

typedef HEAP_TYPE Heap1;

HEAP_DECLARE(static, Heap1, heap1_, type1);
HEAP_BODIES(static, Heap1, heap1_, type1, less1, set1, alloc, null);

/* Define heap having pointers as its elements */

typedef HEAP_TYPE Heap2;

HEAP_DECLARE(static, Heap2, heap2_, type1 *);
HEAP_BODIES(static, Heap2, heap2_, type1 *, less2, set2, alloc, NULL);

/* ====================================================================== */

int tstflags;

#define TSTFLAGS tstflags

#include <sofia-sip/tstdef.h>

char name[] = "torture_heap";

int test_value()
{
  BEGIN();

  Heap1 heap = { NULL };
  unsigned i, previous, n, N;
  unsigned char *tests;

  N = 300000;

  TEST_1(tests = calloc(sizeof (unsigned char), N + 1));

  TEST(heap1_resize(NULL, &heap, 0), 0);

  /* Add N entries in reverse order */
  for (i = N; i > 0; i--) {
    type1 e = { i / 10, i, 0 };
    if (heap1_is_full(heap))
      TEST(heap1_resize(NULL, &heap, 0), 0);
    TEST(heap1_is_full(heap), 0);
    TEST(heap1_add(heap, e), 0);
    tests[i] |= 1;
  }

  TEST(heap1_used(heap), N);

  for (i = 1; i <= N; i++) {
    type1 const e = heap1_get(heap, i);

    TEST(e.index, i);
    TEST(tests[e.value] & 2, 0);
    tests[e.value] |= 2;

    if (2 * i <= N) {
      type1 const left = heap1_get(heap, 2 * i);
      TEST_1(e.key <= left.key);
    }

    if (2 * i + 1 <= N) {
      type1 const right = heap1_get(heap, 2 * i + 1);
      TEST_1(e.key <= right.key);
    }
  }

  /* Remove N entries */
  previous = 0;

  for (n = 0; heap1_used(heap) > 0; n++) {
    type1 const e = heap1_get(heap, 1);
    TEST_1(previous <= e.key);

    TEST(tests[e.value] & 4, 0);
    tests[e.value] |= 4;

    previous = e.key;
    TEST(heap1_remove(heap, 1).index, 1);
  }
  TEST(n, N);

  /* Add N entries in reverse order */
  for (i = N; i > 0; i--) {
    type1 e = { i / 10, i, 0 };
    if (heap1_is_full(heap))
      TEST(heap1_resize(NULL, &heap, 0), 0);
    TEST(heap1_is_full(heap), 0);
    TEST(heap1_add(heap, e), 0);
  }

  TEST(heap1_used(heap), N);

  /* Remove 1000 entries from random places */
  previous = 0;

  for (i = 0; i < 1000 && heap1_used(heap) > 0; i++) {
    type1 e;
    n = i * 397651 % heap1_used(heap) + 1;
    e = heap1_get(heap, n);
    TEST(e.index, n);
    TEST(tests[e.value] & 8, 0); tests[e.value] |= 8;
    TEST(heap1_remove(heap, n).index, n);
  }

  for (i = 1; i <= heap1_used(heap); i++) {
    type1 e = heap1_get(heap, i);
    type1 left = heap1_get(heap, 2 * i);
    type1 right = heap1_get(heap, 2 * i + 1);
    TEST_1(left.index == 0 || e.key <= left.key);
    TEST_1(right.index == 0 || e.key <= right.key);
  }

  /* Remove rest */
  for (n = 0, previous = 0; heap1_used(heap) > 0; n++) {
    type1 e = heap1_get(heap, 1);
    TEST(e.index, 1);
    TEST(tests[e.value] & 8, 0);
    tests[e.value] |= 8;
    TEST_1(previous <= e.key);
    previous = e.key;
    TEST(heap1_remove(heap, 1).index, 1);
  }

  for (i = 1; i <= N; i++) {
    TEST(tests[i], 8 | 4 | 2 | 1);
  }

  TEST(heap1_resize(NULL, &heap, 63), 0);
  TEST(heap1_size(heap), 63);

  TEST(heap1_free(NULL, &heap), 0);
  free(tests);

  END();
}

int test_ref()
{
  BEGIN();

  Heap2 heap = { NULL };
  unsigned i, previous, n, N;
  unsigned char *tests;
  type1 *items;

  N = 300000;

  TEST_1(tests = calloc(sizeof (unsigned char), N + 1));
  TEST_1(items = calloc((sizeof *items), N + 1));

  TEST(heap2_resize(NULL, &heap, 0), 0);

  /* Add N entries in reverse order */
  for (i = N; i > 0; i--) {
    type2 e = items + i;

    e->key = i / 10, e->value = i;

    if (heap2_is_full(heap))
      TEST(heap2_resize(NULL, &heap, 0), 0);
    TEST(heap2_is_full(heap), 0);
    TEST(heap2_add(heap, e), 0);
    tests[i] |= 1;
  }

  TEST(heap2_used(heap), N);

  for (i = 1; i <= N; i++) {
    type2 const e = heap2_get(heap, i);

    TEST_1(e);
    TEST(e->index, i);
    TEST(tests[e->value] & 2, 0);
    tests[e->value] |= 2;

    if (2 * i <= N) {
      type2 const left = heap2_get(heap, 2 * i);
      TEST_1(e->key <= left->key);
    }

    if (2 * i + 1 <= N) {
      type2 const right = heap2_get(heap, 2 * i + 1);
      TEST_1(e->key <= right->key);
    }
  }

  /* Remove N entries */
  previous = 0;

  for (n = 0; heap2_used(heap) > 0; n++) {
    type2 const e = heap2_get(heap, 1);

    TEST_1(e);
    TEST_1(previous <= e->key);

    TEST(tests[e->value] & 4, 0);
    tests[e->value] |= 4;

    previous = e->key;
    TEST(heap2_remove(heap, 1), e);
  }
  TEST(n, N);

  /* Add N entries in reverse order */
  for (i = N; i > 0; i--) {
    type2 e = items + i;

    if (heap2_is_full(heap))
      TEST(heap2_resize(NULL, &heap, 0), 0);
    TEST(heap2_is_full(heap), 0);
    TEST(heap2_add(heap, e), 0);
  }

  TEST(heap2_used(heap), N);

  /* Remove 1000 entries from random places */
  previous = 0;

  for (i = 0; i < 1000 && heap2_used(heap) > 0; i++) {
    type2 e;
    n = i * 397651 % heap2_used(heap) + 1;
    e = heap2_get(heap, n);
    TEST_1(e); TEST(e->index, n);
    TEST(tests[e->value] & 8, 0); tests[e->value] |= 8;
    TEST(heap2_remove(heap, n), e);
  }

  for (i = 1; i <= heap2_used(heap); i++) {
    type2 e = heap2_get(heap, i);

    TEST_1(e); TEST(e->index, i);

    if (2 * i <= heap2_used(heap)) {
      type2 const left = heap2_get(heap, 2 * i);
      TEST_1(left);
      TEST_1(e->key <= left->key);
    }

    if (2 * i + 1 <= heap2_used(heap)) {
      type2 const right = heap2_get(heap, 2 * i + 1);
      TEST_1(right);
      TEST_1(e->key <= right->key);
    }
  }

  /* Remove rest */
  for (n = 0, previous = 0; heap2_used(heap) > 0; n++) {
    type2 e = heap2_remove(heap, 1);
    TEST_1(e); TEST(e->index, 1);
    TEST(tests[e->value] & 8, 0);
    tests[e->value] |= 8;
    TEST_1(previous <= e->key);
    previous = e->key;
  }

  for (i = 1; i <= N; i++) {
    TEST(tests[i], 8 | 4 | 2 | 1);
  }

  TEST(heap2_resize(NULL, &heap, 63), 0);
  TEST(heap2_size(heap), 63);

  TEST(heap2_free(NULL, &heap), 0);
  free(tests);
  free(items);

  END();
}

void usage(int exitcode)
{
  fprintf(stderr,
	  "usage: %s [-v] [-a]\n",
	  name);
  exit(exitcode);
}

int main(int argc, char *argv[])
{
  int retval = 0;
  int i;

  for (i = 1; argv[i]; i++) {
    if (strcmp(argv[i], "-v") == 0)
      tstflags |= tst_verbatim;
    else if (strcmp(argv[i], "-a") == 0)
      tstflags |= tst_abort;
    else
      usage(1);
  }

  retval |= test_value(); fflush(stdout);
  retval |= test_ref(); fflush(stdout);

  return retval;
}

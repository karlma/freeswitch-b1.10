/*
 * replay_driver.c
 *
 * A driver for the replay_database implementation
 *
 * David A. McGrew
 * Cisco Systems, Inc.
 */

/*
 *	
 * Copyright (c) 2001-2005, Cisco Systems, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * 
 *   Neither the name of the Cisco Systems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>

#include "rdb.h"
#include "ut_sim.h"

void
test_rdb_db(void);

double
rdb_check_adds_per_second(void);

int
main (void) {

  printf("testing anti-replay database (rdb_t)...\n");
  test_rdb_db();
  printf("done\n");

  printf("rdb_check/rdb_adds per second: %e\n",
	 rdb_check_adds_per_second());
  
  return 0;
}


void
print_rdb(rdb_t *rdb) {
  printf("rdb: {%u, %s}\n", rdb->window_start, v128_bit_string(&rdb->bitmask));
}

void
rdb_check_add(rdb_t *rdb, uint32_t idx) {

  if (rdb_check(rdb, idx) != err_status_ok) {
    printf("rdb_check failed at index %u\n", idx);
    return;
  }
  if (rdb_add_index(rdb, idx) != err_status_ok)
    printf("rdb_add_index failed at index %u\n", idx);

}

void
rdb_check_expect_failure(rdb_t *rdb, uint32_t idx) {

  if (rdb_check(rdb, idx) != err_status_fail)
    printf("rdb_check failed at index %u (false positive)\n", idx);
}

void
rdb_check_unordered(rdb_t *rdb, uint32_t idx) {
  err_status_t rstat;

  rstat = rdb_check(rdb, idx);
  if ((rstat != err_status_ok) && (rstat != err_status_replay_old))
    printf("rdb_check_unordered failed at index %u\n", idx);
}



#define MAX_IDX 160

void
test_rdb_db() {
  rdb_t rdb;
  uint32_t idx, ircvd;
  ut_connection utc;
  
  if (rdb_init(&rdb) != err_status_ok) {
    printf("rdb_init failed\n");
    exit(1);
  }

  /* test sequential insertion */
  for (idx=0; idx < MAX_IDX; idx++) {
    rdb_check_add(&rdb, idx);
  }

  /* test for false positives */
  for (idx=0; idx < MAX_IDX; idx++) {
    rdb_check_expect_failure(&rdb, idx);
  }

  /* re-initialize */
  if (rdb_init(&rdb) != err_status_ok) {
    printf("rdb_init failed\n");
    exit(1);
  }

  /* test non-sequential insertion */
  ut_init(&utc);
  
  for (idx=0; idx < MAX_IDX; idx++) {
    ircvd = ut_next_index(&utc);
    rdb_check_unordered(&rdb, ircvd);
  }
  
}

#include <time.h>       /* for clock()  */
#include <stdlib.h>     /* for random() */

#define REPLAY_NUM_TRIALS 10000000

double
rdb_check_adds_per_second(void) {
  uint32_t i;
  rdb_t rdb;
  clock_t timer;
  int failures;                    /* count number of failures        */
  
  if (rdb_init(&rdb) != err_status_ok) {
    printf("rdb_init failed\n");
    exit(1);
  }  

  timer = clock();
  for(i=0; i < REPLAY_NUM_TRIALS; i+=3) {
    if (rdb_check(&rdb, i+2) != err_status_ok)
      ++failures;
    if (rdb_add_index(&rdb, i+2) != err_status_ok)
      ++failures;
    if (rdb_check(&rdb, i+1) != err_status_ok)
      ++failures;
    if (rdb_add_index(&rdb, i+1) != err_status_ok)
      ++failures;
    if (rdb_check(&rdb, i) != err_status_ok)
      ++failures;
    if (rdb_add_index(&rdb, i) != err_status_ok)
      ++failures;
  }
  timer = clock() - timer;

  return (double) CLOCKS_PER_SEC * REPLAY_NUM_TRIALS / timer;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/common.h"
#include "common/sem_wrapper.h"
#include "common/shm_wrapper.h"
#include "common/utils.h"

/**
 * @file worker_std.c
 * @brief Standard Worker Process (Producer) - Generates Packages A, B, or C.
 *
 * This file implements the logic for a Standard Worker process.
 * The worker acts as a **Producer** in the system, generating packages of a specific
 * type (A, B, or C) and attempting to place them on the conveyor belt (Shared Memory).
 *
 * Key Responsibilities:
 * - Continuously generating packages with randomized weights within defined bounds.
 * - waiting for available slots on the belt (@ref SEM_EMPTY).
 * - Enforcing the Maximum Belt Weight limit (M) before placing an item.
 * - Updating the Circular Buffer (Push operation).
 *
 * @author Mikołaj Kosiorek
 */
int main(int argc, char *argv[]) {
  // Turn off buffering for real time logging to simulation.log file
  setbuf(stdout, NULL);
  
  if(argc < 2) {
    fprintf(stderr, "Usage: %s <Type A/B/C>\n", argv[0]);
    exit(1);
  }

  // Determine package type
  PackageType type;
  if (strcmp(argv[1], "A") == 0) type = PKG_A;
  else if (strcmp(argv[1], "B") == 0) type = PKG_B;
  else if (strcmp(argv[1], "C") == 0) type = PKG_C;
  else {
    fprintf(stderr, "Usage: %s <Type A/B/C>\n", argv[0]);
    exit(1);
  }

  // Shared memory attachment
  SharedState *shm;

  shm = (SharedState *)attach_memory_block(
    KEY_PATH,
    KEY_ID_SHM,
    sizeof(SharedState)
  );

  // Gets access to semaphores
  int semid = get_sem(KEY_PATH, KEY_ID_SEM, 0);

  int allow_full_belt_msg = 1;
  int worker_id = (type==PKG_A ? 1 : (type==PKG_B ? 2 : 3));
  char time_buf[64];
  srand(time(NULL) ^ getpid()); // Seed random
  
  while(1) {
    if (shm->shutdown) break;

    // Creating package data
    double w = generate_weight(type);
    double v = get_volume(type);

    // Wating for space on belt
    SEM_P(semid, SEM_EMPTY);

    // Critical section
    SEM_P(semid, SEM_MUTEX);

    if(shm->shutdown) {
      SEM_V(semid, SEM_MUTEX);
      break;
    }

    // Checking weight limit
    if (shm->current_belt_weight + w > shm->max_belt_weight_M) {
      // Cannot place item, releasing resources
      SEM_V(semid, SEM_MUTEX);
      SEM_V(semid, SEM_EMPTY);

      // Print  only at first occurrance
      if (allow_full_belt_msg) {
	get_time(time_buf, sizeof(time_buf));
	printf("[" COLOR_YELLOW "%s" COLOR_RESET "]" COLOR_BLUE " P%d  " COLOR_RESET "Worker P%d: pkg %s (%.2f kg) Load: %.2f/%.2f. Limit reached...\n",
	       time_buf, worker_id, worker_id, argv[1], w,
	       shm->current_belt_weight, shm->max_belt_weight_M
	);
	
	allow_full_belt_msg = 0;
      }

      // Waits few 100ms to avoid busy loop slamming
      usleep(100000);

      // Take different package
      continue;
    }
    allow_full_belt_msg = 1; // Allow printing full belt message after successfuly placing next package

    // Placing package on belt
    int idx = shm->tail;
    shm->belt[idx].type = type;
    shm->belt[idx].weight = w;
    shm->belt[idx].volume = v;
    shm->belt[idx].id = rand() % 10000;

    shm->tail = (shm->tail + 1) % shm->max_items_K;
    shm->current_count++;
    shm->current_belt_weight += w;

    get_time(time_buf, sizeof(time_buf));
    printf("[" COLOR_GREEN "%s" COLOR_RESET "]" COLOR_BLUE " P%d  " COLOR_RESET "Worker P%d: Placed pkg %s (%.2f kg) on belt. Load: %.2f/%.2f\n", 
	   time_buf, worker_id, worker_id, argv[1], w, 
	   shm->current_belt_weight, shm->max_belt_weight_M);

    // Unlock access
    SEM_V(semid, SEM_MUTEX);
    SEM_V(semid, SEM_FULL);

    // Simulates work time
    usleep(rand() % 500000 + 200000);

#ifdef SIM_DELAY_MS
    usleep(SIM_DELAY_MS * 1000);
#endif
  }

  detach_memory_block(shm);

  return 0;
}

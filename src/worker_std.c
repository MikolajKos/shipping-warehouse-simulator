#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common/common.h"
#include "common/sem_wrapper.h"
#include "common/shm_wrapper.h"
#include "common/utils.h"

int main(int argc, char *argv[]) {
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

      // Waits few 100ms to avoid busy loop slamming
      usleep(100000);
      continue;
    }

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
    printf("[%s] Worker P%d: Placed pkg %s (%.2f kg) on belt. Load: %.2f/%.2f\n", 
               time_buf, (type==PKG_A ? 1 : (type==PKG_B ? 2 : 3)), argv[1], w, 
               shm->current_belt_weight, shm->max_belt_weight_M);

    // Unlock access
    SEM_V(semid, SEM_MUTEX);
    SEM_V(semid, SEM_FULL);

    // Simulates work time
    usleep(rand() % 500000 + 200000);
  }

  detach_memory_block(shm);

  return 0;
}

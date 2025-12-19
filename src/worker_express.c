#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "common/common.h"
#include "common/sem_wrapper.h"
#include "common/shm_wrapper.h"
#include "common/utils.h"

volatile sig_atomic_t load_signal = 0;

void handle_sigusr1(int sig) {
  (void)sig; // Satisfies compiler
  load_signal = 1;
}

void load_express_packages(SharedState *shm, int count) {
  char time_buf[64];
  get_time(time_buf, sizeof(time_buf));

  printf("[" COLOR_GREEN "%s" COLOR_RESET "]" COLOR_MAGENTA " P4 (Express)  " COLOR_RESET "Attempting to load %d packages...\n", time_buf, count);

  for (int i = 0; i < count; ++i) {
    PackageType type = get_rand_package_type();

    double w = generate_weight(type);
    double v = get_volume(type);

    if(shm->current_truck_load + w <= shm->truck_capacity_W &&
       shm->current_truck_vol + v <= shm->truck_volume_V) {

      // Loading single package
      shm->current_truck_load += w;
      shm->current_truck_vol += v;

      printf("   -> ["COLOR_GREEN"+"COLOR_RESET"] Loaded pkg %d/%d: %.2f kg (Load: %.2f/%.2f)\n",
	     i+1, count, w, shm->current_truck_load, shm->truck_capacity_W);
    } else { // Limit reached
      printf("   -> ["COLOR_YELLOW"-"COLOR_RESET"] Skipped pkg %d/%d (Truck full or limit reached)\n", i+1, count);
    }
  }
}

int main() {
  // Register signal handler
  struct sigaction sa;
  sa.sa_handler = handle_sigusr1;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);

  // IPC Setup
  SharedState *shm;
  shm = attach_memory_block(
    KEY_PATH,
    KEY_ID_SHM,
    sizeof(SharedState)
  );

  int semid = get_sem(KEY_PATH, KEY_ID_SEM, 0);

  srand(time(NULL) ^ getpid());
  char time_buf[64];

  while(1) {
    if (!load_signal) {
      if (shm->shutdown) break;

      // Waits for signal
      pause();
    }

    if(shm->shutdown) break;

    if (load_signal) {
      get_time(time_buf, sizeof(time_buf));
      printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_MAGENTA" P4 (Express)  "COLOR_RESET"Received signal. Attempting to load packet.\n", time_buf);

      //Critical Part
      SEM_P(semid, SEM_MUTEX);
      
      if (!shm->truck_docked) {
	printf("["COLOR_YELLOW"%s"COLOR_RESET"]"COLOR_MAGENTA" P4 (Express)  "COLOR_RESET"No truck at dock. Cannot load.\n", time_buf);
      } else {
	// Generate a batch of express packages. For example 1-5
	int count = (rand() % 5) + 1;
	load_express_packages(shm, count);
      }
      load_signal = 0;
      SEM_V(semid, SEM_MUTEX);
    }
  }

  detach_memory_block(shm);

  return 0;
}

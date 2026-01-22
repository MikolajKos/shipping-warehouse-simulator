/**
 * @file worker_express.c
 * @brief Express Worker Process (P4) - Event-Driven Priority Loader.
 *
 * This file implements the logic for the Express Worker (P4).
 * Unlike standard workers that continuously produce items for the belt, the Express Worker
 * is **event-driven**. It sleeps until it receives a signal (`SIGUSR1`) from the Dispatcher.
 *
 * Key Features:
 * - **Signal Handling:** Uses `pause()` to suspend execution until triggered.
 * - **Direct Loading:** Bypasses the conveyor belt and loads packages directly onto the Truck.
 * - **Priority Logic:** Executed on demand to simulate high-priority shipments.
 *
 * @author Mikołaj Kosiorek
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/common.h"
#include "common/sem_wrapper.h"
#include "common/shm_wrapper.h"
#include "common/utils.h"

/**
 * @brief Atomic flag for signal handling.
 *
 * Defined as `volatile sig_atomic_t` to ensure safe access between the asynchronous
 * signal handler and the main program loop without race conditions.
 */
volatile sig_atomic_t load_signal = 0;

/**
 * @brief Signal Handler for SIGUSR1.
 *
 * Sets the global flag `load_signal` to 1, waking up the main loop.
 *
 * @param sig The signal number (expected SIGUSR1).
 */
void handle_sigusr1(int sig) {
  (void)sig; // Satisfies compiler
  load_signal = 1;
}

/**
 * @brief Simulates loading a batch of express packages directly into the truck.
 *
 * Iterates `count` times, generating random packages. Unlike standard workers,
 * this function checks the **Truck's remaining capacity** directly and updates
 * the load, bypassing the conveyor belt buffer.
 *
 * @param shm   Pointer to the shared memory state.
 * @param count Number of packages to attempt to load in this batch.
 */
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

/**
 * @brief Main Entry Point for Express Worker.
 *
 * **Flow of Execution:**
 * 1. Disables stdout buffering for real-time logging.
 * 2. Registers `handle_sigusr1` for `SIGUSR1`.
 * 3. Attaches to Shared Memory and Semaphores.
 * 4. Enters the Event Loop:
 * - Calls `pause()` to sleep and wait for signals (saves CPU).
 * - **On Wake Up:** Checks if `load_signal` is set.
 * - **Critical Section:** Locks `SEM_MUTEX`.
 * - Checks if a truck is present (`truck_docked`).
 * - Calls `load_express_packages()` to load a random batch (1-5 items).
 * - Unlocks `SEM_MUTEX`.
 * - Resets `load_signal` and goes back to sleep.
 *
 * @return 0 on clean exit.
 */
int main() {
  // Turn off buffering for real time logging to simulation.log file
  setbuf(stdout, NULL);

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

#ifdef SIM_DELAY_MS
    usleep(SIM_DELAY_MS * 1000);
#endif
  }

  detach_memory_block(shm);

  return 0;
}

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
  (void)sig;
  load_signal = 1;
}

int main() {
  // Signal Setup
  struct sigaction sa;
  sa.sa_handler = handle_sigusr1;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);

  // IPC Setup
  SharedState *shm;
  attach_memory_block(
    KEY_PATH,
    KEY_ID_SHM,
    sizeof(SharedState)
  );

  int semid = sem_get(KEY_PATH, KEY_ID_SEM, 0);

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
      printf("[%s] P4 (Express): Received signal. Attempting to load packet.\n", time_buf);

      //Critical Part
      SEM_P(semid, SEM_MUTEX);
      if (!shm->truck_docked) {
	printf("[%s] P4 (Express): No truck at dock. Cannot load.\n", time_buf);
      } else {
	// Generate a batch of express packages. For example 3
	int count = 3;

	printf("[%s] P4 (Express): Loading %d express packages directly...\n", time_buf, count);

	for (int i = 0; i < count; ++i) {
	  
	}
      }
  }

  detach_memory_block(shm);

  return 0;
}

#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "common/common.h"
#include "common/sem_wrapper.h"
#include "common/shm_wrapper.h"
#include "common/utils.h"

volatile sig_atomic_t force_departure = 0;

void handle_sigusr1(int sig) {
  (void)sig; // Satisfies compiler
  force_departure = 1;
}

int main(int argc, char *argv[]) {
  // Turn off buffering for real time logging to simulation.log file
  setbuf(stdout, NULL);

  // Validate Arguments
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <ID>\n", argv[0]);
    exit(1);
  }

  // Register signal handler
  struct sigaction sa;
  sa.sa_handler = handle_sigusr1;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);

  // Shared Memory Attachment
  SharedState *shm;

  shm = attach_memory_block(
    KEY_PATH,
    KEY_ID_SHM,
    sizeof(SharedState)
  );

  // Gets Access to Semaphores
  int semid;

  semid = get_sem(KEY_PATH, KEY_ID_SEM, 0);

  // Assign truck id
  int truck_id = atoi(argv[1]);

  char time_buf[64];
  
  // Truck main loop
  while (1) {
    if (shm->shutdown) break;

    // Dock Truck
    SEM_P(semid, SEM_DOCK);

    // Critical Part
    SEM_P(semid, SEM_MUTEX);

    shm->current_truck_pid = getpid();
    shm->truck_docked = 1;
    shm->current_truck_load = 0.0;
    shm->current_truck_vol = 0.0;

    // Reset force departure
    force_departure = 0;

    SEM_V(semid, SEM_MUTEX);

    get_time(time_buf, sizeof(time_buf));
    printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Truck docked, ready to load.\n",
	   time_buf, truck_id);
    
    // Loading Loop
    while (1) {
      if (force_departure) {
	get_time(time_buf, sizeof(time_buf));
	printf("["COLOR_YELLOW"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Forced departure signal received.\n", time_buf, truck_id);
	
	break;
      }

      if (shm->shutdown) break;

      // case: Limit is reached exactly (eg. truck load: 20/20 kg)
      if (shm->current_truck_load >= shm->truck_capacity_W ||
	  shm->current_truck_vol >= shm->truck_volume_V) {
	get_time(time_buf, sizeof(time_buf));
	printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Truck filled to capacity. Departure...\n", time_buf, truck_id);
	break;
      }
      
      // Waiting For Packages (SEM_FULL)
      // If process waits on SEM_FULL semaphore and forced departure is called
      // truck could possibly stuck here.
      // IPC_NOWAIT flag must be set up so we can regularly check if departure is being forced
      struct sembuf sb = {SEM_FULL, -1, IPC_NOWAIT};
      if(semop(semid, &sb, 1) == -1) {
	if (errno == EAGAIN) {
	  usleep(50000); // Waits 50ms to avoid busy loop slamming
	  continue;
	} else {
	  perror("Truck semop");
	  exit(1);
	}
      }

      // Package Available
      SEM_P(semid, SEM_MUTEX);

      // Get head package data
      int idx = shm->head;
      Package pkg = shm->belt[idx];
      
      double w = pkg.weight;
      double v = pkg.volume;

      // Reached Truck Load Limits Check
      if (shm->current_truck_load + w > shm->truck_capacity_W ||
	  shm->current_truck_vol + v > shm->truck_volume_V) {
	get_time(time_buf, sizeof(time_buf));
	printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Truck is full. Departure...\n",
	       time_buf, truck_id);
	SEM_V(semid, SEM_FULL); // Truck didn't load head package so it is still on belt
	SEM_V(semid, SEM_MUTEX);
	break;
      }

      // Limit NOT Reached
      shm->current_truck_load += w;
      shm->current_truck_vol += v;

      // Moving head
      shm->head = (shm->head + 1) % shm->max_items_K;
      shm->current_count--;
      shm->current_belt_weight -= w;

      SEM_V(semid, SEM_MUTEX);
      SEM_V(semid, SEM_EMPTY);
      
      get_time(time_buf, sizeof(time_buf));
      printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Loaded pkg %s %.2fkg. Total: %.2f/%.2f kg\n",
	     time_buf, truck_id, (pkg.type == 0 ? "A" : (pkg.type == 1 ? "B" : "C")), w, shm->current_truck_load, shm->truck_capacity_W);

      // Simulate loading time
      usleep(100000);
    } // END OF LOADING LOOP
    
    // Undocking
    SEM_P(semid, SEM_MUTEX);
    shm->truck_docked = 0;
    shm->current_truck_pid = 0;

    // case: departure was forced before first package was loaded. Send truck back to queue
    if (shm->current_truck_load == 0.0) {
      get_time(time_buf, sizeof(time_buf));
      printf("["COLOR_YELLOW"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Departure forced. Truck empty. Sending truck back to queue\n",
	     time_buf, truck_id);

      SEM_V(semid, SEM_MUTEX);
      SEM_V(semid, SEM_DOCK);

      sleep(1); // Drive back to queue
      continue;
    }
    
    SEM_V(semid, SEM_MUTEX);
    SEM_V(semid, SEM_DOCK);

    get_time(time_buf, sizeof(time_buf));
    printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Delivering packages...\n",
	   time_buf, truck_id);

    // Simulate delivery time (5s)
    sleep(5);

    get_time(time_buf, sizeof(time_buf));
    printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_CYAN" Truck %d  "COLOR_RESET"Truck returned to queue\n",
	   time_buf, truck_id);
  }
  
  return 0;
}

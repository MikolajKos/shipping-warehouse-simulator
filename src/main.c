#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common/common.h"
#include "common/sem_wrapper.h"
#include "common/shm_wrapper.h"
#include "common/utils.h"

// HELPER FUNCTIONS

void shm_init(SharedState *shm, int K, double M, double W, double V) {
  memset(shm, 0, sizeof(SharedState));

  shm->max_items_K = K;
  shm->max_belt_weight_M = M;
  shm->truck_capacity_W = W;
  shm->truck_volume_V = V;

  shm->shutdown = 0;
  shm->truck_docked = 0;
}

void sem_init(int semid, int K) {
  sem_set(semid, SEM_MUTEX, SETVAL, 1);
  sem_set(semid, SEM_EMPTY, SETVAL, K);
  sem_set(semid, SEM_FULL, SETVAL, 0);
  sem_set(semid, SEM_DOCK, SETVAL, 1);
}

int main(int argc, char *argv[]) {
  if (argc < 6) {
    fprintf(stderr, "Usage: %s <N_Trucks> <K_BeltCap> <M_MaxBeltW> <W_TruckCap> <V_TruckVol>\n", argv[0]);
    exit(1);
  }

  int N = atoi(argv[1]);
  int K = atoi(argv[2]);
  double M = atof(argv[3]);
  double W = atof(argv[4]);
  double V = atof(argv[5]);

  if (N<=0 || K<=0 || M<=0 || W<=0 || V<=0) {
    fprintf(stderr, "All parameters must be positive numbers.\n");
    exit(1);
  }

  if (K > MAX_BELT_CAPACITY) {
    fprintf(stderr, "K cannot exceed internal buffer limit (%d).\n", MAX_BELT_CAPACITY);
    exit(1);
  }

  // --- IPC Initialization ---
  // Semaphore init
  int semid = get_sem(KEY_PATH, KEY_ID_SEM, 4);

  // Shared mem attachment
  SharedState *shm;
  shm = (SharedState *)attach_memory_block(KEY_PATH, KEY_ID_SHM, sizeof(SharedState));

  shm_init(shm, K, M, W, V);
  sem_init(semid, K);

  printf("--- "COLOR_BLUE" Simulation Started "COLOR_RESET"---\n");
  printf("Params: N=%d, K=%d, M=%.2f, W=%.2f, V=%.2f\n", N, K, M, W, V);

  // --- Fork Processes ---

  // Worker P4 (Express)
  pid_t pid_p4 = fork();
  if(pid_p4 == 0) {
    execl("./worker_express", "worker_express", NULL);
    perror("Exec P4"); exit(1);
  }
  shm->p4_pid = pid_p4;
  
  // Workers: P1, P2, P3 (Standard)
  pid_t workers[3];
  const char *types[] = {"A", "B", "C"};
  
  for(int i=0; i<3; ++i) {
    if((workers[i] = fork()) == 0) {
      execl("./worker_std", "worker_std", types[i], NULL);
      perror("Exec Worker"); exit(1);
    }
  }

  // Trucks
  pid_t *trucks = malloc(sizeof(pid_t) * N);

  for(int i=0; i<N; ++i) {
    if((trucks[i] = fork()) == 0) {
      char id_str[11];
      sprintf(id_str, "%d", i+1);
      execl("./truck", "truck", id_str, NULL);
      perror("Exec Truck"); exit(1);
    }
  }

  // --- Dispatcher Loop ---
  int cmd;
  char time_buf[64];
    
  printf("\nCommands:\n 1: Force Truck Departure\n 2: Express Load (P4)\n 3: Shutdown\n");

  while(1) {
    printf("CMD> ");
    fflush(stdout);

    if (scanf("%d", &cmd) != 1) {
      // Consume garbage
      while(getchar() != '\n');
      printf("Incorrect intput. Enter command number\n");
      continue;
    }

    if (cmd == 1) {
      SEM_P(semid, SEM_MUTEX);

      if (shm->truck_docked) {
	get_time(time_buf);
	printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_BLUE"  Dispatcher "COLOR_RESET"Signaling truck %d to depart early.\n", time_buf, shm->current_truck_pid);

	// Sends force departure signal to the truck
	kill(shm->current_truck_pid, SIGUSR1);
      }
      else {
	get_time(time_buf, sizeof(time_buf));
	printf("["COLOR_YELLOW"%s"COLOR_RESET"]"COLOR_BLUE"  Dispatcher "COLOR_RESET"No truck at dock to release.\n", time_buf);
      }

      SEM_V(semid, SEM_MUTEX);
    }
    else if (cmd == 2) { // Signaling P4 (Express)
      get_time(time_buf, sizeof(time_buf));
      printf("["COLOR_GREEN"%s"COLOR_RESET"]"COLOR_BLUE"  Dispatcher "COLOR_RESET"Signaling P4 (Express).\n", time_buf);

      // No need for shared state access, p4_pid is being set once in dispatcher
      kill(shm->p4_pid, SIGUSR1);
    }
    else if (cmd == 3) {
      get_time(time_buf, sizeof(time_buf));
      printf("["COLOR_RED"%s"COLOR_RESET"]"COLOR_BLUE"  Dispatcher "COLOR_RESET"Shutting down...\n", time_buf);

      // Kills P1, P2 and P3
      for(int i=0; i<3; ++i) {
	kill(workers[i], SIGTERM);
	printf(" -- ["COLOR_YELLOW"-"COLOR_RESET"]  Worker: P%d\n", i+1);
      }
      // Kills P4 (Express)
      kill(shm->p4_pid, SIGTERM);
      printf(" -- ["COLOR_YELLOW"-"COLOR_RESET"]  Worker: P4 (Express)\n");
      // Kills trucks
      for(int i=0; i<N; ++i) {
	kill(trucks[i], SIGTERM);
	printf(" -- ["COLOR_YELLOW"-"COLOR_RESET"]  Truck: %d\n", i+1);
      }
    }
  }

  // Wait for child processes to end its work
  while(wait(NULL) > 0);
  
  // Destructing IPC and allocated mem
  free(trucks);
  
  detach_memory_block(shm);
  destroy_memory_block(KEY_PATH, KEY_ID_SHM);

  sem_set(semid, 0, IPC_RMID, 0);
  
  return 0;
}

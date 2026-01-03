#include <stdio.h>
#include <stdlib.h>

#include <common/common.h>
#include <common/sem_wrapper.h>
#include <common/shm_wrapper.h>
#include <common/utils.h>

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
  int semid = get_sem(KEY_PATH, KEY_ID_SEM, 4);
  int shmid = 

  return 0;
}

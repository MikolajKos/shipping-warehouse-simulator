#include "common.h"
#include "utils.h"
#include <time.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if(argc < 2) {
    fprintf(stderr, "Usage: %s <Type A/B/C>\n", argv[0]);
    exit(1);
  }

  // Determine package type
  PackageType type;
  if (strcmp(argv[1], "A")) type = PKG_A;
  else if (strcmp(argv[1], "B")) type = PKG_B;
  else if (strcmp(argv[1], "C")) type = PKG_C;
  else {
    fprintf(stderr, "Usage: %s <Type A/B/C>\n", argv[0]);
    exit(1);
  }

  // IPC Setup
  key_t key_shm = ftok(KEY_PATH, KEY_ID_SHM);
  key_t key_sem = ftok(KEY_PATH, KEY_ID_SEM);

  // Setting minimal privileges
  int shmid = shmget(key_shm, 0, 0600);
  int semid = semget(key_sem, 0, 0600);

  if (shmid == -1 || semid == -1) {
    perror("Std. Worker: IPC attach failed");
    exit(1);
  }

  // Attaching shared memory
  SharedState *shm;

  void* shm_result = shmat(shmid, 0, NULL);
  if (shm_result == (void *)-1) {
    perror("Std. Worker: shmat() failed");
    exit(1);
  }

  shm = (SharedState *)shm_result;

  return 0;
}

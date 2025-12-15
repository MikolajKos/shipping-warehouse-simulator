#include "shm_wrapper.h"

// Private function
static int get_shared_block(const char* filename, int proj_id, size_t size) {
  key_t shm_key = ftok(filename, proj_id);

  int shmid = shmget(shm_key, size, 0600|IPC_CREAT);
  if (shmid == -1) {
    perror("Shm. wrapper: shmget error");
    exit(1);
  }

  return shmid;
}

void* attach_memory_block(const char* filename, int proj_id, size_t size) {
  int shmid = get_shared_block(filename, proj_id, size);

  void *shm_result = shmat(shmid, (void *)0, 0);
  if (shm_result == (void *)-1) {
    perror("Shm. wrapper: shmat error");
    exit(1);
  }

  return shm_result;
}

void detach_memory_block(void *pdata) {
  if (shmdt(pdata) == -1) {
    perror("Shm. wrapper: could not detach memory. shmdt() error.");
    exit(1);
  }
}

void destroy_memory_block(const char* filename, int proj_id) {
  int shmid = get_shared_block(filename, proj_id, 0);

  if(shmctl(shmid, IPC_RMID, NULL) == -1) {
    perror("Shm. wrapper: shmctl() error");
    exit(1);
  }
}


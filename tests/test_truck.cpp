#include <gtest/gtest.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
  #include "../src/common/common.h"

  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  };
}

class TruckTest ::testing::Test {
protected:
  SharedState *shm;
  int semid;
  int shmid;

  void SetUp() {
    // IPC Keys
    key_t key_shm = ftok(KEY_PATH, KEY_ID_SHM);
    key_t key_sem = ftok(KEY_PATH, KEY_ID_SEM);
    
    // Shared Memory Attachment
    shmid = shmget(key_shm, sizeof(SharedState), 0600|IPC_CREAT);
    ASSERT_NE(shmid, -1) << "Failed to create SHM";
    shm = (SharedState *)shmat(shmid, (void *)0, 0);
    ASSERT_NE(shm, (void *)-1) << "Failed to attach SHM";

    // Set shm
    memset(shm, 0, sizeof(SharedState));
    shm->max_items_K = 10;
    shm->max_belt_weight_M = 500.0;
    shm->current_truck_load = 0.0;
    shm->truck_docked = 0.0;
    shm->shutdown = 0;
    
    // Init Semaphores
    semid = shmget(key_sem, 4, 0600|IPC_CREATE);
    ASSERT_NE(semid, -1) << "Failed to create SEM";

    union semun arg;

    arg.val = 1;
    semctl(semid, SEM_MUTEX, SETVAL, arg);

    arg.val = 0;
    semctl(semid, SEM_DOCK, SETVAL, arg);

    semctl(semid, SEM_FULL, SETVAL, arg);
    
    // Belt empty by default
    arg.val = shm->myax_items_K;
    semctl(semid, SEM_EMPTY, SETVAL, arg);
    
  }

  void TearDown() {
    // Detach and destroy shared memory
    shmdt(shm);
    shmctl(shmid, IPC_RMID, 0);

    
  }

  void RunTruckProcess() {

  }
};

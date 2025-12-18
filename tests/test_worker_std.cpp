#include <gtest/gtest.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>

extern "C" {
  #include "../src/common/common.h"

  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  };
}

class WorkerStandardTest : public ::testing::Test {
protected:
  int shmid;
  int semid;
  SharedState *shm;
  pid_t worker_pid = -1;

  void SetUp() override {
    // IPC Key Setup
    key_t key_shm = ftok(KEY_PATH, KEY_ID_SHM);
    key_t key_sem = ftok(KEY_PATH, KEY_ID_SEM);

    // Shared Memory Attachment
    shmid = shmget(key_shm, sizeof(SharedState), 0600|IPC_CREAT);
    ASSERT_NE(shmid, -1) << "Failed to create SHM";

    shm = (SharedState *)shmat(shmid, (void *)0, 0);
    ASSERT_NE(shm, (void *)-1) << "Failed to attach SHM";

    // Initial State
    memset(shm, 0, sizeof(SharedState));
    shm->max_items_K = 5; // 5 slots empty by default
    shm->max_belt_weight_M = 200.0;
    shm->shutdown = 0;
    shm->tail = 0;

    // Create Semaphores
    semid = semget(key_sem, 3, 0600|IPC_CREAT);
    ASSERT_NE(semid, -1) << "Failed to create Semaphores";

    // Sets 5 empty spaces on belt (SEM_EMPTY)
    union semun arg_empty;
    arg_empty.val = shm->max_items_K;
    semctl(semid, SEM_EMPTY, SETVAL, arg_empty);


    // Sets Mutex to 1 allowing shm access (SEM_MUTEX)
    union semun arg_mutex;
    arg_mutex.val = 1;
    semctl(semid, SEM_MUTEX, SETVAL, arg_mutex);

    // Sets occupied belt space to 0 (SEM_FULL)
    union semun arg_full;
    arg_full.val = 0;
    semctl(semid, SEM_FULL, SETVAL, arg_full); 
  }

  void TearDown() {
    // Kill worker process if running
    if (worker_pid > 0) {
      kill(worker_pid, SIGKILL);
      waitpid(worker_pid, NULL, 0);
    }

    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
  }

  void RunWorkerProcess() {
    worker_pid = fork();
    if(worker_pid == 0) {
      // Run process for P2 worker handling PKG_B type packages
      execl("../src/worker_std", "worker_std", "B", NULL); 

      perror("execl failed");
      exit(1);
    }

    // Wait for child process
    usleep(100000); // 100ms
  }
};

// TEST 1: worker should load packages if there is empty space
TEST_F(WorkerStandardTest, PlacesPackagesIfSpaceEmpty) {
  RunWorkerProcess();
  usleep(500000);
  
  EXPECT_GT(shm->current_belt_weight, 0.0);
}

// TEST 1: belts weight limit is reached, worker can't place next package
TEST_F(WorkerStandardTest, BeltsWeightLimitReachedCantPlace) {
  shm->max_belt_weight_M = 0.0;
  
  RunWorkerProcess();
  usleep(500000);

  EXPECT_EQ(shm->current_belt_weight, 0.0);
}

#include <fstream>
#include <gtest/gtest.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" {
  #include "../src/common/common.h"
  
  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  };
}

class WorkerExpressTest : public ::testing::Test {
protected:
  int shmid;
  int semid;
  SharedState *shm;
  pid_t worker_pid = -1;

  void SetUp() override {
    // IPC Setup
    int key_shm = ftok(KEY_PATH, KEY_ID_SHM);
    int key_sem = ftok(KEY_PATH, KEY_ID_SEM);

    // Main simulation
    shmid = shmget(key_shm, sizeof(SharedState), 0600|IPC_CREAT);
    ASSERT_NE(shmid, -1) << "Failed to create SHM";
    shm = (SharedState *)shmat(shmid, (void*)0, 0);
    ASSERT_NE(shm, (void *)-1) << "Failed to attach SHM";

    // Initial state
    memset(shm, 0, sizeof(SharedState));
    shm->truck_capacity_W = 1000.0;
    shm->truck_volume_V = 1000.0;
    shm->current_truck_load = 0.0;
    shm->truck_docked = 0;
    shm->shutdown = 0;

    // Init Sem
    semid = semget(key_sem, 3, 0600|IPC_CREAT);
    ASSERT_NE(semid, -1) << "Failed to create Semaphores";

    // Sets mutex to 1
    union semun arg;
    arg.val = 1;
    semctl(semid, SEM_MUTEX, SETVAL, arg);
  }

  void TearDown() override {
    // Kill worker process if running
    if (worker_pid > 0) {
      kill(worker_pid, SIGKILL);
      waitpid(worker_pid, NULL, 0);
    }

    // Detach and delete resources
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
  }

  void RunWorkerProcess() {
    worker_pid = fork();
    if(worker_pid == 0) {
      execl("../src/worker_express", "worker_express", NULL);

      perror("execl failed");
      exit(1);
    }

    // Parent waits for child
    usleep(100000); // 100ms
  }
};

// TEST 1: worker shouldn't load if truck is not docked
TEST_F(WorkerExpressTest, IgnoresSignalWhenNoTruck) {
  shm->truck_docked = 0;

  RunWorkerProcess();

  kill(worker_pid, SIGUSR1);
  usleep(100000);

  EXPECT_DOUBLE_EQ(shm->current_truck_load, 0.0);
}

// TEST 2: worker should load packages (Truck docked)
TEST_F(WorkerExpressTest, LoadsPackagesWhenTruckDocked) {
  shm->truck_docked = 1;

  RunWorkerProcess();

  kill(worker_pid, SIGUSR1);
  usleep(200000);

  EXPECT_GT(shm->current_truck_load, 0.0);
  EXPECT_GT(shm->current_truck_vol, 0.0);
}

// TEST 3: load limits
TEST_F(WorkerExpressTest, SkipPackagesIfLimitReached) {
  shm->truck_docked = 1;
  shm->truck_capacity_W = 1.0;
  
  RunWorkerProcess();

  kill(worker_pid, SIGUSR1);
  usleep(200000);

  EXPECT_LE(shm->current_truck_load, shm->truck_capacity_W);
}

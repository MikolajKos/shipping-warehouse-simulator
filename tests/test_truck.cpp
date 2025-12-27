#include <gtest/gtest.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
  #include "../src/common/common.h"
  #include "../src/common/utils.h"

  union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  };
}

class TruckTest : public ::testing::Test {
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
    shm->truck_docked = 0;
    shm->shutdown = 0;

    shm->truck_capacity_W = 1000.0; 
    shm->truck_volume_V = 1000.0;
    
    // Init Semaphores
    semid = semget(key_sem, 4, 0600|IPC_CREAT);
    ASSERT_NE(semid, -1) << "Failed to create SEM";

    union semun arg;

    arg.val = 1;
    semctl(semid, SEM_MUTEX, SETVAL, arg);

    semctl(semid, SEM_DOCK, SETVAL, arg);

    arg.val = 0;
    semctl(semid, SEM_FULL, SETVAL, arg);
    
    // Belt empty by default
    arg.val = shm->max_items_K;
    semctl(semid, SEM_EMPTY, SETVAL, arg);
    
  }

  void TearDown() {
    // Kill truck process
    if(shm->current_truck_pid > 0) {
      kill(shm->current_truck_pid, SIGKILL);
      waitpid(shm->current_truck_pid, NULL, 0);
    }

    // Detach and destroy shared memory
    shmdt(shm);
    shmctl(shmid, IPC_RMID, 0);

    semctl(semid, 0, IPC_RMID);
  }

  void RunTruckProcess() {
    shm->current_truck_pid = fork();

    if (shm->current_truck_pid == 0) {
      execl("../src/truck", "truck", "1", NULL);

      perror("execl failed");
      exit(1);
    }

    // Parent waits for child
    usleep(100000);
  }

  void PlacePkgsOnBelt(int count) {
    shm->max_items_K = count;
    shm->max_belt_weight_M = count * 25.0;

    Package pkg;
    for (int i = 0; i < count; ++i) {
      pkg.id = i; 
      pkg.type = get_rand_package_type();
      pkg.weight = generate_weight(pkg.type);
      pkg.volume = get_volume(pkg.type);

      shm->belt[i] = pkg;

      shm->current_belt_weight += pkg.weight;
    }
    shm->tail = count % shm->max_items_K;
    
    union semun arg;
    arg.val = count;
    semctl(semid, SEM_FULL, SETVAL, arg);
  }
};

TEST_F(TruckTest, LoadingAllPackages) {
  PlacePkgsOnBelt(5);

  ASSERT_GT(shm->current_belt_weight, 0.0);
  double initial_belt_weight = shm->current_belt_weight;

  RunTruckProcess();
  sleep(3);

  EXPECT_DOUBLE_EQ(initial_belt_weight, shm->current_truck_load);
}

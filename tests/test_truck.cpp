#include <gtest/gtest.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <vector>

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
  //pid_t saved_truck_pid = -1;
  std::vector<pid_t> truck_pids; // Collection of all runnig truck pids
  
  SharedState *shm;
  int semid;
  int shmid;
  int current_belt_item_id = 0;
  
  void SetUp() override {
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
    shm->max_items_K = 100;
    shm->max_belt_weight_M = 1000.0;
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

  void TearDown() override {
    // Kill truck processes
    for (pid_t pid : truck_pids) {
      if (pid > 0) {
	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
      }
    }
    truck_pids.clear();
    
    // Detach and destroy shared memory
    shmdt(shm);
    shmctl(shmid, IPC_RMID, 0);

    // Destroy Semaphores
    semctl(semid, 0, IPC_RMID);
  }

  void RunTruckProcess(int id) {
    pid_t pid = fork();

    if (pid == 0) {
      std::string id_str = std::to_string(id);
      
      execl("../src/truck", "truck", id_str.c_str(), NULL);
      perror("execl failed");
      exit(1);
    }

    truck_pids.push_back(pid);

    // Parent waits for child
    usleep(100000);
  }

  void PlacePkgsOnBelt(int count, double preset_w = 0, double preset_v = 0, PackageType preset_type = PKG_END) {
    shm->max_belt_weight_M += count * 25.0;
    
    Package pkg;
    for (int i = 0; i < count; ++i) {
      pkg.id = current_belt_item_id;

      pkg.type = preset_type == PKG_END ? get_rand_package_type() : preset_type;
      pkg.weight = preset_w ? preset_w : generate_weight(pkg.type);
      pkg.volume = preset_v ? preset_v : get_volume(pkg.type);

      shm->belt[current_belt_item_id++] = pkg;

      shm->current_belt_weight += pkg.weight;
      shm->current_count++;
    }
    shm->tail = shm->current_count % shm->max_items_K;
    
    union semun arg;
    arg.val = shm->current_count;
    semctl(semid, SEM_FULL, SETVAL, arg);
  }
};

// Truck loaded all packages from belt
TEST_F(TruckTest, LoadingAllPackages) {
  PlacePkgsOnBelt(5);

  ASSERT_GT(shm->current_belt_weight, 0.0);
  double initial_belt_weight = shm->current_belt_weight;

  RunTruckProcess(1);
  sleep(3);

  EXPECT_DOUBLE_EQ(initial_belt_weight, shm->current_truck_load);
}

TEST_F(TruckTest, PkgLoadingAndDeparture) {
  int count = 2;
  double weight = 20.0;
  double weight_sum = count * weight;

  // Set truck capacity
  shm->truck_capacity_W = weight;

  PlacePkgsOnBelt(count, weight, 0, PKG_C);
  ASSERT_EQ(shm->current_belt_weight, weight_sum);
  
  union semun arg;
  arg.val = count;
  semctl(semid, SEM_FULL, SETVAL, arg);
  
  RunTruckProcess(1);
  
  // Wait for truck to load and deliver all 3 packgages
  // About 5.5s for each package
  sleep(20);
  
  EXPECT_DOUBLE_EQ(shm->current_truck_load, 0.0);
  EXPECT_DOUBLE_EQ(shm->current_truck_vol, 0.0);

  EXPECT_DOUBLE_EQ(shm->current_belt_weight, 0.0);
}

TEST_F(TruckTest, RespectsVolumeLimits) {
  shm->truck_capacity_W = 1000.0;
  shm->truck_volume_V = 10.0;

  int count = 2;
  double weight = 10.0;
  double volume = 6.0;
  double weight_sum = count * weight;
  
  PlacePkgsOnBelt(count, weight, volume, PKG_C);
  ASSERT_EQ(weight_sum, shm->current_belt_weight);

  RunTruckProcess(1);
  sleep(2); // Loading package

  EXPECT_DOUBLE_EQ(shm->current_truck_load, 10.0);
  EXPECT_DOUBLE_EQ(shm->current_truck_vol, 6.0);

  // One package left on belt
  EXPECT_DOUBLE_EQ(shm->current_belt_weight, 10.0);
}


TEST_F(TruckTest, ForcedDepartureBySignal) {
  shm->truck_docked = 0;

  int count = 10;
  PlacePkgsOnBelt(count);

  RunTruckProcess(1);
  usleep(200000); // Loads 1-3 packages

  pid_t current_t_pid = truck_pids.at(0);
  
  ASSERT_GT(current_t_pid, 0);
  // Sends signal
  kill(current_t_pid, SIGUSR1);

  sleep(1); // Wait for action

  // Few packages must be loaded
  EXPECT_GT(shm->current_truck_load, 0.0);

  // Truck can't load all packages
  EXPECT_LT(shm->current_truck_load, shm->current_belt_weight + shm->current_truck_load);

  // Truck should be delivering
  EXPECT_EQ(shm->truck_docked, 0);
}


TEST_F(TruckTest, SkipOversizedPackage) {
  shm->truck_capacity_W = 10.0;
  shm->truck_volume_V = 100.0;

  int count = 1;
  double weight = 20.0;

  PlacePkgsOnBelt(count, weight);

  RunTruckProcess(1);
  usleep(400000); // Loading package

  // Truck should be empty
  EXPECT_DOUBLE_EQ(shm->current_truck_load, 0.0);
  EXPECT_DOUBLE_EQ(shm->current_truck_vol, 0.0);

  // Package wasn't loaded, should be still on belt
  EXPECT_DOUBLE_EQ(shm->current_belt_weight, weight);
  EXPECT_EQ(shm->current_count, 1);
}

// Testing consistency of package loading
// Next arriving truck should load first package from belt
TEST_F(TruckTest, NextTruckLoadsFirstItem) {
  shm->truck_capacity_W = 10.0;
  shm->truck_volume_V = 100.0;

  // Placing two different packages to distinguish trucks
  PlacePkgsOnBelt(1, 10.0);
  PlacePkgsOnBelt(1, 7.0);
  // This Package should not be loaded
  PlacePkgsOnBelt(1, 100.0);
  
  RunTruckProcess(1);

  // This is process that should load last package
  RunTruckProcess(2);

  sleep(2); // Waits for last truck to load package

  EXPECT_DOUBLE_EQ(shm->current_truck_load, 7.0);
  EXPECT_DOUBLE_EQ(shm->current_belt_weight, 100.0);
  EXPECT_EQ(shm->current_count, 1);
}

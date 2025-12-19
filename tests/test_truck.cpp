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
    
  }

  void TearDown() {
    
  }

  void RunTruckProcess() {

  }
};

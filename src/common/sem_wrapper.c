#include "sem_wrapper.h"

// Union definition for semctl function
union semun {
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

void sem_op(int semid, int sem_num, int op) {
  struct sembuf sb;
  sb.sem_num = sem_num;
  sb.sem_op = op;
  sb.sem_flg = SEM_UNDO;

  while (semop(semid, &sb, 1) == -1) {
    if (errno == EINTR) continue; // Signal was handled, retry wait
    perror("Sem. wrapper: semop() error");
    exit(1);
  }
}

void sem_set(int semid, int sem_num, int cmd, int val) {
  union semun su;
  su.val = val;

  if (semctl(semid, sem_num, cmd, su) == -1) {
    perror("Sem. wrapper: semctl() error");
    exit(1);
  }
}

int get_sem(const char* filename, int proj_id, int sem_num) {
  key_t sem_key = ftok(filename, proj_id);

  int semid = semget(sem_key, sem_num, 0600|IPC_CREAT);
  if (semid == -1) {
    perror("Sem. wrapper: semget() error");
    exit(1);
  }

  return semid;
}

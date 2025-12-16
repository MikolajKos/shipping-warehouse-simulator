#ifndef SEM_WRAPPER_H
#define SEM_WRAPPER_H

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define SEM_P(semid, sem_num) sem_op(semid, sem_num, -1)
#define SEM_V(semid, sem_num) sem_op(semid, sem_num, 1)

#define SEM_INIT_LOCKED(semid, sem_num) sem_set(semid, sem_num, SETVAL, 0)
#define SEM_INIT_OPEN(semid, sem_num) sem_set(semid, sem_num, SETVAL, 1)

void sem_op(int semid, int sem_num, int op);

void sem_set(int semid, int sem_num, int cmd, int val);

int get_sem(const char* filename, int proj_id, int semnum);

#endif // SEM_WRAPPER_H

#ifndef SEM_WRAPPER_H
#define SEM_WRAPPER_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @file sem_wrapper.h
 * @brief Wrapper functions for System V Semaphores.
 *
 * This header provides a simplified interface for creating, initializing,
 * and operating on semaphore sets using standard IPC mechanisms.
 * It includes macros for standard P (wait/lock) and V (signal/unlock) operations.
 */

/**
 * @brief Performs a P operation on a semaphore.
 *
 * Decrements the semaphore value by 1. If the value is 0, the process blocks
 * until the semaphore becomes available (greater than 0).
 * Effectively locks a resource.
 *
 * @param semid The semaphore set identifier.
 * @param sem_num The index of the semaphore in the set.
 */
#define SEM_P(semid, sem_num) sem_op(semid, sem_num, -1)

/**
 * @brief Performs a V operation on a semaphore.
 *
 * Increments the semaphore value by 1. If other processes are waiting
 * for this semaphore, one of them will be unblocked.
 * Effectively unlocks a resource.
 *
 * @param semid The semaphore set identifier.
 * @param sem_num The index of the semaphore in the set.
 */
#define SEM_V(semid, sem_num) sem_op(semid, sem_num, 1)

/**
 * @brief Initializes a semaphore to 0 (Locked/Taken state).
 *
 * Helper macro used during system initialization.
 *
 * @param semid The semaphore set identifier.
 * @param sem_num The index of the semaphore to initialize.
 */
#define SEM_INIT_LOCKED(semid, sem_num) sem_set(semid, sem_num, SETVAL, 0)

/**
 * @brief Initializes a semaphore to 1 (Unlocked/Available state).
 *
 * Helper macro used during system initialization.
 *
 * @param semid The semaphore set identifier.
 * @param sem_num The index of the semaphore to initialize.
 */
#define SEM_INIT_OPEN(semid, sem_num) sem_set(semid, sem_num, SETVAL, 1)

/**
 * @brief Executes a generic operation on a semaphore.
 *
 * This function wraps `semop()`. It sets the `SEM_UNDO` flag, ensuring that
 * if the process terminates unexpectedly (e.g., crash), the changes to the
 * semaphore are automatically undone by the OS to prevent deadlocks.
 * Also EINTR errno value is handled in case when process was interupted
 * by a signal. Without this handler, after receiving signal, semaphore
 * would return -1 and crash simulation.
 *
 * @param semid The semaphore set identifier.
 * @param sem_num The index of the specific semaphore within the set (0-based).
 * @param op The operation value (negative to wait/decrement, positive to signal/increment).
 */
void sem_op(int semid, int sem_num, int op);

/**
 * @brief Sets the value of a specific semaphore (wrapper for semctl).
 *
 * Typically used for initializing semaphores before use.
 *
 * @param semid The semaphore set identifier.
 * @param sem_num The index of the semaphore within the set.
 * @param cmd The command flag (typically SETVAL).
 * @param val The value to set the semaphore to.
 */
void sem_set(int semid, int sem_num, int cmd, int val);

/**
 * @brief Creates or retrieves a semaphore set.
 *
 * Generates a unique key using `ftok` based on the provided filename and project ID,
 * then calls `semget` to obtain the semaphore set identifier.
 *
 * @param filename The path to an existing file used for key generation.
 * @param proj_id The project identifier (single character integer) for key generation.
 * @param semnum The number of semaphores to create in the set (array size).
 * @return int The semaphore set identifier (semid) on success. Exits on failure.
 */
int get_sem(const char* filename, int proj_id, int semnum);

#endif // SEM_WRAPPER_H

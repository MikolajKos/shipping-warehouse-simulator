#ifndef SHM_WRAPPER_H
#define SHM_WRAPPER_H

#include "common.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @file shm_wrapper.h
 * @brief Interface for System V Shared Memory operations.
 *
 * This header declares functions to attach, detach, and destroy shared memory blocks
 * used for inter-process communication (IPC).
 */

/**
 * @brief Creates or attaches to a shared memory block.
 *
 * This function attempts to get a shared memory identifier associated with the
 * provided filename. If the block does not exist, it creates one. Then, it attaches
 * the block to the process's address space.
 *
 * @param filename The file path used to generate a unique key (using ftok).
 * @param proj_id Project unique ID number for key generation (using ftok).
 * @param size The size of the shared memory block in bytes.
 * @return void* A pointer to the attached shared memory block. Returns (void*)-1 or NULL on failure.
 */
void* attach_memory_block(const char* filename, int proj_id, size_t size);

/**
 * @brief Detaches the shared memory block from the process.
 *
 * Detaching does not destroy the memory block; it only makes it inaccessible
 * to the current process.
 *
 * @param pdata A pointer to the shared memory block to be detached.
 */
void detach_memory_block(void *pdata);

/**
 * @brief Destroys the shared memory block.
 *
 * This function marks the shared memory segment for removal. The segment is
 * typically removed by the OS only after all processes have detached from it.
 *
 * @param filename The file path associated with the shared memory block.
 * @param proj_id Project ID number used to generate ftok key.
 */
void destroy_memory_block(const char* filename, int proj_id);

#endif // SHM_WRAPPER_H

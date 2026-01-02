#ifndef COMMON_H
#define COMMON_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>

/**
 * @file common.h
 * @brief Common definitions, IPC structures, and helper functions
*/

/**
 * @name ANSI Color Codes
 * @brief ANSI escape sequences used for colorizing terminal output.
 *
 * These macros allow for colored text in standard output to visually distinguish
 * between different processes (e.g., Workers, Trucks) or message types (Errors, Info).
 *
 * @note Always ensure to append @ref COLOR_RESET at the end of the string to restore
 * default terminal colors.
 *
 * @{
 */
#define COLOR_RED     "\x1b[31m" /**< Sets text color to Red (often used for Errors or Alerts). */
#define COLOR_GREEN   "\x1b[32m" /**< Sets text color to Green (often used for Success messages). */
#define COLOR_YELLOW  "\x1b[33m" /**< Sets text color to Yellow (often used for Warnings). */
#define COLOR_BLUE    "\x1b[34m" /**< Sets text color to Blue. */
#define COLOR_MAGENTA "\x1b[35m" /**< Sets text color to Magenta. */
#define COLOR_CYAN    "\x1b[36m" /**< Sets text color to Cyan. */
#define COLOR_RESET   "\x1b[0m"  /**< Resets text color to default terminal settings. */
/** @} */

/**
 * @name IPC Key Generation Constants
 * Constants used to generate unique IPC keys via ftok().
 * @{
 */
#define KEY_PATH "."
#define KEY_ID_SHM 65
#define KEY_ID_SEM 66
/** @} */

/**
 * @name Constraints
 * @{
 */
/** @brief Physical hard limit for the belt array size. Logical limit is passed via arguments. */
#define MAX_BELT_CAPACITY 100
/** @} */

/**
 * @name Semaphore Indices
 * Indices used to access specific semaphores in the set.
 * @{
 */
#define SEM_MUTEX 0    /**< Binary Semaphore: Protects critical sections in Shared Memory. */
#define SEM_EMPTY 1    /**< Counting Semaphore: Tracks available empty slots on the belt. */
#define SEM_FULL  2    /**< Counting Semaphore: Tracks number of items currently on the belt. */
#define SEM_DOCK  3    /**< Binary Semaphore: 1 if Loading Dock is free, 0 if occupied. */
#define SEM_NUM   4    /**< Total number of semaphores in the set. */
/** @} */

/**
 * @brief Defines the types of packages available in the simulation.
 * * Dimensions and Volumes:
 * - Type A: 64x38x8  cm -> 0.019 m3
 * - Type B: 64x38x19 cm -> 0.046 m3
 * - Type C: 64x38x41 cm -> 0.099 m3
 */
typedef enum {
  PKG_A, /**< Small package (0.019 m3) */
  PKG_B, /**< Medium package (0.046 m3) */
  PKG_C, /**< Large package (0.099 m3) */
  PKG_END/**< End of package type list (type count)*/
} PackageType;

/**
 * @brief Represents a single package on the conveyor belt.
 */
typedef struct {
    int id;             /**< Unique identifier for the package. */
    PackageType type;   /**< Type of the package (A, B, or C). */
    double weight;      /**< Weight of the package in kg. */
    double volume;      /**< Volume of the package in m3. */
} Package;

/**
 * @brief Main Shared Memory structure.
 * * This structure acts as the central data store for the simulation, containing
 * configuration, the circular buffer for the belt, and synchronization flags.
 */
typedef struct {
  /* Configuration set by main process */
  int max_items_K;
  double max_belt_weight_M;
  double truck_capacity_W;
  double truck_volume_V;

  /* System State */
  int shutdown;         /**< Flag to signal all process to terminate. */
  pid_t p4_pid;         /**< Express worker (P4) pid */

  /* Belt State */
  Package belt[MAX_BELT_CAPACITY];
  int head;             /**< Index to pop from belt */
  int tail;             /**< Index to place intems into from belt (push) */
  int current_count;
  double current_belt_weight;

  /* Truck Interface */
  pid_t current_truck_pid;
  int truck_docked;
  double current_truck_load;
  double current_truck_vol;
  //int force_departure;  /**< Flag for early truck departure */

} SharedState;

#endif // COMMON_H

#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/**
 * @file utils.h
 * @brief Utility functions header file.
 *
 * Contains declarations for time handling, random package property generation,
 * and physical parameter calculations.
 */

/**
 * @brief Retrieves the current system timestamp as a string.
 *
 * Gets the current time and formats it into the provided buffer.
 * Ensure the buffer is large enough to hold the resulting string.
 *
 * @param buffer Pointer to the character buffer where the time string will be stored.
 * @param size The size of the buffer (in bytes).
 */
void get_time(char* buffer, size_t size);

/**
 * @brief Generates a random weight for a specific package type.
 *
 * Uses a simple heuristic where smaller package types tend to be lighter.
 * The generated weight is typically between 0.1 and 25.0.
 *
 * @param type The type of the package (defined in common.h).
 * @return double The generated weight of the package.
 */
double generate_weight(PackageType type);

/**
 * @brief Retrieves the volume for a given package type.
 *
 * Returns a fixed volume value associated with the specific package category.
 *
 * @param type The type of the package.
 * @return double The volume corresponding to the package type.
 */
double get_volume(PackageType type);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H

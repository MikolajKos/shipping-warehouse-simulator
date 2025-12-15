#include <string.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "shm_wrapper.h"
#include "utils.h"

int main(int argc, char *argv[]) {
  if(argc < 2) {
    fprintf(stderr, "Usage: %s <Type A/B/C>\n", argv[0]);
    exit(1);
  }

  // Determine package type
  PackageType type;
  if (strcmp(argv[1], "A")) type = PKG_A;
  else if (strcmp(argv[1], "B")) type = PKG_B;
  else if (strcmp(argv[1], "C")) type = PKG_C;
  else {
    fprintf(stderr, "Usage: %s <Type A/B/C>\n", argv[0]);
    exit(1);
  }

  // Shared Memory and Semaphores attachment
  SharedState *shm;

  shm = (SharedState *)attach_memory_block(
    KEY_PATH,
    KEY_ID_SHM,
    sizeof(SharedState)
  );
  
  return 0;
}

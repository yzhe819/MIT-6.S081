#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  // no arguments or too many arguments
  if (argc != 2) {
    fprintf(2, "usage: grep sleep [duration]\n");
    exit(1);
  }
  // use atoi convert string to int
  sleep(atoi(argv[1]));
  exit(0);
}

#include "kernel/types.h"
#include "user/user.h"

// 0 for read and 1 for write
#define RD 0
#define WR 1

void primes(int *p);

int main() {
  int p[2];
  pipe(p);

  if (fork() == 0) { // child process
    primes(p);
  } else { // parent process
    close(p[RD]);
    for (int i = 2; i <= 35; i++) {
      write(p[WR], &i, sizeof(int));
    }
    close(p[WR]);
    wait(0);
  }
  exit(0);
}

void primes(int *p) {
  int prime, num, p_next[2];
  close(p[WR]);

  if (read(p[RD], &prime, sizeof(int)) == 0) {
    close(p[RD]);
    exit(0);
  }

  printf("prime %d\n", prime);
  pipe(p_next);
  if (fork() > 0) { // parent process
    close(p_next[RD]);
    while (read(p[RD], &num, sizeof(int))) {
      if (num % prime) {
        write(p_next[WR], &num, sizeof(int));
      }
    }
    close(p[RD]);
    close(p_next[WR]);
    wait(0);
  } else { // child process
    primes(p_next);
  }

  close(p[RD]);
}
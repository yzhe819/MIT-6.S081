#include "kernel/types.h"
#include "user/user.h"

#define RD 0
#define WR 1

// the child receives the primes from the parent
void primes(int *p);
// print the first prime and return it
int get_first_prime(int *p);
// transmit the data to the next child
void transmit_data(int *p, int *p_next, int val);

int main(int argc, char *argv[]) {
  int p[2]; // 0 for read and 1 for write
  pipe(p);

  for (int i = 2; i <= 35; i++) {
    write(p[WR], &i, sizeof(int));
  }

  int pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    close(p[RD]);
    close(p[WR]);
    exit(1);
  } else if (pid == 0) {
    // child process
    close(p[WR]);
    primes(p);
    close(p[RD]);
    exit(0);
  } else {
    // parent process
    close(p[RD]);
    close(p[WR]);
    wait(0);
    exit(0);
  }
}

void primes(int *p) {
  int prime_num = get_first_prime(p);
  int p_next[2]; // 0 for read and WR for write
  pipe(p_next);

  int pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    // close the parent pipe
    close(p[RD]);
    close(p_next[RD]);
    close(p_next[WR]);
    exit(WR);
  } else if (pid == 0) {
    // child process
    close(p[0]);
    close(p_next[WR]);
    primes(p_next);
    close(p_next[RD]);
    exit(0);
  } else {
    // parent process
    close(p_next[RD]);
    transmit_data(p, p_next, prime_num);
    close(p[RD]);
    close(p_next[WR]);
    wait(0);
    exit(0);
  }
}

int get_first_prime(int *p) {
  int num;
  int len = read(p[RD], &num, sizeof(int));
  if (len == 0) {
    // no data to read
    close(p[RD]);
    exit(0);
  } else {
    printf("prime %d\n", num); // print the first prime number
    return num; // actually, the first received number must be the prime number
  }
}

void transmit_data(int *p, int *p_next, int val) {
  int num;
  // continue to read the next number
  while (read(p[RD], &num, sizeof(int))) {
    if (num % val) {
      write(p_next[WR], &num, sizeof(int)); // only write the prime number
    }
  }
}
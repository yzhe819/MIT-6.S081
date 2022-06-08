#include "kernel/types.h"
#include "user/user.h"

#define RD 0 // the read end of the pipe
#define WR 1 // the write end of the pipe

int main(int argc, char *argv[]) {
  int ptc[2], ctp[2]; // 0 for read, 1 for write

  // create pipe
  pipe(ptc);
  pipe(ctp);

  char buf = 'P';

  int pid = fork();

  do {
    if (pid > 0) {
      // parent
      close(ptc[RD]); // close read end of pipe
      close(ctp[WR]); // close write end of pipe

      // send the ping to the child
      if (write(ptc[WR], &buf, sizeof(char)) != sizeof(char)) {
        fprintf(2, "parent write error\n");
        break;
      }

      // receive the pong from the child
      if (read(ctp[RD], &buf, sizeof(char)) != sizeof(char)) {
        fprintf(2, "parent read error\n");
        break;
      } else {
        fprintf(1, "%d: received pong\n", getpid());
      }

      close(ctp[WR]); // close write end of pipe
      close(ptc[RD]); // close read end of pipe

    } else if (pid == 0) {
      // child
      close(ptc[WR]); // close write end of pipe
      close(ctp[RD]); // close read end of pipe

      // receive ping from the parent
      if (read(ptc[RD], &buf, sizeof(char) != sizeof(char))) {
        fprintf(2, "child read error\n");
        break;
      } else {
        fprintf(1, "%d: received ping\n", getpid());
      }

      // send pong to the parent
      if (write(ctp[WR], &buf, sizeof(char)) != sizeof(char)) {
        fprintf(2, "child write error\n");
        break;
      }

      close(ctp[RD]); // close read end of pipe
      close(ptc[WR]); // close write end of pipe

    } else {
      break;
    }
    exit(0); // success exit
  } while (0);

  // error cases
  fprintf(2, "error\n");
  close(ptc[RD]);
  close(ptc[WR]);
  close(ctp[RD]);
  close(ctp[WR]);
  exit(1);
}
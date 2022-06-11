#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAXSZ 512

enum state {
  S_WAIT,         // the initial state, or the current is a space
  S_ARG,          // the current char is a arg
  S_ARG_END,      // the end of the arg
  S_ARG_LINE_END, // new line next to the arg, "abc\n"
  S_LINE_END,     // the new line next to the space, "abc \n"
  S_END,          // end, EOF
};

enum char_type { C_SPACE, C_CHAR, C_LINE_END };

enum char_type get_char_type(char c) {
  switch (c) {
  case ' ':
    return C_SPACE;
  case '\n':
    return C_LINE_END;
  default:
    return C_CHAR;
  }
}

enum state transform_state(enum state cur, enum char_type ct) {
  switch (cur) {
  case S_WAIT:
    if (ct == C_SPACE)
      return S_WAIT;
    if (ct == C_LINE_END)
      return S_LINE_END;
    if (ct == C_CHAR)
      return S_ARG;
    break;
  case S_ARG:
    if (ct == C_SPACE)
      return S_ARG_END;
    if (ct == C_LINE_END)
      return S_ARG_LINE_END;
    if (ct == C_CHAR)
      return S_ARG;
    break;
  case S_ARG_END:
  case S_ARG_LINE_END:
  case S_LINE_END:
    if (ct == C_SPACE)
      return S_WAIT;
    if (ct == C_LINE_END)
      return S_LINE_END;
    if (ct == C_CHAR)
      return S_ARG;
    break;
  default:
    break;
  }
  return S_END;
}

void clearArgv(char *x_argv[MAXARG], int beg) {
  for (int i = beg; i < MAXARG; ++i)
    x_argv[i] = 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("xargs: minium amount of args is 2 !\n");
    exit(1);
  } else if (argc - 1 >= MAXARG) {
    printf("xargs: maxium amount of args is %d !\n", MAXARG);
    exit(1);
  }

  char lines[MAXSZ];
  char *p = lines;
  char *x_argv[MAXARG] = {0}; // used to store the args

  // record all the args
  // start 1 for skip the first space
  for (int i = 1; i < argc; i++) {
    x_argv[i - 1] = argv[i];
  }

  int start = 0;
  int end = 0;
  int current = argc - 1;

  enum state st = S_WAIT; // the initial state

  while (st != S_END) {
    if (read(0, p, sizeof(char)) != sizeof(char)) {
      st = S_END;
    } else {
      st = transform_state(st, get_char_type(*p));
    }

    if (++end >= MAXSZ) {
      printf("xargs: arguments too long.\n");
      exit(1);
    }

    switch (st) {
    case S_WAIT:
      ++start;
      break;
    case S_ARG_END:
      x_argv[current++] = &lines[start];
      start = end;
      *p = '\0';
      break;
    case S_ARG_LINE_END:
      x_argv[current++] = &lines[start];
    case S_LINE_END:
      start = end;
      *p = '\0';
      if (fork() == 0) {
        exec(argv[1], x_argv);
      }
      current = argc - 1;
      clearArgv(x_argv, current);
      wait(0);
      break;
    default:
      break;
    }

    ++p;
  }
  exit(0);
}
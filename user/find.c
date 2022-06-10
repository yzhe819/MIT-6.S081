#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *fileName) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory !\n", path);
    close(fd);
    return;
  }

  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
    printf("find: path too long\n");
    return;
  }

  // add the '/' to the path end
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {

    if (de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ); // copy the name to the buffer
    p[DIRSIZ] = 0;               // add the null terminator
    if (stat(buf, &st) < 0) {
      printf("find: cannot stat %s\n", buf);
      continue;
    }

    if (strcmp(de.name, fileName) == 0) {
      printf("%s\n", buf);
    } else if (st.type == T_DIR && strcmp(p, ".") != 0 &&
               strcmp(p, "..") != 0) {
      // do not recurse into the current directory or the parent directory
      find(buf, fileName);
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc == 3) {
    find(argv[1], argv[2]);
  } else {
    printf("usage: fing <PATH> <FILENAME>\n");
  }
  exit(0);
}
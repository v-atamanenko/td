#include <dirent.h>
#include <sys/unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/fcntl.h>

#include <cstdlib>
#include <ctype.h>
#include <string.h>
#include <cstdio>
#include <sys/errno.h>

//#define fcntl(A,B,C) 0

int usleep(unsigned useconds) {
  return sceKernelDelayThread(useconds);
}

int utimes (const char *, const struct timeval [2]) {
  fprintf(stderr, "[WARNING] utimes used\n");
  return -1;
}

int fchown (int __fildes, uid_t __owner, gid_t __group){
  fprintf(stderr, "[WARNING] fchown used\n");
  return -1;
}

uid_t geteuid (void){
  fprintf(stderr, "[WARNING] geteuid used\n");
  return -1;
}

ssize_t readlink (const char *__restrict __path, char *__restrict __buf, size_t __buflen){
  fprintf(stderr, "[WARNING] readlink used\n");
  return -1;
}

int dup2 (int __fildes, int __fildes2) {
  fprintf(stderr, "[WARNING] dup2 used\n");
  return -1;
}

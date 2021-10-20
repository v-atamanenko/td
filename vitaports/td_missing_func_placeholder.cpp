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

#define fcntl(A,B,C) 0

int fsync (int __fd) {
    if (sceIoSyncByFd(__fd, 0) == 0) {
        return -1;
    } else {
        return 0;
    }
}

char * realpath (const char *__restrict path, char *__restrict resolved_path) {
  strcpy(resolved_path, path);
  return resolved_path;
}

int usleep(unsigned useconds) {
    return sceKernelDelayThread(useconds);
}

int chdir (const char *__path) {
 fprintf(stderr, "warn:chdir");
  return -1;
}

DIR *fdopendir(int) {
 fprintf(stderr, "warn:fdopendir");
  return nullptr;
}

int utimes (const char *, const struct timeval [2]) {
 fprintf(stderr, "warn:utimes");
  return -1;
}

char * getcwd (char *__buf, size_t size) {
 fprintf(stderr, "warn:getcwd");
  return nullptr;
}

int fchmod (int __fildes, mode_t __mode) {
 fprintf(stderr, "warn:fchmod");
  return -1;
}

int fchown (int __fildes, uid_t __owner, gid_t __group){
 fprintf(stderr, "warn:fchown");
  return -1;
}

uid_t geteuid (void){
 fprintf(stderr, "warn:geteuid");
  return -1;
}

ssize_t readlink (const char *__restrict __path, char *__restrict __buf, size_t __buflen){
 fprintf(stderr, "warn:readlink");
  return -1;
}

int lstat (const char *__restrict __path, struct stat *__restrict __buf ){
 fprintf(stderr, "warn:lstat");
  return -1;
}

int dup2 (int __fildes, int __fildes2) {
 fprintf(stderr, "warn:dup2");
  return -1;
}

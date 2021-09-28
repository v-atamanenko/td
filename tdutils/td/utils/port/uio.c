// https://github.com/lattera/glibc/blob/master/sysdeps/posix/readv.c
// https://github.com/lattera/glibc/blob/master/sysdeps/posix/writev.c


#if defined(__cplusplus)
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <errno.h>

#ifndef	SSIZE_MAX
/* ssize_t is not formally required to be the signed type
   corresponding to size_t, but it is for all configurations supported
   by glibc.  */
# if __WORDSIZE == 64 || __WORDSIZE32_SIZE_ULONG
#  define SSIZE_MAX	LONG_MAX
# else
#  define SSIZE_MAX	INT_MAX
# endif
#endif

static void
ifree (char **ptrp)
{
  free (*ptrp);
}

/* Write data pointed by the buffers described by VECTOR, which
   is a vector of COUNT 'struct iovec's, to file descriptor FD.
   The data is written in the order specified.
   Operates just like 'write' (see <unistd.h>) except that the data
   are taken from VECTOR instead of a contiguous buffer.  */
ssize_t
writev (int fd, const struct iovec *vector, int count)
{
  /* Find the total number of bytes to be written.  */
  size_t bytes = 0;
  for (int i = 0; i < count; ++i)
  {
    /* Check for ssize_t overflow.  */
    if (SSIZE_MAX - bytes < vector[i].iov_len)
    {
      return -1;
    }
    bytes += vector[i].iov_len;
  }

  /* Allocate a temporary buffer to hold the data.  We should normally
     use alloca since it's faster and does not require synchronization
     with other threads.  But we cannot if the amount of memory
     required is too large.  */
  char *buffer;
  char *malloced_buffer __attribute__ ((__cleanup__ (ifree))) = NULL;
  /*if (__libc_use_alloca (bytes))
    buffer = (char *) __alloca (bytes);
  else
  {*/
    malloced_buffer = buffer = (char *) malloc (bytes);
    if (buffer == NULL)
      /* XXX I don't know whether it is acceptable to try writing the data in chunks.  Probably not so we just fail here.  */
      return -1;
  /*}*/

  /* Copy the data into BUFFER.  */
  size_t to_copy = bytes;
  char *bp = buffer;
  for (int i = 0; i < count; ++i)
  {
    size_t copy = MIN (vector[i].iov_len, to_copy);

    bp = mempcpy ((void *) bp, (void *) vector[i].iov_base, copy);

    to_copy -= copy;
    if (to_copy == 0)
      break;
  }

  ssize_t bytes_written = write (fd, buffer, bytes);

  return bytes_written;
}

/* Read data from file descriptor FD, and put the result in the
   buffers described by VECTOR, which is a vector of COUNT 'struct iovec's.
   The buffers are filled in the order specified.
   Operates just like 'read' (see <unistd.h>) except that data are
   put in VECTOR instead of a contiguous buffer.  */
ssize_t
readv (int fd, const struct iovec *vector, int count)
{
  /* Find the total number of bytes to be read.  */
  size_t bytes = 0;
  for (int i = 0; i < count; ++i)
  {
    /* Check for ssize_t overflow.  */
    if (SSIZE_MAX - bytes < vector[i].iov_len)
    {
      return -1;
    }
    bytes += vector[i].iov_len;
  }

  /* Allocate a temporary buffer to hold the data.  We should normally
     use alloca since it's faster and does not require synchronization
     with other threads.  But we cannot if the amount of memory
     required is too large.  */
  char *buffer;
  char *malloced_buffer __attribute__ ((__cleanup__ (ifree))) = NULL;
  /*if (__libc_use_alloca (bytes))
    buffer = (char *) __alloca (bytes);
  else
  {*/
    malloced_buffer = buffer = (char *) malloc (bytes);
    if (buffer == NULL)
      return -1;
  /*}*/

  /* Read the data.  */
  ssize_t bytes_read = read (fd, buffer, bytes);
  if (bytes_read < 0)
    return -1;

  /* Copy the data from BUFFER into the memory specified by VECTOR.  */
  bytes = bytes_read;
  for (int i = 0; i < count; ++i)
  {
    size_t copy = MIN (vector[i].iov_len, bytes);

    (void) memcpy ((void *) vector[i].iov_base, (void *) buffer, copy);

    buffer += copy;
    bytes -= copy;
    if (bytes == 0)
      break;
  }

  return bytes_read;
}

#if defined(__cplusplus)
}  /* extern "C" */
#endif

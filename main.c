// By Penguin, 2014.12
// This test is used to study MM

#define ONE_K    0x400
#define ONE_M    0x100000
#define ONE_G    0x40000000

////////////
// INCLUDES
////////////
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

////////////
// PROTOTYPES
////////////
void printBuf(void* buf, int size);

////////////
// LOCALS
////////////


////////////
// FUNCTION
////////////
int main(int agrc, char* argv[])
{
  int      Mem_fd;
  int      lasterror;
  size_t   count;
  char     buf[10];
  ssize_t  retSize;

  printf("This is Penguin's mm\n");

  // Try to open /dev/mem
  Mem_fd = open("/dev/mem", O_RDWR);
  if (Mem_fd < 0)
  {
    lasterror = errno;
    printf("Open /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  // Try to read directly
  lseek(Mem_fd, 0, SEEK_SET);
  count = 10;
  retSize = read(Mem_fd, buf, count);
  if (retSize < 0)
  {
    lasterror = errno;
    printf("read /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  printBuf(buf, count);

  // Close /dev/mem
  if (close(Mem_fd) < 0 )
  {
    lasterror = errno;
    printf("clos /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  return 0;
}

void printBuf(void* buf, int size)
{
  int   i;
  char* ptr;

  ptr = buf;
  for(i = 0; i < size; i++)
  {
    printf("%x ", ptr[i]);
  }
  printf("\n");
}

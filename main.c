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
#include <sys/mman.h>

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
  int      mem_fd;
  int      lasterror;
  size_t   count;
  char     buf[10];
  ssize_t  retSize;

  printf("This is Penguin's mm\n");

  // Try to open /dev/mem
  mem_fd = open("/dev/mem", O_RDWR);
  if (mem_fd < 0)
  {
    lasterror = errno;
    printf("Open /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  // Try to read directly
  // Read the first 10 bytes
  lseek(mem_fd, 0, SEEK_SET);
  count = 10;
  retSize = read(mem_fd, buf, count);
  if (retSize < 0)
  {
    lasterror = errno;
    printf("read /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  printBuf(buf, count);

  // Try to read directly
  // Read 10 bytes above 1M
  lseek(mem_fd, ONE_M, SEEK_SET);
  count = 10;
  retSize = read(mem_fd, buf, count);
  if (retSize < 0)
  {
    lasterror = errno;
    printf("read /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    //return -1;
  }

  // Try to read it by mmap
  // Read 10 bytes above 1M
  int *mappedAddr;
  int addr = 0;
  mappedAddr = mmap(&addr, 10, PROT_READ, MAP_SHARED, mem_fd, 0);
  if (retSize < 0)
  {
    lasterror = errno;
    printf("map 10 bytes of /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  munmap(&addr, 10);

  // Close /dev/mem
  if (close(mem_fd) < 0 )
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

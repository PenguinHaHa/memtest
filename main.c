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
#include <stdlib.h>

////////////
// PROTOTYPES
////////////
void printBuf(void* buf, int size);
void directRead(int fd, int phyAddr, int count);
void mmapRead(int fd, int phyAddr, int count);

////////////
// LOCALS
////////////
int      lasterror;


////////////
// FUNCTION
////////////
int main(int agrc, char* argv[])
{
  int      mem_fd;

  printf("This is Penguin's mm\n");

  // Try to open /dev/mem
  mem_fd = open("/dev/mem", O_RDWR);
  if (mem_fd == -1)
  {
    lasterror = errno;
    printf("Open /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  // Try to read directly
  directRead(mem_fd, 0, 0x10);

  // Try to mmap /dev/mem
  mmapRead(mem_fd, 0, 0x10);

  // Close /dev/mem
  if (close(mem_fd) == -1)
  {
    lasterror = errno;
    printf("close /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  return 0;

}

void mmapRead(int fd, int phyAddr, int count)
{
  char *mappedAddr;
  int pageSize;
  int *ptr;

  pageSize = getpagesize();

  // Check if Page aligned
  if (phyAddr & (pageSize - 1))
  {
    printf("phyAddr should be page aligned\n");
    return;
  }
  
  mappedAddr = (char*)mmap(NULL, count, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phyAddr);
  if (*mappedAddr == -1)
  {
    lasterror = errno;
    printf("map /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
  }
  printf("PhyAddr %x | mappedAddr %x : ", phyAddr, *mappedAddr);

  printBuf(mappedAddr, count);

  if (munmap(mappedAddr, count) == -1)
  {
    lasterror = errno;
    printf("unmap mmappedAddr %x of /dev/mem failed (%d) - %s\n", *mappedAddr, lasterror, strerror(lasterror));
  }
}

void directRead(int fd, int phyAddr, int count)
{
  char* buf;
  ssize_t  retSize;

  buf = (char*)malloc(count);
  memset(buf, 0, count);
  lseek(fd, phyAddr, SEEK_SET);
  retSize = read(fd, buf, count);
  if (retSize < 0)
  {
    lasterror = errno;
    printf("read /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
  }
  else
  {
    printBuf(buf, count);
  }
  free(buf);
}

void printBuf(void* buf, int count)
{
  int   i;
  int* ptr;

  ptr = (int*)buf;
  for(i = 0; i < count/sizeof(int); i++)
  {
    printf("%08x ", ptr[i]);
  }
  printf("\n");
}

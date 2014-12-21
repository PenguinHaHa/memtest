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
  int    MemDescriptor;
  int    lasterror;
  int   buf[10];
  size_t retSize;

  printf("This is Penguin's mm\n");

  // Try to open /dev/mem
  MemDescriptor = open("/dev/mem", O_RDWR);
  if (MemDescriptor == -1)
  {
    lasterror = errno;
    printf("Open /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  // Try to read directly
  retSize = read(MemDescriptor, buf, 10);
  if (retSize == -1)
  {
    lasterror = errno;
    printf("read /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  printBuf(buf, 10);

  return 0;
}

void printBuf(void* buf, int size)
{
  int   i;
  int* ptr;

  ptr = (int*)buf;
  for(i = 0; i < size; i++)
  {
    printf("%08x ", *ptr++);
  }
  printf("\n");
}

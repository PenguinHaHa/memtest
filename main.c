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
#include <getopt.h>

typedef enum _METHOD {
  F,   // use function read/write
  M    // use function mmap
} METHOD;

typedef enum _OPERATION {
  R,   // Read
  W,   // Write
  NONE // do nothing
} OPERATION;

typedef struct _PARAMTERS {
  METHOD    method;
  OPERATION operation;
  int    startAddr;
  int    count;
} PARAMTERS;

#ifndef DIMENSION
#define DIMENSION(x) (sizeof(x)/sizeof(x[0]))
#endif

////////////
// PROTOTYPES
////////////
void printBuf(void* buf, int size);
void fileInterface(int fd, OPERATION opera, int phyAddr, int count);
void mmapInterface(int fd, OPERATION opera, int phyAddr, int count);
void print_usage(void);
void parse_options(PARAMTERS *param, int argc, char** argv);
void e820_info(void);

////////////
// LOCALS
////////////
int  lasterror;
const char* const short_options = "hlm:o:s:c:";
const struct option long_options[] = {
  {"help", 0, NULL, 'h'},
  {"list", 0, NULL, 'l'},
  {"method", 1, NULL, 'm'},
  {"operation", 1, NULL, 'o'},
  {"startaddr", 1, NULL, 's'},
  {"count", 1, NULL, 'c'}
};

////////////
// FUNCTION
////////////
int main(int argc, char *argv[])
{
  int mem_fd;
  PARAMTERS mem_param;

  printf("This is Penguin's mm\n");

  parse_options(&mem_param, argc, argv);
  if (mem_param.operation == NONE)
    return 0;

  printf("method %d, operation %d, startaddr 0x%x, count %d\n", mem_param.method, mem_param.operation, mem_param.startAddr, mem_param.count);

  // Try to open /dev/mem
  mem_fd = open("/dev/mem", O_RDWR);
  if (mem_fd == -1)
  {
    lasterror = errno;
    printf("Open /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }

  // Use file operation interface 
  if (mem_param.method == F)
    fileInterface(mem_fd, mem_param.operation, mem_param.startAddr, mem_param.count);

  // Try to mmap /dev/mem
  if (mem_param.method == M)
    mmapInterface(mem_fd, mem_param.operation, mem_param.startAddr, mem_param.count);

  // Close /dev/mem
  if (close(mem_fd) == -1)
  {
    lasterror = errno;
    printf("close /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    return -1;
  }
  return 0;

}

void print_usage(void)
{
  printf("This is Penguin's mm\n");
  printf("  -h  --help                 Display usage information\n");
  printf("  -l  --list                 List the layout information of system address\n");
  printf("  -m  --method file/mmap     Specify how to access memory\n");
  printf("  -o  --operation read/write Specify operation\n");
  printf("  -s  --startaddr phyaddr    Specify the start memory addr\n");
  printf("  -c  --count count          Specify memory size\n");
}

void parse_options(PARAMTERS *param, int argc, char** argv)
{
  int option;
  char* opt_arg = NULL;

  // Default value
  param->method = F;
  param->operation = R;
  param->startAddr = 0;
  param->count = 0x10;
  
  if (argc == 1)
  {
    print_usage();
    param->operation = NONE;
    return;
  }

  do
  {
    option = getopt_long(argc, argv, short_options, long_options, NULL);
    switch (option)
    {
      case 'h':
        print_usage();
        param->operation = NONE;
        break;

      case 'l':
        e820_info();
        param->operation = NONE;
        break;
      
      case 'm':
        opt_arg = optarg;
        if(!strcmp(opt_arg, "file"))
        {
          param->method = F;
        }
        else if (!strcmp(opt_arg, "mmap"))
        {
          param->method = M;
        }
        else
        {
          printf("invalid parameter of method!\n");
          print_usage();
        }
        break;

      case 'o':
        opt_arg = optarg;
        if(!strcmp(opt_arg, "read"))
        {
          param->operation = R;
        }
        else if(!strcmp(opt_arg, "write"))
        {
          param->operation = W;
        }
        else
        {
          printf("invalid parameter of method!\n");
          print_usage();
        }
        break;

      case 's':
        opt_arg = optarg;
        param->startAddr = strtol(opt_arg, NULL, 0);
        break;

      case 'c':
        opt_arg = optarg;
        param->count = strtol(opt_arg, NULL, 0);
        break;

      case -1:
        break;

      default:
        printf("Unsupported option %c\n", option);
        break;
    }
  }while (option != -1);
}

void e820_info(void)
{
  unsigned long long e820_start;
  unsigned long long e820_end;
  char e820_type[256];
  unsigned int e820_index = 0;
  FILE *e820_handle;
  char szline[256];
  char szfilename[256];

  for(;;)
  {
    //Open /sys/firmware/memmap/#/start
    snprintf(szfilename, DIMENSION(szfilename), "/sys/firmware/memmap/%d/start", e820_index);
    e820_handle = fopen(szfilename, "rb");
    if (e820_handle != NULL)
    {
      fgets(szline, DIMENSION(szline), e820_handle);
      sscanf(szline, "%llX", &e820_start);
      fclose(e820_handle);
    }
    else
    {
      break;
    }

    //Open /sys/firmware/memmap/#/end
    snprintf(szfilename, DIMENSION(szfilename), "/sys/firmware/memmap/%d/end", e820_index);
    e820_handle = fopen(szfilename, "rb");
    if (e820_handle != NULL)
    {
      fgets(szline, DIMENSION(szline), e820_handle);
      sscanf(szline, "%llX", &e820_end);
      fclose(e820_handle);
    }
    else
    {
      break;
    }
    
    //Open /sys/firmware/memmap/#/type
    snprintf(szfilename, DIMENSION(szfilename), "/sys/firmware/memmap/%d/type", e820_index);
    e820_handle = fopen(szfilename, "rb");
    if (e820_handle != NULL)
    {
      fgets(e820_type, DIMENSION(szline), e820_handle);
      fclose(e820_handle);
    }
    else
    {
      break;
    }

    printf("%d: START 0x%llX, END 0x%llX, TYPE %s\n", e820_index, e820_start, e820_end, e820_type);
    e820_index++;
  }
}

void mmapInterface(int fd, OPERATION opera, int phyAddr, int count)
{
  char *mappedPtr;
  int pageSize;
  int *ptr;

  pageSize = getpagesize();

  // Check if Page aligned
  if (phyAddr & (pageSize - 1))
  {
    printf("phyAddr should be page aligned\n");
    return;
  }
  
  mappedPtr = (char*)mmap(NULL, count, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phyAddr);
  if (*mappedPtr == -1)
  {
    lasterror = errno;
    printf("map /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
  }

  if (opera == R)
  {
    printf("PhyAddr %x | mappedAddr %x : ", phyAddr, *mappedPtr);
    printBuf(mappedPtr, count);
  }
  else if (opera == W)
  {
    // all set 0x12
    int i;
    char *ptr;
    ptr = mappedPtr;
    for (i = 0; i < count; i++)
    {
      *ptr++ = 0x12;
    }
  }
  else
  {
    printf("Invalid operation\n");
  }

  if (munmap(mappedPtr, count) == -1)
  {
    lasterror = errno;
    printf("unmap mmappedAddr %x of /dev/mem failed (%d) - %s\n", *mappedPtr, lasterror, strerror(lasterror));
  }
}

void fileInterface(int fd, OPERATION opera, int phyAddr, int count)
{
  char* buf;
  ssize_t  retSize;

  buf = (char*)malloc(count);
  memset(buf, 0, count);
  lseek(fd, phyAddr, SEEK_SET);
  
  if (opera == R)
  {
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
  }
  else if (opera == W)
  {
    int i;
    char *ptr;
    ptr = buf;
    for (i = 0; i < count; i++)
    {
      *ptr++ = 0x12;
    }

    retSize = write(fd, buf, count);
    if (retSize < 0)
    {
      lasterror = errno;
      printf("write /dev/mem failed (%d) - %s\n", lasterror, strerror(lasterror));
    }
  }
  else
  {
    printf("Invalid operation\n");
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

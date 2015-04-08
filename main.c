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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

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

typedef union _SPD_DATA_
{
   unsigned char RawData[256];
   
   struct _COMMON_
   {
      unsigned char Size;
      unsigned char Revision;
      unsigned char DimmType;
   } Common;

   struct _DDR_SPD_DATA_
   {
      unsigned char Size;              // Byte 0      - Number of Serial PD Bytes Written/SPD Device Size/CRC Coverage
      unsigned char Revision;          // Byte 1      - SPD Revision
      unsigned char DimmType;          // Byte 2      - DRAM Device Type
      unsigned char NumRows;           // Byte 3      - Number of Row Addresses on this assembly
      unsigned char NumColumns;        // Byte 4      - Number of Column Addresses on this assembly
      unsigned char NumBanks;          // Byte 5      - Number of Physical Banks on the Dimm
      unsigned char Width;             // Byte 6      - Module Width of this assembly
      unsigned char ModuleOrg;         // Byte 7      - Module Organization
      unsigned char BusWidth;          // Byte 8      - Module Memory Bus Width
      unsigned char CycleTime;         // Byte 9      - SDRAM Device Cycle time at Max Supported CAS Latency
      unsigned char tAC;               // Byte 10     - SDRAM Device Access from Clock (tAC)
      unsigned char ConfigType;        // Byte 11     - DIMM Configuration Type (non-ECC = 00h, ECC = 02h)
      unsigned char RefreshRate;       // Byte 12     - Refresh Rate/Type
      unsigned char Pad13to17[5];      // Byte 13-17 
      unsigned char CasLatency;        // Byte 18     - CAS Latency (CL)
      unsigned char Pad19to20[2];      // Byte 19-20
      unsigned char Attributes;        // Byte 21     - SDRAM Module Attributes
      unsigned char Pad22to30[9];      // Byte 22-30
      unsigned char Density;           // Byte 31     - Module Bank Density
      unsigned char Pad32to62[31];     // Byte 32-62
      unsigned char Checksum;          // Byte 63     - Checksum for byte 0 to 62
      unsigned char JedecId[8];        // Byte 64-71  - JEDEC ID Code
      unsigned char Location;          // Byte 72     - Manufacture Location
      unsigned char PartNumber[18];    // Byte 73-90  - Part Number Code 
      unsigned char RevisionCode[2];   // Byte 91-92
      unsigned char ManufactureYear;   // Byte 93     - Manufacturing Date (year)
      unsigned char ManufactureWeek;   // Byte 94     - Manufacturing Date (week: 01h-34h)
      unsigned char SerialNumber[4];   // Byte 95-98  - Module Serial Number
      unsigned char Pad99to255[157];   // Byte 99-255
      
   } DdrData;
   
   struct _DDR3_SPD_DATA_
   {
      unsigned char Size;              // Byte 0      - Number of Serial PD Bytes Written/SPD Device Size/CRC Coverage
      unsigned char Revision;          // Byte 1      - SPD Revision
      unsigned char DimmType;          // Byte 2      - DRAM Device Type
      unsigned char Pad3;              // Byte 3
      unsigned char Density;           // Byte 4      - SDRAM Density and Banks
      unsigned char Pad5to6[2];        // Byte 5-6
      unsigned char ModuleOrg;         // Byte 7      - Module Organization
      unsigned char BusWidth;          // Byte 8      - Module Memory Bus Width
      unsigned char Pad9;              // Byte 9
      unsigned char MtbDividend;       // Byte 10     - Medium Timebase (MTB) Dividend
      unsigned char MtbDivisor;        // Byte 11     - Medium Timebase (MTB) Divisor
      unsigned char tCKCycleTime;      // Byte 12     - Minimum SDRAM Cycle Time (tCK min)
      unsigned char Pad13;             // Byte 13
      unsigned char CasLatencyLowByte; // Byte 14     - CAS Latencies Supported, Low Byte
      unsigned char CasLatencyHiByte;  // Byte 15     - CAS Latencies Supported, Hi Byte
      unsigned char Pad16to116[101];   // Byte 16-116 
      unsigned char JedecIdLSB;        // Byte 117    - Module Manufacturer ID Code LSB
      unsigned char JedecIdMSB;        // Byte 118    - Module Manufacturer ID Code MSB
      unsigned char Location;          // Byte 119    - Manufacture Location
      unsigned char ManufactureYear;   // Byte 120    - Manufacturing Date (year)
      unsigned char ManufactureWeek;   // Byte 121    - Manufacturing Date (week: 01h-34h)
      unsigned char SerialNumber[4];   // Byte 122-125- Module Serial Number
      unsigned char CrcValueLSB;       // Byte 126    - SPD CRC Value
      unsigned char CrcValueMSB;       // Byte 127    - SPD CRC Value MSB
      unsigned char PartNumber[18];    // Byte 128-145- Module Part Number
      unsigned char Pad146to256[110];     // Byte 146-255 
   
   } Ddr3Data;

} SPD_DATA;

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
void parseSpdData(char *filename, int addr, SPD_DATA *spdData);
void spd_info(void);
int spdReadData(int file, unsigned char *spdRawData);
int spdReadByte(int file, int reg, unsigned char *byteData);

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
        spd_info();
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

void spd_info(void)
{
  char szFilename[20];
  int  adapterNumber = 0;
  int  file;
  int  adapter;
  int  addr;
  unsigned char data;

  SPD_DATA spdData;
  
  for(adapter = 0; adapter < 0x20; adapter++)
  {
    snprintf(szFilename, 29, "/dev/i2c-%d", adapterNumber);
    file = open(szFilename, O_RDWR);
    if (file < 0)
    {
      lasterror = errno;
 //     printf("ERROR, open %s, %s: line %d, (%d) - %s\n", szFilename, __func__, __LINE__, lasterror, strerror(lasterror));
      adapterNumber++;
      continue;
    }

    for (addr = 0x50; addr < 0x58; addr++)
    {
      if (ioctl(file, I2C_SLAVE, addr) != 0)
        continue;

      if (spdReadByte(file, 0, &data) == 0)
      {
        memset(spdData.RawData, 0, 256);
        if (spdReadData(file, spdData.RawData) != -1)
          parseSpdData(szFilename, addr, &spdData);
      }
    }

    close(file);
    adapterNumber++;
  }
}

int spdReadData(int file, unsigned char *spdRawData)
{
  int reg;
  union     i2c_smbus_data data;
  struct    i2c_smbus_ioctl_data smbusArgs;

  for (reg = 0; reg < 256; reg++)
  {
    if (spdReadByte(file, reg, &spdRawData[reg]) == -1)
      return -1;
  }

  return 0;
}

int spdReadByte(int file, int reg, unsigned char *byteData)
{
  union     i2c_smbus_data data;
  struct    i2c_smbus_ioctl_data smbusArgs;

  smbusArgs.command    = reg;
  smbusArgs.read_write = I2C_SMBUS_READ;
  smbusArgs.data       = &data;
  smbusArgs.size       = I2C_SMBUS_BYTE_DATA;
  if (ioctl(file, I2C_SMBUS, &smbusArgs) < 0)
  {
//    lasterror = errno;
//    printf("ERROR read reg %d, %s: line %d, (%d) - %s\n", reg, __func__, __LINE__, lasterror, strerror(lasterror));
    return -1;
  }
  *byteData = data.byte;
  return 0;
}

void parseSpdData(char *filename, int addr, SPD_DATA *spdData)
{
  int i;
  printf("%s addr 0x%02x:", filename, addr << 1);
  for (i = 0; i < 256; i++)
    printf("0x%02x ", spdData->RawData[i]);

  printf("\n");

//  printf("Size 0x%02x, Revision 0x%02x, DimmType 0x%02x\n", spdData->Common.Size, spdData->Common.Revision, spdData->Common.DimmType);
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

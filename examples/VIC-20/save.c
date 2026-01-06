#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint8_t *mem;
static uint8_t *interrupt;
static uint16_t base_address;

char filename[16];
uint16_t return_address;

void savefile(char *filename, uint16_t start, uint16_t end) 
{
    int fd;
    char buffer[256];

    fd=open(filename,O_CREAT | O_WRONLY);

    if (fd > 0) {
        buffer[0]=start&0xff;
        buffer[1]=start>>8;
        write(fd,buffer,2);

        uint16_t addr=start;

        while (addr < end) {
            buffer[0]=mem[addr++];
            write(fd,buffer,1);
        }
        close(fd);

        return_address=0xF728;
    } else {
        printf("Could not open file: %s\r\n",filename);
        return_address=0xF793;
    }
}

void save_trap()
{
    printf("SAVE TRAP\r\n");
 //   printf("DEV: %i\r\n",mem[0xBA]);
 //   printf("NLENGTH: %i\r\n",mem[0xB7]);

    memcpy(filename,&mem[mem[0xBC]*256 + mem[0xBB]],mem[0xB7]);
    filename[mem[0xB7]]=0;

    printf("FILE: %s\r\n",filename);

    uint16_t start = mem[0xc1] + mem[0xC2]*256;
    uint16_t end = mem[0xAE] + mem[0xAF]*256;

    printf("Save from 0x%04X to 0x%04X\r\n",start,end);
   savefile(filename,start,end);
}

// Basic info
const char* name()
{
    return "save trap";
}

void mon_init(uint16_t base_addr, void *mon_mem, uint8_t *mon_interrupt)
{
  // Do all the fancy stuff here, like start threads, connect to hardware and so on.
  mem = mon_mem;
  interrupt = mon_interrupt;
  base_address = base_addr;
}

uint8_t mon_trap_length()
{
  return 3;
}

int trap_active = 0;

// The real IO
//
// mem read
uint8_t mon_read(uint16_t address)
{
    if (address == base_address) {
        trap_active = 1;
        save_trap();
        return 0x4C; //JMP
    }
    else if (trap_active && address == base_address+1) {
        trap_active = 1;
        return return_address & 0xff;
    }
    else if (trap_active && address == base_address+2) {
        trap_active = 0;
        return return_address >> 8;
    }

  return mem[address];
}

// mem write
void mon_write(uint16_t address, uint8_t data)
{
  mem[address] = data;
}

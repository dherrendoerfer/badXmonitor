#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint8_t *mem;
static uint8_t *interrupt;
static uint16_t base_address;

char filename[40];
uint16_t return_address;

void loadfile(char *filename) 
{
    int fd;
    char buffer[256];

    fd=open(filename,O_RDONLY);

    if (fd > 0) {
        read(fd,buffer,2);

        uint16_t addr = buffer[1] * 256 + buffer[0];

        while ((read(fd,buffer,1))>0) {
            mem[addr++]=buffer[0];
        }

    	mem[0xAF] = (addr) & 0xff;
    	mem[0xAE] = (addr) / 256;
        close(fd);

        return_address=0xF66A;
    } else {
        printf("Could not open file: %s\r\n",filename);
        return_address=0xF787;
    }
}

void load_trap()
{
    #ifdef DEBUG
    printf("LOAD TRAP\r\n");
    printf("DEV: %i\r\n",mem[0xBA]);
    printf("NLENGTH: %i\r\n",mem[0xB7]);
    #endif

    char buffer[32];

    memcpy(buffer, &mem[mem[0xBC]*256 + mem[0xBB]],mem[0xB7]);
    buffer[mem[0xB7]]=0;

    snprintf(filename, 32, "%s.prg", buffer);

    for (int i=0; filename[i]!=0;i++)
        filename[i]=tolower(filename[i]);

    #ifdef DEBUG
    printf("FILE: %s\r\n",filename);
    #endif

    loadfile(filename);
}

// Basic info
const char* name()
{
    return "load trap";
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
        load_trap();
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

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

static uint8_t *mem;
static uint8_t *interrupt;
static uint16_t base_address;

// Basic info
const char* name()
{
    return "hardware prototype";
}

void mon_init(uint16_t base_addr, void *mon_mem, uint8_t *mon_interrupt)
{
  // Do all the fancy stuff here, like start threads, connect to hardware and so on.
  mem = mon_mem;
  interrupt = mon_interrupt;
  base_address = base_addr;
}

int mon_tick()
{
  return 0;
}

int mon_banks()
{
  return 1;
}

// The real IO
//
// mem read
uint8_t mon_read(uint16_t address)
{
  return mem[address];
}

// mem write
void mon_write(uint16_t address, uint8_t data)
{
  mem[address] = data;
}

uint8_t mon_do_tick(uint8_t ticks)
{
  return *interrupt;
}
/*  bad6502 A Raspberry Pi-based backend to a 65C02 CPU
    Copyright (C) 2025  D.Herrendoerfer
*/

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>

#include "bad6502.h"

#define IS_RAM(x)((x & 0xFC) == 0)
#define IS_ROM_P0(x) (x & 0x04)
#define IS_IO(x) (x & 0x20)
#define IS_TRAP(x) (x & 0x40)

#define SET_ROM_P0(x) (x |= 0x04)
#define SET_IO(x) (x |= 0x20)
#define SET_TRAP(x) (x |= 0x40)

// the ram
uint8_t mem[0x10000];
uint8_t mem_desc[0x10000];

void *io_map_read[0x1000];
void *io_map_write[0x1000];

void *trap_map_read[0x10000];
void *trap_map_write[0x10000];

uint8_t interrupt = 0;

// plugins
void *plugins[32]; 
void *plugins_tick[32]; 
uint8_t plugin_num = 0;
uint8_t plugin_tick_num = 0;

// input buffer
char data_buffer[256];

// input related globals
int use_stdin=1;
int infile=0;

// terminal settings
struct termios previous_termios;
int old_kbdmode;

extern uint32_t mmio_peri_base;
#define ST_BASE                  (mmio_peri_base + 0x003000) /* Timer */
uint32_t *st;
volatile uint64_t *timer;
uint64_t timer_now;

void setup_timer()
{
  int mem_fd;
  void *st_map;

   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   st_map = mmap(
      NULL, 
      4096,
      PROT_READ|PROT_WRITE,
      MAP_SHARED,
      mem_fd,
      ST_BASE
   );
   /* close fd */
   close(mem_fd); 

   if (st_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)st_map);//errno also set!
      exit(-1);
   }

  /* store map pointer */
  st = st_map;
  timer=(volatile void*)st+4;
}

#ifdef BLINKENLIGHTS
void blinkenlights_init()
{
  uint8_t blinkenprog[]={};

}
#endif

void reset_terminal_mode()
{
    printf("\r\nBYE\r\n");
    ioctl(0, KDSKBMODE, old_kbdmode);
    tcsetattr(0, TCSANOW, &previous_termios);
}

void signal_handler(__attribute__((unused)) const int signum) {
    exit(3);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &previous_termios);
    memcpy(&new_termios, &previous_termios, sizeof(new_termios));

    /* remember old keyboard mode */
 	  if (ioctl(0, KDGKBMODE, &old_kbdmode)) {
        old_kbdmode=K_XLATE; // probably on a terminal
    }

    /* register cleanup handler, and set the new terminal mode */
    atexit(&reset_terminal_mode);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        if (c == 0x18) {
          printf("\r\nCNTL+X pressed, exiting\r\n");
          exit(1);
        }
        return c;
    }
}

int getch_from_file(int file)
{
    int r;
    unsigned char c;
    if ((r = read(file, &c, sizeof(c))) < 0) {
      return r;
    } else {
      if (r == 0) {
        printf("\r\nFILE END, exiting\r\n");
        exit(1);
      }
      return c;
    }
}

int load_hw_lib(char* libname, uint16_t base_address)
{

  void *handle;
  const char* (*name)(void);
  void (*mon_init)(uint16_t, void*, uint8_t*);
  int (*mon_banks)();
  int (*mon_tick)();
  uint8_t (*mon_read)(uint16_t);
  void (*mon_write)(uint16_t, uint8_t);
  uint8_t (*mon_do_tick)(uint8_t);
  char *error;

  handle = dlopen (libname, RTLD_LAZY);
  if (!handle) {
      fputs (dlerror(), stderr);
      exit(1);
  }
  // get the plugin name
  name = dlsym(handle, "name");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }
  printf("\r\nloaded: %s at base_address: 0x%04X\r\n",name(),base_address);

  // register the plugin address and memory
  mon_init = dlsym(handle, "mon_init");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }
  (*mon_init) (base_address, mem, &interrupt);

  // get the number of mem banks to register
  mon_banks = dlsym(handle, "mon_banks");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }
  int banks_to_register=(*mon_banks)();

  // see if this plugin needs a tick()
  mon_tick = dlsym(handle, "mon_tick");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }
  int register_tick=(*mon_tick)();

  if (register_tick == 1) {
    mon_do_tick = dlsym(handle, "mon_do_tick");
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        exit(1);
    }
  }

  // get the read and write functions
  mon_read = dlsym(handle, "mon_read");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }

  // see if this plugin needs a tick()
  mon_write = dlsym(handle, "mon_write");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }

  // finally register everything
  for (int i=0; i<banks_to_register*16; i++) {
    SET_IO(mem_desc[base_address+i]);
    io_map_read[((base_address+1)>>4)] = mon_read; 
    io_map_write[((base_address+i)>>4)] = mon_write; 
  }

  if (register_tick == 1) {
    plugins_tick[plugin_tick_num]=mon_do_tick;
    plugin_tick_num++;
  }

  plugins[plugin_num++]=handle;

  return 0;
}

int load_trap_lib(char* libname, uint16_t base_address)
{
  void *handle;
  const char* (*name)(void);
  void (*mon_init)(uint16_t, void*, uint8_t*);
  uint8_t (*mon_read)(uint16_t);
  void (*mon_write)(uint16_t, uint8_t);
  uint8_t (*mon_trap_length)();
  char *error;

  handle = dlopen (libname, RTLD_LAZY);
  if (!handle) {
      fputs (dlerror(), stderr);
      exit(1);
  }
  // get the plugin name
  name = dlsym(handle, "name");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }
  printf("\r\nloaded: %s at base_address: 0x%04X\r\n",name(),base_address);

  // register the plugin address and memory
  mon_init = dlsym(handle, "mon_init");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }
  (*mon_init) (base_address, mem, &interrupt);


    // get the read and write functions
  mon_read = dlsym(handle, "mon_read");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }

  // see if this plugin needs a tick()
  mon_write = dlsym(handle, "mon_write");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }

  mon_trap_length = dlsym(handle, "mon_trap_length");
  if ((error = dlerror()) != NULL)  {
      fputs(error, stderr);
      exit(1);
  }

  for (int i=0; i<(*mon_trap_length)(); i++) {
    SET_TRAP(mem_desc[base_address+i]);
    trap_map_read[base_address+i] = mon_read; 
    trap_map_write[base_address+i] = mon_write; 
  }

  plugins[plugin_num++]=handle;

  return 0;
}

// mem read
uint8_t read6502(uint16_t address, uint8_t bank)
{  
  // Virtual hardware (reads a char from stdin)
  if (address == 0xFF01) {
    if (!kbhit())
      return 0;
    return getch();
  }

  if (IS_RAM(mem_desc[address])){
    return mem[address];
  }

  if (IS_TRAP(mem_desc[address])) {
    uint8_t (*trap_read)(uint16_t) = trap_map_read[address] ;
    return (*trap_read)(address);
  }

  if (IS_ROM_P0(mem_desc[address])) //simulated ROM 
    return mem[address];

  if (IS_IO(mem_desc[address])) {
    uint8_t (*io_read)(uint16_t) = io_map_read[address>>4] ;
    return (*io_read)(address);
  }

  return mem[address];
}

// mem write
void write6502(uint16_t address, uint8_t bank, uint8_t data)
{
  // Virtual hardware (writes char to stdout)
  if (address == 0xFF00) {
    printf("%c",(char)data);
    fsync(1);
    return;
  }

  if (IS_RAM(mem_desc[address])) {
    mem[address] = data;
    return;
  }

  if (IS_ROM_P0(mem_desc[address])){ //simulated ROM 
    // printf("X: 0x%04X ",address);
    return;
  }

  if (IS_IO(mem_desc[address])) {
    void (*io_write)(uint16_t, uint8_t) = io_map_write[address>>4];
    (*io_write)(address, data);
    return;
  }

  if (IS_TRAP(mem_desc[address])) {
    void (*trap_write)(uint16_t, uint8_t) = trap_map_write[address];
    (*trap_write)(address, data);
    return;
  }

  //printf("X");
  mem[address] = data;
}

int getLine(char *buffer)
{
  int key;
  int valid = 0;

  buffer[0]=0;

  // consume leading spaces
  if (use_stdin) {
    while ((key=getch()) == ' ');
  } else {
    while ((key=getch_from_file(infile)) == ' ');
  }

    // consume until space
  
  while (key != 13 && key != '\n') {
    buffer[valid++]=key;

    printf("%c",key);

    if (use_stdin) {
      key=getch();
    } else {
      key=getch_from_file(infile);
    }
  }

  buffer[valid] = 0; //zero termination
  return valid; 
}

int getBuffer(char *buffer)
{
  int key;
  int valid = 0;

  buffer[0]=0;

  // consume leading spaces
  if (use_stdin) {
    while ((key=getch()) == ' ');
  } else {
    while ((key=getch_from_file(infile)) == ' ');
  }

    // consume until space
  
  while (key != ' ' && key != 13 && key != '\n') {
    buffer[valid++]=key;

    printf("%c",key);

    if (use_stdin) {
      key=getch();
    } else {
      key=getch_from_file(infile);
    }
  }

  buffer[valid] = 0; //zero termination
  return valid; 
}

int getByte(uint8_t *data)
{
  int key;
  int valid = 0;

  *data=0;

  // consume 1 leading spaces, linebreaks etc.
  if (use_stdin) {
    while ((key=getch()) == ' ');
  } else {
    key=getch_from_file(infile);
    while (1) {
      if (key == ' ')
        key=getch_from_file(infile);
      if (key == ',')
        key=getch_from_file(infile);
      else if (key == '\\') {
        key=getch_from_file(infile); //skip newline
        key=getch_from_file(infile);
      } else
        break;    
    }
  }

  while (valid < 2){ 
    if ((key == 13) || (key == 0) || (key == ' ') || (key == '\n')) {
      return valid;
    } else {
      if (data)
        *data = *data << 4;
      if ( key >= '0' && key <= '9')
        *data += key - '0';
      else if ( key >= 'a' && key <= 'f')
        *data += key - 'a' + 10;
      else if ( key >= 'A' && key <= 'F')
        *data += key - 'A' + 10;
      else if ( key == 'x' && valid == 1) {
        valid = -1;
      } else 
        return -1;

      printf("%c",key);
    
      if ( key == 'x')
        key = '0';

      valid++;
      if (valid == 2)
        return valid;    

      //get the next key/char
      if (use_stdin) {
        key=getch();
      } else {
        key=getch_from_file(infile);
      }
    }
  }
  return valid;
}

int getDouble(uint16_t *address)
{
  int key;
  int valid = 0;

  *address=0;

  // consume 1 leading spaces
  if (use_stdin) {
    while ((key=getch()) == ' ');
  } else {
    while ((key=getch_from_file(infile)) == ' ');
  }

  while (valid < 4){ 
    if ((key == 13) || (key == 0) || (key == ' ') || (key == '\n')) {
      return valid;
    } else {
      if (address)
        *address = *address << 4;
      if ( key >= '0' && key <= '9')
        *address += key - '0';
      else if ( key >= 'a' && key <= 'f')
        *address += key - 'a' + 10;
      else if ( key >= 'A' && key <= 'F')
        *address += key - 'A' + 10;
      else 
        return -1;

      printf("%c",key);

      valid++;
      if (valid == 4)
        return valid;    

      //get the next key/char
      if (use_stdin) {
        key=getch();
      } else {
        key=getch_from_file(infile);
      }
    }
  }
  return valid;
}

// Main prog
int main(int argc, char **argv)
{
  if(argc == 2){
    use_stdin = 0;
    infile=open(argv[1], O_RDONLY);
    if (infile < 0)
      exit(1);
    // Consume the 1st line of the script
    while (getch_from_file(infile) != '\n');
  }

  // setup terminal
  set_conio_terminal_mode();
  setbuf(stdout, NULL);
  
  bzero(mem,0x10000);

  int line=1;
  
  uint8_t int_before_step = interrupt;

  while (1==1) {
    int i=0;
    data_buffer[0]=0;
    line++;

    // THIS IS WHERE A LINE STARTS (always !)

    if(use_stdin)
      i=getch();
    else
      i=getch_from_file(infile);

    if ( i != 0) {
        printf("%c",i);
    }

    if (i=='#'){
      printf("### ");
      if (use_stdin) {
        while ((i=getch()) != 13){
          printf("%c",i);
        }
      } else {
        while ((i=getch_from_file(infile)) != '\n') {
          printf("%c",i);
        }
      }
      printf("\r\n");
    }

    // EXAMINE
    if (i=='e'){
      printf(" ");

      //get the address
      uint16_t address = 0;
      int i;
      int err=0;
      
      if ((err=getDouble(&address)) > 0){
        printf("\r\n--> 0x%04X  0x%02X",address, read6502(address,0));
      }
      else {
        printf(" ????\r\n");
        if (!use_stdin) {
          printf("\r\nERROR in line %i\r\n",line);
          exit(1);
        }
      }

      if (err > 0) {
        if (use_stdin) {
          i=getch();
        } else {
          i=getch_from_file(infile);
        }

        int count=0;
        while (i == ' ' || i == '.') {
          address++;
          if ((count++)==15) {
            printf("\r\n--> 0x%04X  0x%02X",address, read6502(address,0));
            count=0;
          } else {
            printf(" 0x%02X",read6502(address,0));
          }

          if (use_stdin) {
            i=getch();
          } else {
            i=getch_from_file(infile);
          }
        }
        printf("\r\n");
      }
    }

    // DEPOSIT
    if (i=='d' || i=='f'){
      printf(" ");

      int mark_rom = 0;
      if (i=='f')
        mark_rom=1;

      //get the address
      uint16_t address = 0;
      uint8_t data = 0;
      int err=0;
      
      if ((err=getDouble(&address)) > 0){
        printf("\r\n<-- 0x%04X ",address);
      }
      else {        
        if (!use_stdin) {
          printf("\r\nERROR in line %i\r\n",line);
          exit(1);
        }
      }

      if (err > 0) {
        int count=0;
        
        while ((err=getByte(&data))>0){
          if((count++) == 15) {
            printf("\r\n<-- 0x%04X ",address);
            count=0;
          } else {
            printf(" ");
          }
          write6502(address, 0, data);
          if (mark_rom) 
            SET_ROM_P0(mem_desc[address]);
          address++;
        }
        if (err<0){
          printf(" ????\r\n");
          if (!use_stdin) {
            printf("\r\nERROR in line %i\r\n",line);
            exit(1);
          }
        }
        printf("\r\n");
      }
    }

    // PROTECT
    if (i=='p'){
      printf(" ");

      //get the address
      uint16_t address = 0;
      int err=0;
      
      if ((err=getDouble(&address)) > 0){
        printf(" PROT PAGE 0x%02X\r\n",address>>8);
        for (int i=0;i<0xff;i++)
          SET_ROM_P0(mem_desc[(address & 0xff00)+i]);
      } else {        
        printf(" ????\r\n");
        if (!use_stdin) {
          printf("\r\nERROR in line %i\r\n",line);
          exit(1);
        }
      }
    }

    // LOAD/ROM
    if (i=='l' || i=='r') {
      int mark_rom = 0;

      if (i=='r')
        mark_rom=1;

      printf(" ");

      //get the address
      uint16_t address = 0;
      int err=0;
      
      if ((err=getDouble(&address)) > 0){
        printf(" ");
        if ((err=getBuffer(data_buffer)) > 0){
          printf("\r\n<-- 0x%04X %s \r\n", address, data_buffer);
        } else {
          printf(" ????\r\n");
          if (!use_stdin) {
            printf("\r\nERROR in line %i\r\n",line);
            exit(1);
          }
        }
      } else {
        printf(" ????\r\n");
        if (!use_stdin) {
          printf("\r\nERROR in line %i\r\n",line);
          exit(1);
        }
      }

      if (err > 0) {
        // load
        int fd;
        char data;
        fd = open(data_buffer, O_RDONLY);

        while ((read(fd,&data,1))>0) {
          mem[address]=data;
          if (mark_rom)
            SET_ROM_P0(mem_desc[address]);
          address++;
        }

        printf("read until 0x%04X\r\n",(address-1));

        close(fd);
      }
    }
    // GO
    if (i=='g') {
      printf(" ");

      //wait for enter
      while (i != '\n' && i != 13) {
        if (use_stdin) {
          i=getch();
        } else {
          i=getch_from_file(infile);
        }
      }

      printf("\r\n RUNNING CPU ...\r\n");

      //reset the cpu
      reset6502(0);

      setup_timer();

      // step past the reset code
      for (int i=0 ; i<32 ; i++) {
        tick6502();
        usleep(10);
        if(bus_addr == 0xFFFD)
          break;
      }

      //uint16_t savedirq = 0;
      uint8_t cycles;
      timer_now=*timer;

loop:  
      int_before_step = interrupt;
      cycles = step6502();
//      printf("cycles:%d\r\n",cycles);
      interrupt = 0;
      // hardware ticks
      for (int i=0;i<plugin_tick_num;i++) {
        uint8_t (*mon_do_tick)(uint8_t);
        mon_do_tick = plugins_tick[i];
        interrupt |= (*mon_do_tick)(cycles);    
      }
      
      if (interrupt != int_before_step) {
//        printf("int:%i ",interrupt);
        if (interrupt) {
            irq6502(1);
        } else {
          irq6502(0);
        }
      }
      
#ifdef BLINKENLIGHTS
      usleep(10000);
#endif

      //consume time()
      while(timer_now + cycles > *timer)
        asm volatile ("nop\nnop\nnop\nnop\nnop");
      timer_now+=cycles;


/*
loop:
      int_before_step = interrupt;
      tick6502();

      // hardware ticks
      for (int i=0;i<plugin_tick_num;i++) {
        uint8_t (*mon_do_tick)(uint8_t);
        mon_do_tick = plugins_tick[i];
        interrupt |= (*mon_do_tick)(1);    
      }
      
      if (interrupt != int_before_step)
        irq6502(interrupt);

      //usleep(100000);
      #ifndef FAST
      usleep(10000);
      #endif
*/
      //if (bus_addr != 0xfffc)
        goto loop;
//^^^ loop ///

      printf("\r\n CPU RESET...\r\n");
    }
    // HARDWARE (load .so with hardware plugins)
    if (i=='h') {

      printf(" ");

      //get the address
      uint16_t address = 0;
      int err=0;
      
      if ((err=getDouble(&address)) > 0){
        printf(" ");
        if ((err=getBuffer(data_buffer)) > 0){
          printf("\r\n<-- 0x%04X %s \r\n", address, data_buffer);
        } else {
          printf(" ????\r\n");
          if (!use_stdin) {
            printf("\r\nERROR in line %i\r\n",line);
            exit(1);
          }
        }
      } else {
        printf(" ????\r\n");
        if (!use_stdin) {
          printf("\r\nERROR in line %i\r\n",line);
          exit(1);
        }
      }

      if (err > 0) {
        // load
        load_hw_lib(data_buffer,address);
      }
    }
        // TRAP (load .so with trap plugins)
    if (i=='t') {

      printf(" ");

      //get the address
      uint16_t address = 0;
      int err=0;
      
      if ((err=getDouble(&address)) > 0){
        printf(" ");
        if ((err=getBuffer(data_buffer)) > 0){
          printf("\r\n<-- 0x%04X %s \r\n", address, data_buffer);
        } else {
          printf(" ????\r\n");
          if (!use_stdin) {
            printf("\r\nERROR in line %i\r\n",line);
            exit(1);
          }
        }
      } else {
        printf(" ????\r\n");
        if (!use_stdin) {
          printf("\r\nERROR in line %i\r\n",line);
          exit(1);
        }
      }

      if (err > 0) {
        // load
        load_trap_lib(data_buffer,address);
      }
    }
    if (i=='s') {
          int err=0;
          printf(" ");

          if ((err=getLine(data_buffer)) <= 0){
            printf(" ????\r\n");
            if (!use_stdin) {
              printf("\r\nERROR in line %i\r\n",line);
              exit(1);
            }
          }

          if (err > 0) {
            // load
            printf("\r\n");
            system(data_buffer);
            printf("\r\n");
          }
        }
      }
  
}

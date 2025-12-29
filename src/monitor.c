/*  bad6502 A Raspberry Pi-based backend to a 65C02 CPU
    Copyright (C) 2025  D.Herrendoerfer
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>

#include "bad6502.h"

// the ram
uint8_t mem[0x10000];
uint8_t rom_map[256];

// input buffer
char data_buffer[256];

// input related globals
int use_stdin=1;
int infile=0;

// terminal settings
struct termios previous_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &previous_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &previous_termios);
    memcpy(&new_termios, &previous_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
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


// mem read
uint8_t read6502(uint16_t address, uint8_t bank)
{
  // Manual reset vector
//  if (address == 0xFFFC)
//    return COLD_START & 0xff; 
//  if (address == 0xFFFD)
//    return COLD_START >> 8;
  
  // Virtual hardware (reads a char from stdin)
  if (address == 0xFF01) {
    if (!kbhit())
      return 0;
    return getch();
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

  if (rom_map[address>>8] == 1) //simulated ROM 
    return;

  mem[address] = data;
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
      }
      else
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
      }
      else 
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
  
  int line=1;

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
        printf("\r\n--> 0x%04X  0x%02X",address, mem[address]);
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
            printf("\r\n--> 0x%04X  0x%02X",address, mem[address]);
            count=0;
          } else {
            printf(" 0x%02X",mem[address]);
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
          if (mark_rom)
            rom_map[address>>8]=1;
          mem[address++]=data;
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
        rom_map[address>>8]=1;
      }
      else {        
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
          if (mark_rom)
            rom_map[address>>8]=1;
          mem[address++]=data;
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

      // step past the reset code
      for (int i=0 ; i<32 ; i++) {
        step6502();
        usleep(1000);
      }

loop:  
      step6502();
      
      #ifndef FAST
      usleep(100000);
      #endif

      if (bus_addr != 0xfffc)
        goto loop;

      printf("\r\n CPU RESET...\r\n");
    }
  }
}
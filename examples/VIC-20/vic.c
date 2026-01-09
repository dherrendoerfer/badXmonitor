#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <linux/fb.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

//#define DEBUG 1
//#define DEBUGIO 1

#define VID_MEMSTART 0x1000
#define CHAR_ROMSTART 0x8000

#define CHAR_CURSOR_HIDE "\033[?25l"
#define CHAR_CURSOR_SHOW "\033[?25h"

static uint8_t *mem;
static uint8_t *interrupt;
static uint16_t base_address;

uint16_t palette[] = {0x0000,0xFFFF,0x69AD,0x752E,0x69ED,0x5C6B,0x3146,0xBE37,0x6A6D,0x41C8,0x9B33,0x4228,0x6B6D,0x9E93,0x6AED,0x94B2};

uint16_t mem_addrs[]={0x8000,0x8400,0x8800,0x8C00,0x9000,0x9400,0x9800,0x9C00,0x0000,0x0400,0x0800,0x0c00,0x1000,0x1400,0x1800,0x1C00};
uint16_t color_addrs[]={0x9400,0x9600};

//Palette transcoder
//uint32_t pal[]={0x000000,0xFFFFFF,0x68372B,0x70A4B2,0x6F3D86,0x588D43,0x352879,0xB8C76F,0x6F4F25,0x433900,0x9A6759,0x444444,0x6C6C6C,0x9AD284,0x6C5EB5,0x959595};

// internal registers expanded
uint8_t interlaced;
uint8_t h_origin = 0x05;  //NTSC PAL:0x0C
uint8_t v_origin = 0x19;  //NTSC PAL:0x26
uint8_t screen_mem_offset;
uint16_t raster_value;
uint8_t columns=23;
uint8_t rows=22;
uint8_t double_size;
uint8_t screenmem = 0xf;
uint16_t screenmem_loc = 0x1000;
uint8_t charmem = 0;
uint16_t charmem_loc = 0x8000;
uint16_t colmem_loc = 0x9400;
uint8_t aux_color;
uint8_t reverse_mode;
uint8_t screen_color;
uint8_t border_color;


struct fb_var_screeninfo screen_info;
struct fb_fix_screeninfo fixed_info;
int fbfd = -1;
char *fbbuffer = NULL;
size_t fbbuflen;

uint16_t counter = 0;
pthread_t videoThread;

volatile uint16_t current_line = 0;
uint16_t current_column = 0;

#define ntsc_screen_width 240
#define ntsc_screen_height 242 //233
//uint8_t pal_screen_width = 233;
//uint8_t pal_screen_height = 284;

#define US_PERLINE 50

static inline void _point(uint16_t x, uint16_t y, uint8_t col)
{
    uint16_t pixel = palette[col];

    x=x<<1; y=y<<1;

    uint32_t location = x*screen_info.bits_per_pixel/8 + 
                                y*fixed_info.line_length;
    *((uint16_t*) (fbbuffer + location)) = pixel;
    *((uint16_t*) (fbbuffer + location+screen_info.bits_per_pixel/8)) = pixel;
    *((uint16_t*) (fbbuffer + location+fixed_info.line_length)) = pixel;
    *((uint16_t*) (fbbuffer + location+fixed_info.line_length+screen_info.bits_per_pixel/8)) = pixel;
}

static void *display_thread(void *arg)
{
  uint16_t g,h,i,j;
  uint8_t buf, c_rom;

  // Old, monochrome, hires.
  while (0) {
    for (g=0; g<rows; g++) {
      for (h=0; h<columns; h++) {
        buf = mem[screenmem_loc+(columns*g)+h];

        for (i=0; i<8; i++) {
          c_rom=mem[charmem_loc+(8*buf)+i];
        
          for (j=0; j<8; j++) {
            if (c_rom&(1<<j)) {
              //setpixel
                _point( 70+(h*8)+8-j,10+(g*8)+i,0x00);
            }
            else {
                //clearpixel 
                _point( 70+(h*8)+8-j,10+(g*8)+i,0x01);
            }
          }
        }
      }
    }

    usleep(25000);
  }

  current_line = 0;
  current_column = 0;

  //ntsc color screen draw
  while(1) {
    uint8_t x_border = h_origin<<2;
    uint8_t y_border = v_origin;

    uint16_t h_screen_pos = 0;  //pos in text screen
    uint16_t v_screen_pos = 0;

    current_line = 0;
    current_column = 0;


    for (current_line;current_line<y_border;current_line++) {
      for (current_column=0; current_column < ntsc_screen_width; current_column++)
        _point( current_column, current_line, border_color);
      usleep(US_PERLINE);
    }
    

    if ( double_size) {
      //double sized characters (16bytes per char)
      for ( v_screen_pos = 0 ; v_screen_pos < rows; v_screen_pos++) {
        // draw 8 lines
        for (uint8_t h=0; h<16; h++) {
          //draw border
          for (current_column=0; current_column < x_border; current_column++)
            _point( current_column, current_line, border_color);

          for ( h_screen_pos=0; h_screen_pos < columns; h_screen_pos++) {
            uint8_t thischar  = mem[screenmem_loc + (v_screen_pos*columns+h_screen_pos)];
            uint8_t col_ram = mem[colmem_loc + v_screen_pos*columns+h_screen_pos];
            uint8_t c_rom = mem[charmem_loc + (thischar<<4) + h];
            
            if (col_ram & 0x08) {
              //Multicolor mode
              uint8_t pixels[]={(c_rom & 0xC0)>>6, (c_rom & 0x30)>>4, (c_rom & 0x0C)>>2, (c_rom & 0x03) };
              uint8_t pcols[]={screen_color,border_color,col_ram & 0x07,aux_color};

              _point( current_column++, current_line, pcols[pixels[0]]);
              _point( current_column++, current_line, pcols[pixels[0]]);
              _point( current_column++, current_line, pcols[pixels[1]]);
              _point( current_column++, current_line, pcols[pixels[1]]);
              _point( current_column++, current_line, pcols[pixels[2]]);
              _point( current_column++, current_line, pcols[pixels[2]]);
              _point( current_column++, current_line, pcols[pixels[3]]);
              _point( current_column++, current_line, pcols[pixels[3]]);

            } else {
              //Singlecolor
              uint8_t pcols[] = {screen_color,col_ram & 0x7};

               _point( current_column++, current_line, pcols[((c_rom&1<<7) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<6) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<5) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<4) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<3) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<2) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<1) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<0) != 0)]);
            }
          }
          for (current_column; current_column < ntsc_screen_width; current_column++)
            _point( current_column, current_line, border_color);

          usleep(US_PERLINE);
          current_line++;
        }
      }
    } else {
      //normale size chars
      for ( v_screen_pos = 0 ; v_screen_pos < rows; v_screen_pos++) {
        // draw 8 lines
        for (uint8_t h=0; h<8; h++) {
          //draw border
          for (current_column=0; current_column < x_border; current_column++)
            _point( current_column, current_line, border_color);

          for ( h_screen_pos=0; h_screen_pos < columns; h_screen_pos++) {
            uint8_t thischar  = mem[screenmem_loc + (v_screen_pos*columns+h_screen_pos)];
            uint8_t col_ram = mem[colmem_loc + v_screen_pos*columns+h_screen_pos];
            uint8_t c_rom = mem[charmem_loc + (thischar<<3) + h];
            
            if (col_ram & 0x08) {
              //Multicolor mode
              uint8_t pixels[]={(c_rom & 0xC0)>>6, (c_rom & 0x30)>>6, (c_rom & 0x0C)>>6, (c_rom & 0x03)>>6 };
              uint8_t pcols[]={screen_color,border_color,col_ram & 0x07,aux_color};

              _point( current_column++, current_line, pcols[pixels[0]]);
              _point( current_column++, current_line, pcols[pixels[0]]);
              _point( current_column++, current_line, pcols[pixels[1]]);
              _point( current_column++, current_line, pcols[pixels[1]]);
              _point( current_column++, current_line, pcols[pixels[2]]);
              _point( current_column++, current_line, pcols[pixels[2]]);
              _point( current_column++, current_line, pcols[pixels[3]]);
              _point( current_column++, current_line, pcols[pixels[3]]);

            } else {
              //Singlecolor
              uint8_t pcols[] = {screen_color,col_ram & 0x7};
              if (!reverse_mode) {
                pcols[0]=pcols[1];
                pcols[1]=screen_color;
              }

               _point( current_column++, current_line, pcols[((c_rom&1<<7) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<6) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<5) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<4) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<3) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<2) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<1) != 0)]);
               _point( current_column++, current_line, pcols[((c_rom&1<<0) != 0)]);
            }
          }

          for (current_column; current_column < ntsc_screen_width; current_column++)
            _point( current_column, current_line, border_color);

          usleep(US_PERLINE);
          current_line++;
        }
      }
    }

    for (current_line; current_line<ntsc_screen_height; current_line++) {
      for (current_column=0; current_column < ntsc_screen_width; current_column++)
        _point( current_column, current_line, border_color);
      usleep(US_PERLINE);
    }

    usleep(25000);
  }
}

int fb_init()
{
   fbfd = open("/dev/fb0", O_RDWR);
   if (fbfd >= 0)
   {
      if (!ioctl(fbfd, FBIOGET_VSCREENINFO, &screen_info) &&
          !ioctl(fbfd, FBIOGET_FSCREENINFO, &fixed_info)) {
         fbbuflen = screen_info.yres_virtual * fixed_info.line_length;
         fbbuffer = mmap(NULL,
                       fbbuflen,
                       PROT_READ|PROT_WRITE,
                       MAP_SHARED,
                       fbfd,
                       0);
         if (fbbuffer != MAP_FAILED) {
            // Try to hide the cursor on the FB
            int consolefd;
            consolefd=open("/dev/console",O_RDWR);
            if (consolefd > 0) {
              write(consolefd,CHAR_CURSOR_HIDE,6);
              close(consolefd);
            }
 
            return 0;   /* Indicate success */
         }
         else {
            fprintf(stderr,"mmap failed");
         }
      } else {
            fprintf(stderr,"ioctl failed");
        }
    } else {
        fprintf(stderr,"open failed");
    }
    if (fbbuffer && fbbuffer != MAP_FAILED)
      munmap(fbbuffer, fbbuflen);
    if (fbfd >= 0)
      close(fbfd);
    
    return 1;
}


// Basic info
const char* name()
{
    return "Commodore VIC framebuffer";
}

void mon_init(uint16_t base_addr, void *mon_mem, uint8_t *mon_interrupt)
{
  // Do all the fancy stuff here, like start threads, connect to hardware and so on.
  mem = mon_mem;
  interrupt = mon_interrupt;
  base_address = base_addr;

  if (fb_init()) {
    printf("\r\nFremebuffer init failed.!!\r\n");
    exit(1);
  }

  if (pthread_create(&videoThread, NULL, display_thread, NULL)) {
      printf("kb thread create failed\n");
      exit(1);
  }
/*
  for(int i=0;i<16;i++) {
    uint8_t r = pal[i] >> 16;
    uint8_t g = (pal[i] >> 8)&0xff;
    uint8_t b = (pal[i] >> 16)&0xff;
    
      printf("0x%04X,",((r>>3)<<11 | (g>>2)<<5 | (b>>3)));
      printf("\r\n");
  }
*/
}

int mon_tick()
{
  return 1;
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
  address &= 0x0f;
  #ifdef DEBUGIO
  printf("VIC READ REG: 0x%02X\r\n", address);
  #endif

  switch(address) {
    case 2:
            return ((screen_mem_offset<<7) | (columns & 0x7f));
    case 3:
            return(((current_line & 1) << 7) | ((rows << 1) & 0x7e) | (double_size & 1));
    case 4:
            #ifdef DEBUGIO
            printf("raster: 0x%03X\r\n", current_line);
            #endif
            return((current_line>>1) & 0xff);
    case 5:
            return (((screenmem & 0xf) << 8) | (charmem & 0xf));
  }

  return mem[address];
}


// mem write
void mon_write(uint16_t address, uint8_t data)
{
  address &= 0x0f;

  #ifdef DEBUGIO
  printf("VIC WRITE REG: 0x%02X, DATA: 0x%02X\r\n", address);
  #endif

  switch(address) {
    case 0:
            interlaced = (data & 0x80) >> 7;
            h_origin   = data & 0x7F;
            #ifdef DEBUG
            printf("interlaced: %04X\r\n",interlaced);
            printf("h_origin  : %04X\r\n",h_origin);
            #endif
            break;
    case 1: 
            v_origin = data;
            #ifdef DEBUG
            printf("v_origin  : %04X\r\n",v_origin);
            #endif
            break;
    case 2: 
            screen_mem_offset = (data & 0x80) >> 7;
            colmem_loc=color_addrs[screen_mem_offset];
            screenmem_loc=mem_addrs[screenmem] + (screen_mem_offset << 9);
            columns = data & 0x7f;
            #ifdef DEBUG
            printf("color_mem : %04X\r\n",colmem_loc);
            printf("columns   : %04X\r\n",columns);
            #endif
            break;
    case 3:
            raster_value |= (data & 0x80) >> 7;
            rows = (data & 0x7e) >> 1;
            double_size = data & 0x01;
            #ifdef DEBUG
            printf("rows      : %04X\r\n",rows);
            printf("doublesize: %04X\r\n",double_size);
            #endif
            break;
    case 4:
            raster_value = data << 1;
            break;
    case 5:
            screenmem=(data & 0xf0) >> 4;
            screenmem_loc = mem_addrs[screenmem]+ (screen_mem_offset << 9);
            charmem=data & 0x0f;
            charmem_loc = mem_addrs[charmem];
            #ifdef DEBUG
            printf("screen_mem: %04X\r\n",screenmem_loc);
            printf("char_mem. : %04X\r\n",charmem_loc);
            #endif
            break;
    case 0x0e:
            aux_color = (data & 0xf0) >> 4;
            #ifdef DEBUG
            printf("aux_color : %04X\r\n",aux_color);
            #endif
            break;
    case 0x0f:
            screen_color = (data & 0xf0) >> 4;
            reverse_mode = (data & 0x8) >> 3;
            border_color = (data & 0x7);
            #ifdef DEBUG
            printf("screen_col: %04X\r\n",screen_color);
            printf("border_col: %04X\r\n",border_color);
            printf("reverse.  : %04X\r\n",reverse_mode);
            #endif
            break;

    default:
            break;
  }

  mem[address] = data;

}

uint8_t mon_do_tick(uint8_t ticks)
{
  return *interrupt;
}

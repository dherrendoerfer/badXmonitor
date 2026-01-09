#include <stdint.h>

/* Mappings between host keycodes and VIC-20 scancodes 
Note bit 7 indicates to also send the SHIFT scancode   */

/* Some keys are still missing */

/* XLATE, for terminal sessions etc. */
uint8_t key_to_scancode_xlate[256]= {
/*       0             1             2             3             4             5             6             7             8             9             A             B             C             D             E             F         */
/* 0*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*RET*/ 0x71, /*   */ 0xFF, /*   */ 0xFF,   // 0
/* 1*   */ 0x00, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*ESC*/ 0x03, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // 1
/* 2*SPC*/ 0x04, /* ! */ 0x80, /* " */ 0x87, /* # */ 0x90, /* $ */ 0x97, /* % */ 0x80, /* & */ 0xA7, /* ' */ 0xB0, /* ( */ 0xB7, /* ) */ 0xC0, /* * */ 0x61, /* + */ 0x50, /* , */ 0x53, /* - */ 0x57, /* . */ 0x54, /* / */ 0x63,   // 2
/* 3* 0 */ 0x47, /* 1 */ 0x00, /* 2 */ 0x07, /* 3 */ 0x10, /* 4 */ 0x17, /* 5 */ 0x20, /* 6 */ 0x27, /* 7 */ 0x30, /* 8 */ 0x37, /* 9 */ 0x40, /* : */ 0x55, /* ; */ 0x62, /* < */ 0xD3, /* = */ 0x65, /* > */ 0xD4, /* ? */ 0xE3,   // 3
/* 4* @ */ 0x56, /* A */ 0x92, /* B */ 0xB4, /* C */ 0xA4, /* D */ 0xA2, /* E */ 0x96, /* F */ 0xA5, /* G */ 0xB2, /* H */ 0xB5, /* I */ 0xC1, /* J */ 0xC2, /* K */ 0xC5, /* L */ 0xD2, /* M */ 0xC4, /* N */ 0xC3, /* O */ 0xC6,   // 4
/* 5* P */ 0xD1, /* Q */ 0x86, /* R */ 0xA1, /* S */ 0x95, /* T */ 0xA6, /* U */ 0xB6, /* V */ 0xB3, /* W */ 0x91, /* X */ 0xA3, /* Y */ 0xB1, /* Z */ 0x94, /* [ */ 0xD5, /*   */ 0xFF, /* ] */ 0xE2, /* ^ */ 0x66, /*   */ 0xFF,   // 5
/* 6*   */ 0xFF, /* a */ 0x12, /* b */ 0x34, /* c */ 0x24, /* d */ 0x22, /* e */ 0x16, /* f */ 0x25, /* g */ 0x32, /* h */ 0x35, /* i */ 0x41, /* j */ 0x42, /* k */ 0x45, /* l */ 0x52, /* m */ 0x44, /* n */ 0x43, /* o */ 0x46,   // 6
/* 7* p */ 0x51, /* q */ 0x06, /* r */ 0x21, /* s */ 0x15, /* t */ 0x26, /* u */ 0x36, /* v */ 0x33, /* w */ 0x11, /* x */ 0x23, /* y */ 0x31, /* z */ 0x14, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*DEL*/ 0x70,   // 7
/* 8*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // 8
/* 9*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // 9
/* A*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // A
/* B*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // B
/* C*   */ 0xFF, /*   */ 0x74, /*   */ 0x75, /*   */ 0x76, /*   */ 0x77, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // C
/* D*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // D
/* E*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF,   // E
/* F*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF, /*   */ 0xFF }; // F



/* Source : https://www.lemon64.com/forum/viewtopic.php?t=68210
VIC20 Keyboard Matrix

Write to Port B($9120)column
Read from Port A($9121)row

     7   6   5   4   3   2   1   0
    --------------------------------
  7| F7  F5  F3  F1  CDN CRT RET DEL    CRT=Cursor-Right, CDN=Cursor-Down
   |
  6| HOM UA  =   RSH /   ;   *   BP     BP=British Pound, RSH=Should be Right-SHIFT,
   |                                    UA=Up Arrow
  5| -   @   :   .   ,   L   P   +
   |
  4| 0   O   K   M   N   J   I   9
   |
  3| 8   U   H   B   V   G   Y   7
   |
  2| 6   T   F   C   X   D   R   5
   |
  1| 4   E   S   Z   LSH A   W   3      LSH=Should be Left-SHIFT
   |
  0| 2   Q   CBM SPC STP CTL LA  1      LA=Left Arrow, CTL=Should be CTRL, STP=RUN/STOP
   |                                    CBM=Commodore key

C64/VIC20 Keyboard Layout

  LA  1  2  3  4  5  6  7  8  9  0  +  -  BP HOM DEL    F1
  CTRL Q  W  E  R  T  Y  U  I  O  P  @  *  UA RESTORE   F3
STOP SL A  S  D  F  G  H  J  K  L  :  ;  =  RETURN      F5
C= SHIFT Z  X  C  V  B  N  M  ,  .  /  SHIFT  CDN CRT   F7
         [        SPACE BAR       ]


*/
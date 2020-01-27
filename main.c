/* *********************************************************************
      SMS Test Suite - by sverx
           ("largely inspired by Artemio's 240p, but not a fork of it!")
************************************************************************ */

#define MAJOR_VER 0
#define MINOR_VER 29

#define MIN(a,b)  (((a)<(b))?(a):(b))
#define MAX(a,b)  (((a)>=(b))?(a):(b))

#include <stdio.h>
#include <stdbool.h>

#define MD_PAD_SUPPORT
#define VDPTYPE_DETECTION
#include "../SMSlib/SMSlib.h"
#include "../PSGlib/PSGlib.h"

#include "bank1.h"

#define MAIN_MENU_ITEMS 5
const unsigned char *main_menu[MAIN_MENU_ITEMS] =   {"Video Tests",
                                                     "Audio Tests",
                                                     "Pad Tests",
                                                     "System Info",
                                                     "Paddle Tests"};

#define VIDEO_MENU_ITEMS 10
const unsigned char *video_menu[VIDEO_MENU_ITEMS] = {"PLUGE",
                                                     "Color bars",
                                                     "Color bleed",
                                                     "Grid",
                                                     "Stripes/Checks",
                                                     "Full colors",
                                                     "Linearity",
                                                     "Drop Shadow",
                                                     "Striped sprite",
                                                     "[ back ]"};
#define BIOSES_ITEMS 13
const struct {
  const unsigned int sum8k;
  const unsigned char *name;
} BIOSes[BIOSES_ITEMS] = {
  {0x9204,"Alex Kidd in Miracle World"},
  {0x8738,"Hang On & Safari Hunt"},
  {0x7f6f,"Hang On"},
  {0xde36,"Sonic the Hedgehog"},
  {0xf124,"Missile Defense 3-D"},
  {0xaf19,"Samsung Gam*Boy/Aladdin Boy"},     // this has AKiMW game in it

  {0xef78,"US-European SMS BIOS v1.3"},
  {0x95c3,"Japanese SMS BIOS v2.1"},
  {0x934d,"M404 Prototype BIOS"},
  {0x5f04,"Prototype BIOS v1.0"},
  {0xe8b0,"SMS BIOS v2.0"},

  {0xdc4e,"Store Display Unit BIOS"},         // not sure this is really useful

  {0x6ae3,"Emulicious (emulator) BIOS"}       //  ;)
};


#define MENU_FIRST_ROW  8
#define MENU_FIRST_COL  4

#define FOOTER_COL  1
#define FOOTER_ROW  21

unsigned char cur_menu_item, main_menu_items, pointer_anim;

/* hardware tests results */
unsigned char TV_model;
bool is_MegaDrive,has_new_VDP,is_Japanese,do_Port3E_works;
unsigned char some_paddle_connected;

/* VDP sprite zoom capabilities */
#define SZC_ABSENT   0
#define SZC_PARTIAL  1
#define SZC_COMPLETE 2

/*  ****************** for PADS TESTS *********************** */

#define COLOR_UP         10
#define COLOR_DOWN       12
#define COLOR_LEFT       5
#define COLOR_RIGHT      6
#define COLOR_1          2
#define COLOR_2          11
#define COLOR_A          7
#define COLOR_X          9
#define COLOR_Y          3
#define COLOR_Z          4
#define COLOR_START      8
#define COLOR_MODE       1
#define COLOR_RESET      13
#define COLOR_PAUSE      14

#define HILIT_COLOR      0x2F
#define BLACK            0x00

/*  ****************** for PADDLE TESTS *********************** */

#define COLOR_PADDLE_A       10
#define COLOR_PADDLE_B       12
#define COLOR_PADDLE_A_1     2
#define COLOR_PADDLE_B_1     11

#define PADDLE_THRESHOLD     8

#define APPROX_1_SEC     55
#define APPROX_3_SECS    (APPROX_1_SEC*3)

#define PAUSE_TIMEOUT    APPROX_1_SEC


/*  *********** for running code in RAM *******************  */

#define CODE_IN_RAM_SIZE  256
#define TEMP_BUF_SIZE     (4*1024)

#define CART_CHECK_ADDR   0x7F00
#define CART_CHECK_SIZE   256

unsigned char pause_cnt;
unsigned int kp,kr,mp,mr,ks;
unsigned char code_in_RAM[CODE_IN_RAM_SIZE];   // 256 bytes should be enough for everything
unsigned char temp_buf[TEMP_BUF_SIZE];

/*  **************** [[[ CODE ]]] ************************** */

unsigned int compute_BIOS_sum_8K (void) __naked {
  /* *************************
     NOTE: this code will be copied to RAM and run from there!
     *************************  */
  __asm
    di               ; interrupts should be disabled!

    ld a,#0b11100011 ; reset bits 3,4 (enable BIOS/RAM) and set bits 5,6,7 (to disable card/cartridge/expansion)
    out (#0x3E),a    ; do!

    ld de,#0         ; sum
    ld hl,#0x0000    ; src
loop:
    ld a,(hl)        ; a=*src
    add a,e
    ld e,a
    jr nc,nocarry
    inc d
nocarry:
    inc hl           ; src++
    ld a,h
    cp #0x20         ; 0x2000 = 8K
    jr nz,loop
    ex de,hl         ; hl=sum

    ; ld a,#0b10101011 ; reset bits 4,6 (enable RAM/cartridge) and set bits 3,5,7 (to disable BIOS/card/expansion)
    ld a,(_SMS_Port3EBIOSvalue)    
    out (#0x3E),a    ; restore it

    ei               ; re-enable interrupts!

    ret              ; because I am naked ;)
  __endasm;
}

unsigned int get_BIOS_sum (void) __naked {
  __asm
    ld hl,#_compute_BIOS_sum_8K
    ld de,#_code_in_RAM
    ld bc,#CODE_IN_RAM_SIZE
    ldir                           ; copy code in RAM
    jp _code_in_RAM
  __endasm;
}

void ldir_BIOS_SRAM (void) __naked {
  /* *************************
     NOTE: this code will be copied to RAM and run from there!
     *************************  */
  __asm
    di               ; interrupts should be disabled!

    ; 1st half ***********

    ld hl,#0x0000    ; src (BIOS)
    ld de,#0x8000    ; dst (SRAM)
    ld b,#4          ; 4 times (4 times 4KB)

outloop1:
    push bc
      ld a,#0b11100011    ; reset bits 3,4 (enable BIOS/RAM) and set bits 5,6,7 (to disable card/cartridge/expansion)
      out (#0x3E),a       ; do!

      push de
        ld de,#_temp_buf         ; dst (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir

        ; ld a,#0b10101011  ; reset bits 4,6 (enable RAM/cartridge) and set bits 3,5,7 (to disable BIOS/card/expansion)
        ld a,(_SMS_Port3EBIOSvalue)
        out (#0x3E),a     ; restore it
      pop de
      push hl
        ld hl,#_temp_buf         ; src (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir
      pop hl

    pop bc
    djnz outloop1

/* *************************************

    // ---> My Master EverDrive is the old model
    //      and doesn't seem to support 32 KB saves :(

    ld a,#0x0C       ; select SRAM bank 2
    ld (#0xfffc),a

    ; 2nd half ***********

    ld b,#4          ; 4 times
    ld de,#0x8000    ; dst (SRAM)

outloop2:
    push bc

      ld a,#0b11100011    ; reset bits 3,4 (enable BIOS/RAM) and set bits 5,6,7 (to disable card/cartridge/expansion)
      out (#0x3E),a       ; do!

      push de
        ld de,#_temp_buf         ; dst (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir

        ld a,#0b10101011  ; reset bits 4,6 (enable RAM/cartridge) and set bits 3,5,7 (to disable BIOS/card/expansion)
        out (#0x3E),a     ; restore it
      pop de
      push hl
        ld hl,#_temp_buf         ; src (RAM)
        ld bc,#TEMP_BUF_SIZE     ; 4K
        ldir
      pop hl

    pop bc
    djnz outloop2

 *************************************  */

    ei               ; re-enable interrupts!

    ret              ; because I am naked ;)
  __endasm;
}

void dump_BIOS (void) __naked {
  __asm
    ld hl,#_ldir_BIOS_SRAM
    ld de,#_code_in_RAM
    ld bc,#CODE_IN_RAM_SIZE
    ldir                           ; copy code in RAM
    jp _code_in_RAM
  __endasm;
}

unsigned char detect_Port3E_effectiveness (void)  __naked {
  /* *************************
     NOTE: this code will be copied to RAM and run from there!
     *************************  */
  __asm
    di               ; interrupts should be disabled!

    ld a,(_SMS_Port3EBIOSvalue)
    or #0xE0         ; set bits 5,6,7 (to disable card/cartridge/expansion)
    out (#0x3E),a    ; do! (should have NO effect on a GameGear and on a Mark III)

    ld hl,#CART_CHECK_ADDR
    ld de,#_temp_buf
    ld b,#CART_CHECK_SIZE
match_loop:
    ld a,(de)
    cp (hl)
    jr nz,no_match   ; check if I can still read from card/cartridge/expansion
    inc hl
    inc de
    djnz match_loop
    ld l,#0          ; I can: port3E is *not* effective
    jr cont

no_match:
    ld l,#1          ; I failed: port3E *is* effective

cont:
    ld a,(_SMS_Port3EBIOSvalue)
    out (#0x3E),a    ; restore port 0x3E
    ei               ; re-enable interrupts!

    ret              ; because I am naked ;)
  __endasm;
}

bool is_Port3E_effective (void) __naked {
  __asm
    ld hl,#CART_CHECK_ADDR
    ld de,#_temp_buf
    ld bc,#CART_CHECK_SIZE
    ldir                           ; copy some bytes from card/cartridge/expansion to RAM
    ld hl,#_detect_Port3E_effectiveness
    ld de,#_code_in_RAM
    ld bc,#CODE_IN_RAM_SIZE
    ldir                           ; copy code in RAM
    jp _code_in_RAM
  __endasm;
}

bool isJapanese (void) __naked {
  /* ==============================================================
   Region detector
   Returns 1 for Japanese, 0 for export
   Based on code found in many Sega games (eg. Super Tennis)
   ==============================================================  */
  /* *** taken from http://www.smspower.org/Development/RegionDetection *** */
  __asm

    ld a,#0b11110101  ; Output 1s on both TH lines
    out (#0x3f),a
    in a,(#0xdd)
    and #0b11000000   ; See what the TH inputs are
    cp #0b11000000    ; If the input does not match the output then it is a Japanese system
    jp nz,_IsJap

    ld a,#0b01010101  ; Output 0s on both TH lines
    out (#0x3f),a
    in a,(#0xdd)
    and #0b11000000   ; See what the TH inputs are
    jp nz,_IsJap      ; If the input does not match the output then it is a Japanese system

    ld a,#0b11111111  ; Set everything back to being inputs
    out (#0x3f),a

    ld l,#0
    ret

_IsJap:
    ld l,#1
    ret
  __endasm;
}

#pragma save
#pragma disable_warning 85
bool paddle_detection (unsigned char which) __z88dk_fastcall __naked {
  __asm
    ld a,l
    or a
    ld bc,#0
    jr nz, detect_second_pad

read:    
    in a,(#0xDC)
    and #0x20
    jr nz, skip_inc
    inc c
    
skip_inc:
    djnz read
    jr discriminate
  
detect_second_pad:
    in a,(#0xDD)
    and #08
    jr nz, skip_inc_2
    inc c
    
skip_inc_2:
    djnz detect_second_pad
  
discriminate:
    ld a,c
    ld l,#0     ; set false
    sub #0x60
    ret c
    cp #0x40
    ret nc
    inc l       ; set true
    ret
  __endasm;
}

unsigned char read_paddle (unsigned char which) __z88dk_fastcall __naked {
  __asm
    ld a,l
    or a
    jr nz, read_second_pad
  
wait_5_reset:  
    in a,(#0xDC)
    bit 5,a
    jr nz, wait_5_reset     ; wait until bit 5 is 0
    and #0x0F
    ld l,a                  ; save lower 4 bits into l
    
wait_5_set:    
    in a,(#0xDC)
    bit 5,a
    jr z, wait_5_set        ; wait until bit 5 is 1
    and #0x0F               ; save lower 4 bits
    add a,a
    add a,a
    add a,a
    add a,a
    or l                    ; move to high nibble
    ld l,a                  ; together with lower part
    ret
    
read_second_pad:
    ld c,#0xDC

wait_3_reset:
    in a,(#0xDD)            ; ensure we are reading both ports same moment
    ld e,a
    in b,(c)
    in a,(#0xDD)
    or e
    bit 3,a
    jr nz, wait_3_reset     ; wait until bit 5 is 0
    ld a,b
    and #0xC0               ; save upper 2 bits 
    rlca
    rlca
    ld l,a                  ; into l (bits 0,1)
    ld a,e
    and #0x03               ; save lower 2 bits
    rlca
    rlca
    or l                    ; together with l
    ld l,a                  ; into l (bits 2,3)
    
wait_3_set:
    in a,(#0xDD)            ; ensure we are reading both ports same moment
    ld e,a
    in b,(c)
    in a,(#0xDD)
    and e
    bit 3,a
    jr z, wait_3_set        ; wait until bit 5 is 1
    ld a,b
    and #0xC0               ; save upper 2 bits 
    rrca
    rrca
    ld h,a                  ; into h (bits 4,5)
    ld a,e
    and #0x03               ; save lower 2 bits
    rrca
    rrca
    or h                    ; together with h (bits 6,7)
    or l                    ; together with lower part
    ld l,a
    ret

  __endasm;
}
#pragma restore

/*
unsigned char port0 (void) __naked {
  __asm
    in a,(#0)
    ld l,a
    ret
  __endasm;
}
*/

unsigned char detectVDPSpriteZoomCapabilities (void) {
  unsigned char i;
  SMS_VRAMmemset (0x0000, 0xFF, 32);         // a 'full' tile
  SMS_useFirstHalfTilesforSprites(true);
  SMS_setSpriteMode (SPRITEMODE_ZOOMED);
  
  SMS_initSprites();
  SMS_addSprite (0, 0, 0);    // first sprite
  SMS_addSprite (15, 0, 0);   // second sprite, one pixel 'too' to the left
  SMS_waitForVBlank();           // wait VBlank
  SMS_copySpritestoSAT();        // copy sprites to SAT
  SMS_waitForVBlank();           // wait next VBlank...
  // ... and if it's a MD the collision flag will be OFF
  if (!(SMS_VDPFlags & VDPFLAG_SPRITECOLLISION))
    return (SZC_ABSENT);
  
  SMS_initSprites();
  for (i=0;i<5;i++)
    SMS_addSprite (16*i, 0, 0);  // first 5 sprites, evenly spaced horizontally
  SMS_addSprite (16*i-1, 0, 0);  // 6th sprite, one pixel 'too' to the left (will overlap 5th sprite if it has been correctly zoomed)
  SMS_waitForVBlank();           // wait VBlank
  SMS_copySpritestoSAT();        // copy sprites to SAT
  SMS_waitForVBlank();           // wait next VBlank...
  // ... and if it's a fixed VDP the collision flag will be ON
  if (SMS_VDPFlags & VDPFLAG_SPRITECOLLISION)
    return(SZC_COMPLETE);   // sprite zoom fully working
  else
    return(SZC_PARTIAL);    // zoom bugged
}

void draw_footer_and_ver (void) {

  // print console model
  SMS_setNextTileatXY(FOOTER_COL,FOOTER_ROW);
  printf (" Model:");
  if (is_MegaDrive)
      printf ("Genesis/MegaDrive      ");
  else if (do_Port3E_works) {
    if (has_new_VDP)
      printf ("Master System II       ");
    else
      printf ("Master System          ");
  } else {                               //   !do_Port3E_works
    if (has_new_VDP)
      printf ("Game Gear              ");    // no port3E support, new VDP
    else
      printf ("Mark III               ");    // no port3E support, old VDP
  }

  // print region
  SMS_setNextTileatXY(FOOTER_COL,FOOTER_ROW+1);
  printf (" Region:");
  if (is_Japanese)
    printf ("JPN/KOR");                 // Japanese/Korean
  else if (TV_model & VDP_NTSC)
    printf ("USA/BRA");                 // not Japanese and 60 Hz => USA/Brasil
  else if (TV_model & VDP_PAL)
    printf ("EUR/AUS");                 // not Japanese and 50 Hz => EUR/Australia
  else
    printf ("- ??? -");                 // not Japanese and undetected TV (this shouldn't happen with current detection routine)

  // print TV mode
  printf (" TV:");
  if (TV_model & VDP_PAL)
    printf ("50Hz (PAL) ");
  else if (TV_model & VDP_NTSC)
    printf ("60Hz (NTSC)");
  else
    printf ("undetected ");            // (this shouldn't happen with current detection routine)
    
  // print if paddle is detected
  if (some_paddle_connected) {
    SMS_setNextTileatXY(FOOTER_COL,FOOTER_ROW-1);
    printf (" Paddle Control found in [%c]  ",(some_paddle_connected==0x01)?'1':'2');
  }

  // print program version (just under the title)
  SMS_setNextTileatXY(3,4);
  printf ("ver %d.%2d",MAJOR_VER,MINOR_VER);

}

void draw_menu (unsigned char *menu[], unsigned int max) {
  unsigned char i;
  for (i=0;i<max;i++) {
    SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+i);
    printf ("  %-16s",menu[i]);
  }
}

void load_menu_assets (void) {
  // load background tiles, map and palette
  SMS_loadPSGaidencompressedTiles (BG__tiles__psgcompr,96);
  SMS_loadSTMcompressedTileMap (0,0,BG__tilemap__stmcompr);
  SMS_loadBGPalette(BG__palette__bin);
  // load sprite tile, build palette, turn on text renderer
  SMS_loadPSGaidencompressedTiles (arrow__tiles__psgcompr,256);
  SMS_setSpritePaletteColor (0, RGB(0,0,0));
  SMS_setSpritePaletteColor (1, RGB(3,3,3));
  SMS_autoSetUpTextRenderer();
}

unsigned int filter_paddle(unsigned int value) {
  if (some_paddle_connected & 0x01)
    value&=~PORT_A_KEY_2;
  if (some_paddle_connected & 0x02)
    value&=~PORT_B_KEY_2;
  return (value);
}

void static_screen (void* tiles, void* tilemap, void* palette, void* alt_tiles) {
  bool alt=false;
  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_loadPSGaidencompressedTiles (tiles,0);
  SMS_loadSTMcompressedTileMap (0,0,tilemap);
  SMS_loadBGPalette(palette);
  SMS_displayOn();
  for (;;) {
    SMS_waitForVBlank();
    kp=filter_paddle(SMS_getKeysPressed());
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      break;
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1))
      if (alt_tiles==NULL)
        break;
      else
        alt=!alt, SMS_loadPSGaidencompressedTiles (((alt)?alt_tiles:tiles),0);
  }
  SMS_displayOff();
}

void drop_shadow_striped_sprite (bool striped) {
  unsigned char frame=0,pos=0;

  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_loadPSGaidencompressedTiles (AlexKidd__tiles__psgcompr,0);
  SMS_loadSTMcompressedTileMap (0,0,AlexKidd__tilemap__stmcompr);
  SMS_loadBGPalette(AlexKidd__palette__bin);
  if (striped) {
    SMS_loadPSGaidencompressedTiles (striped__tiles__psgcompr,256);
  } else {
    SMS_loadPSGaidencompressedTiles (drop__tiles__psgcompr,256);
  }
  SMS_zeroSpritePalette();
  SMS_setSpriteMode(SPRITEMODE_TALL);
  SMS_displayOn();
  for (;;) {
    SMS_initSprites();
    ks=SMS_getKeysStatus();

    if (striped) {
      SMS_addTwoAdjoiningSprites (110+pos,   80+pos,0);
      SMS_addTwoAdjoiningSprites (110+16+pos,80+pos,4);
      SMS_addTwoAdjoiningSprites (110+pos,   80+16+pos,8);
      SMS_addTwoAdjoiningSprites (110+16+pos,80+16+pos,12);

      if (ks & (PORT_A_KEY_UP|PORT_A_KEY_LEFT|PORT_B_KEY_UP|PORT_B_KEY_LEFT))
        pos--;
      else if (ks & (PORT_A_KEY_DOWN|PORT_A_KEY_RIGHT|PORT_B_KEY_DOWN|PORT_B_KEY_RIGHT))
        pos++;

    } else {
      SMS_addTwoAdjoiningSprites (32+pos,(64*frame)+pos,0);
      SMS_addTwoAdjoiningSprites (32+16+pos,(64*frame)+pos,4);
      SMS_addTwoAdjoiningSprites (32+pos,(64*frame)+pos+16,8);
      SMS_addTwoAdjoiningSprites (32+16+pos,(64*frame)+pos+16,12);

      SMS_addTwoAdjoiningSprites (96+pos,48+(64*frame)+pos,0);
      SMS_addTwoAdjoiningSprites (96+16+pos,48+(64*frame)+pos,4);
      SMS_addTwoAdjoiningSprites (96+pos,48+(64*frame)+pos+16,8);
      SMS_addTwoAdjoiningSprites (96+16+pos,48+(64*frame)+pos+16,12);

      SMS_addTwoAdjoiningSprites (160+pos,96+(64*frame)+pos,0);
      SMS_addTwoAdjoiningSprites (160+16+pos,96+(64*frame)+pos,4);
      SMS_addTwoAdjoiningSprites (160+pos,96+(64*frame)+pos+16,8);
      SMS_addTwoAdjoiningSprites (160+16+pos,96+(64*frame)+pos+16,12);

      pos++;
      frame=1-frame;
    }

    SMS_waitForVBlank();
    SMS_copySpritestoSAT();

    kp=filter_paddle(SMS_getKeysPressed());
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2|PORT_A_KEY_1|PORT_B_KEY_1))
      break;
  }
  SMS_displayOff();
  SMS_setSpriteMode(SPRITEMODE_NORMAL);
}

void fullscreen (void) {
  unsigned char which=0;
  SMS_displayOff();
  SMS_VRAMmemset (0x0000, 0x00, 32);               // a 'empty' tile
  SMS_VRAMmemset (XYtoADDR(0,0), 0x00, 32*28*2);  // full map of 'empty' tiles
  SMS_setBGPaletteColor (0, RGB(3,3,3));
  SMS_displayOn();
  for (;;) {
    SMS_initSprites();
    SMS_waitForVBlank();
    SMS_copySpritestoSAT();
    kp=filter_paddle(SMS_getKeysPressed());
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      break;
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1)) {
      which=(which+1)%4;
      switch (which) {
        case 0:SMS_setBGPaletteColor (0, RGB(3,3,3)); break;
        case 1:SMS_setBGPaletteColor (0, RGB(3,0,0)); break;
        case 2:SMS_setBGPaletteColor (0, RGB(0,3,0)); break;
        case 3:SMS_setBGPaletteColor (0, RGB(0,0,3)); break;
      }
    }
  }
  SMS_displayOff();
}

void video_tests (void) {
  bool go_back=false;
  draw_menu(video_menu, VIDEO_MENU_ITEMS);
  draw_footer_and_ver();
  cur_menu_item=0,pointer_anim=0;
  while (!go_back) {
    SMS_initSprites();
    SMS_addSprite(MENU_FIRST_COL*8+(pointer_anim/8),(MENU_FIRST_ROW+cur_menu_item)*8,0);
    SMS_waitForVBlank();
    SMS_copySpritestoSAT();

    if ((++pointer_anim)==7*8)
      pointer_anim=0;

    kp=filter_paddle(SMS_getKeysPressed());
    
    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP)) {        // UP
      if (cur_menu_item>0)
        cur_menu_item--;
      else
        cur_menu_item=VIDEO_MENU_ITEMS-1;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN)) {    // DOWN
      if (cur_menu_item<(VIDEO_MENU_ITEMS-1))
        cur_menu_item++;
      else
        cur_menu_item=0;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_1|PORT_A_KEY_2|PORT_B_KEY_1|PORT_B_KEY_2)) {
      switch (cur_menu_item) {
        case 0:static_screen(PLUGE__tiles__psgcompr,PLUGE__tilemap__stmcompr,PLUGE__palette__bin,NULL); break;
        case 1:static_screen(color_bars__tiles__psgcompr,color_bars__tilemap__stmcompr,color_bars__palette__bin,NULL); break;
        case 2:static_screen(color_bleed__tiles__psgcompr,color_bleed__tilemap__stmcompr,color_bars__palette__bin,color_bleed2__tiles__psgcompr); break;
        case 3:static_screen(grid__tiles__psgcompr,grid__tilemap__stmcompr,grid__palette__bin,NULL); break;
        case 4:static_screen(stripes__tiles__psgcompr,fullscreen__tilemap__stmcompr,bw_palette_bin,checkerboard__tiles__psgcompr); break;
        case 5:fullscreen(); break;
        case 6:if ((!is_MegaDrive) && (!do_Port3E_works) && (has_new_VDP))  // if it's a GameGear
                 static_screen(linearity_GG__tiles__psgcompr,linearity_GG__tilemap__stmcompr,linearity__palette__bin,NULL);
               else if (TV_model & VDP_PAL)
                 static_screen(linearity_PAL__tiles__psgcompr,linearity_PAL__tilemap__stmcompr,linearity__palette__bin,NULL);
               else
                 static_screen(linearity_NTSC__tiles__psgcompr,linearity_NTSC__tilemap__stmcompr,linearity__palette__bin,NULL);
               break;
        case 7:drop_shadow_striped_sprite(false);break;
        case 8:drop_shadow_striped_sprite(true);break;
        case VIDEO_MENU_ITEMS-1:go_back=true; break;
      }
      if (!go_back) {
        load_menu_assets();
        draw_menu(video_menu, VIDEO_MENU_ITEMS);
        draw_footer_and_ver();
      }
    }
  }
  cur_menu_item=0,pointer_anim=0;
}

void audio_test (void) {
  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_loadPSGaidencompressedTiles (controller__tiles__psgcompr,0);
  SMS_loadSTMcompressedTileMap (0,0,controller__tilemap__stmcompr);
  SMS_loadBGPalette(controller__palette__bin);
  SMS_displayOn();

  for(;;) {
    SMS_waitForVBlank();
    PSGFrame();
    kp=filter_paddle(SMS_getKeysPressed());
    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP))
      PSGPlay(CH0_psgc);
    if (kp & (PORT_A_KEY_RIGHT|PORT_B_KEY_RIGHT))
      PSGPlay(CH1_psgc);
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN))
      PSGPlay(CH2_psgc);
    if (kp & (PORT_A_KEY_LEFT|PORT_B_KEY_LEFT))
      PSGPlay(CH3_psgc);
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1))
      PSGPlay(VolumeTest_psgc);
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      break;
  }
  PSGStop();
}

void pad_tests (void) {
  unsigned char i,stay_cnt=0;
  bool should_stay=true;

  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_loadPSGaidencompressedTiles (pads__tiles__psgcompr,0);
  SMS_loadSTMcompressedTileMap (0,0,pads__tilemap__stmcompr);
  for (i=0;i<15;i++)
    SMS_setBGPaletteColor(i,0x00);    // black
  SMS_setBGPaletteColor(15,0x3f);     // white

  SMS_resetPauseRequest();            // there might be a previous pending request
  SMS_displayOn();
  while(should_stay) {
    SMS_waitForVBlank();

    // read controller
    kp=filter_paddle(SMS_getKeysPressed());
    kr=filter_paddle(SMS_getKeysReleased());

    // read MD controller
    mp=SMS_getMDKeysPressed();
    mr=SMS_getMDKeysReleased();

    // press
    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP))
      SMS_setBGPaletteColor(COLOR_UP,HILIT_COLOR);
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN))
      SMS_setBGPaletteColor(COLOR_DOWN,HILIT_COLOR);
    if (kp & (PORT_A_KEY_LEFT|PORT_B_KEY_LEFT))
      SMS_setBGPaletteColor(COLOR_LEFT,HILIT_COLOR);
    if (kp & (PORT_A_KEY_RIGHT|PORT_B_KEY_RIGHT))
      SMS_setBGPaletteColor(COLOR_RIGHT,HILIT_COLOR);
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1))
      SMS_setBGPaletteColor(COLOR_1,HILIT_COLOR);
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2))
      SMS_setBGPaletteColor(COLOR_2,HILIT_COLOR);

    if (kp & RESET_KEY)
      SMS_setBGPaletteColor(COLOR_RESET,HILIT_COLOR);
    if (SMS_queryPauseRequested()) {
      SMS_setBGPaletteColor(COLOR_PAUSE,HILIT_COLOR);
      pause_cnt=PAUSE_TIMEOUT;
      SMS_resetPauseRequest();
      stay_cnt=0;
    }

    if (mp & PORT_A_MD_KEY_A)
      SMS_setBGPaletteColor(COLOR_A,HILIT_COLOR);
    if (mp & PORT_A_MD_KEY_X)
      SMS_setBGPaletteColor(COLOR_X,HILIT_COLOR);
    if (mp & PORT_A_MD_KEY_Y)
      SMS_setBGPaletteColor(COLOR_Y,HILIT_COLOR);
    if (mp & PORT_A_MD_KEY_Z)
      SMS_setBGPaletteColor(COLOR_Z,HILIT_COLOR);
    if (mp & PORT_A_MD_KEY_START)
      SMS_setBGPaletteColor(COLOR_START,HILIT_COLOR);
    if (mp & PORT_A_MD_KEY_MODE)
      SMS_setBGPaletteColor(COLOR_MODE,HILIT_COLOR);

    // release
    if (kr & (PORT_A_KEY_UP|PORT_B_KEY_UP))
      SMS_setBGPaletteColor(COLOR_UP,BLACK);
    if (kr & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN))
      SMS_setBGPaletteColor(COLOR_DOWN,BLACK);
    if (kr & (PORT_A_KEY_LEFT|PORT_B_KEY_LEFT))
      SMS_setBGPaletteColor(COLOR_LEFT,BLACK);
    if (kr & (PORT_A_KEY_RIGHT|PORT_B_KEY_RIGHT))
      SMS_setBGPaletteColor(COLOR_RIGHT,BLACK);
    if (kr & (PORT_A_KEY_1|PORT_B_KEY_1))
      SMS_setBGPaletteColor(COLOR_1,BLACK);
    if (kr & (PORT_A_KEY_2|PORT_B_KEY_2))
      SMS_setBGPaletteColor(COLOR_2,BLACK);
    if (kr & RESET_KEY)
      SMS_setBGPaletteColor(COLOR_RESET,BLACK);

    if (mr & PORT_A_MD_KEY_A)
      SMS_setBGPaletteColor(COLOR_A,BLACK);
    if (mr & PORT_A_MD_KEY_X)
      SMS_setBGPaletteColor(COLOR_X,BLACK);
    if (mr & PORT_A_MD_KEY_Y)
      SMS_setBGPaletteColor(COLOR_Y,BLACK);
    if (mr & PORT_A_MD_KEY_Z)
      SMS_setBGPaletteColor(COLOR_Z,BLACK);
    if (mr & PORT_A_MD_KEY_START)
      SMS_setBGPaletteColor(COLOR_START,BLACK);
    if (mr & PORT_A_MD_KEY_MODE)
      SMS_setBGPaletteColor(COLOR_MODE,BLACK);

    if (pause_cnt)
      if (--pause_cnt==0)
        SMS_setBGPaletteColor(COLOR_PAUSE,BLACK);

    // if there are no keys pressed or held or released in (approx.) 3 seconds, leave the test
    if ((((kp | kr | mp | mr) & ~CARTRIDGE_SLOT)==0) && ((filter_paddle(SMS_getKeysHeld()) & ~CARTRIDGE_SLOT)==0) && (SMS_getMDKeysHeld()==0)) {
      if (++stay_cnt>APPROX_3_SECS)
        should_stay=false;
    } else
      stay_cnt=0;
  }
}

void paddle_test (bool stay_forever) {
  unsigned char i,stay_cnt=0;
  unsigned char p1=0,p2=0,old_p1=0,old_p2=0;
  bool should_stay=true;

  SMS_displayOff();
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_loadPSGaidencompressedTiles (paddles__tiles__psgcompr,0);
  SMS_loadPSGaidencompressedTiles (ball__tiles__psgcompr,256);
  SMS_loadSTMcompressedTileMap (0,0,paddles__tilemap__stmcompr);
  for (i=0;i<15;i++)
    SMS_setBGPaletteColor(i,0x00);    // black
  SMS_setBGPaletteColor(15,0x3f);     // white

  SMS_resetPauseRequest();            // there might be a previous pending request
  SMS_displayOn();
  while(should_stay) {
    SMS_waitForVBlank();
    SMS_copySpritestoSAT();
    
    SMS_initSprites();
    // read paddle controller knob position
    if (some_paddle_connected & 0x01) {
      old_p1=p1;
      p1=read_paddle(0);
      SMS_addSprite(p1/2+58,85,0);  // cursor paddle 1
    }
    if (some_paddle_connected & 0x02) {
      old_p2=p2;
      p2=read_paddle(1);
      SMS_addSprite(p2/2+58,180,0);  // cursor paddle 2
    }
    
    // read paddle buttons
    kp=filter_paddle(SMS_getKeysPressed());
    kr=filter_paddle(SMS_getKeysReleased());
    
    // press key
    if (kp & PORT_A_KEY_1)
      SMS_setBGPaletteColor(COLOR_PADDLE_A_1,HILIT_COLOR);
    if (kp & PORT_B_KEY_1)
      SMS_setBGPaletteColor(COLOR_PADDLE_B_1,HILIT_COLOR);
      
    // release key
    if (kr & PORT_A_KEY_1)
      SMS_setBGPaletteColor(COLOR_PADDLE_A_1,BLACK);
    if (kr & PORT_B_KEY_1)
      SMS_setBGPaletteColor(COLOR_PADDLE_B_1,BLACK);  
    
    // moving paddle 1
    if ((MAX(p1,old_p1)-MIN(p1,old_p1))>PADDLE_THRESHOLD) {
      SMS_setBGPaletteColor(COLOR_PADDLE_A,HILIT_COLOR);
      stay_cnt=0;
    } else
      SMS_setBGPaletteColor(COLOR_PADDLE_A,BLACK);
    
    // moving paddle 2  
    if ((MAX(p2,old_p2)-MIN(p2,old_p2))>PADDLE_THRESHOLD) {
      SMS_setBGPaletteColor(COLOR_PADDLE_B,HILIT_COLOR);
      stay_cnt=0; 
    } else
      SMS_setBGPaletteColor(COLOR_PADDLE_B,BLACK);    
    
    if ((((kp | kr) & ~CARTRIDGE_SLOT)==0) && ((filter_paddle(SMS_getKeysHeld()) & ~CARTRIDGE_SLOT)==0) && (!stay_forever)) {
      if (++stay_cnt>APPROX_3_SECS)
        should_stay=false;
    } else
      stay_cnt=0;
  
  }
}

void prepare_and_show_main_menu (void) {
  SMS_displayOff();
  load_menu_assets();
  draw_footer_and_ver();
  draw_menu(main_menu, main_menu_items);
}

void sysinfo (void) {
  unsigned int sum8k;
  unsigned char i=0;

  SMS_initSprites();
  SMS_copySpritestoSAT();

  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW);
  printf (" Hardware tests ");
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+1);
  printf (" JPN?:%-3s     ",(is_Japanese?"Yes":"No"));
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+2);
  printf (" VDP?:");
  if (is_MegaDrive)
    printf ("315-5313");
  else
    printf ("%s",(has_new_VDP?"315-5246":"315-5124"));
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+3);
  printf (" TV? :");
  if (TV_model & VDP_NTSC)
    printf ("60Hz (NTSC)");               // 60 Hz
  else if (TV_model & VDP_PAL)
    printf ("50Hz (PAL) ");               // 50 Hz
  else
    printf ("*???*      ");               // undetected TV (?)
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+4);
  printf (" MediaEnable?:%-3s ",(do_Port3E_works?"Yes":"No"));
  SMS_setNextTileatXY(MENU_FIRST_COL,MENU_FIRST_ROW+5);
  if ((!is_MegaDrive) && (do_Port3E_works)) {   // if not MegaDrive and not GameGear and not Mark III
    sum8k=get_BIOS_sum();
    printf (" BIOS sum: 0x%04X ",sum8k);
    SMS_setNextTileatXY(1,MENU_FIRST_ROW+7);
    while (i<BIOSES_ITEMS) {
      if (sum8k==BIOSes[i].sum8k) {
        printf ("%-30s",BIOSes[i].name);
        break;
      }
      i++;
    }
    if (i==BIOSES_ITEMS)
      printf ("** unidentified BIOS found! **");
  }

  for (;;) {
    SMS_waitForVBlank();
    kp=filter_paddle(SMS_getKeysPressed());
      
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN)) {
      SMS_enableSRAM();
      dump_BIOS();
      SMS_disableSRAM();
    }
    if (kp & (PORT_A_KEY_2|PORT_B_KEY_2|PORT_A_KEY_1|PORT_B_KEY_1))
      break;
  }
}

void main (void) {
  // detect region
  is_Japanese=isJapanese();

  // detect if Port3E features are present
  do_Port3E_works=is_Port3E_effective();

  // detect TV and console type using various means
  SMS_displayOn();
  TV_model=SMS_VDPType();
  //has_new_VDP=newVDP();
  unsigned char ZoomCapabilities=detectVDPSpriteZoomCapabilities();
  is_MegaDrive=(ZoomCapabilities==SZC_ABSENT);
  has_new_VDP=(ZoomCapabilities==SZC_COMPLETE);
  
  SMS_displayOff();
  
  some_paddle_connected=(paddle_detection(0))|(paddle_detection(1)<<1);
  main_menu_items=(some_paddle_connected)?MAIN_MENU_ITEMS:MAIN_MENU_ITEMS-1;

  // restore standard operation modes
  SMS_initSprites();
  SMS_copySpritestoSAT();
  SMS_useFirstHalfTilesforSprites(false);
  SMS_setSpriteMode (SPRITEMODE_NORMAL);
  
  if (some_paddle_connected==0x03)  // two paddles connected!
    paddle_test(true);              // show the paddle test until someone turns off the SMS and remove at least one of them!

  // prepare and show main menu
  prepare_and_show_main_menu();


  for (;;) {
    SMS_initSprites();
    SMS_addSprite(MENU_FIRST_COL*8+(pointer_anim/8),(MENU_FIRST_ROW+cur_menu_item)*8,0);
    SMS_waitForVBlank();
    SMS_copySpritestoSAT();

    if ((++pointer_anim)==7*8)
      pointer_anim=0;

    kp=filter_paddle(SMS_getKeysPressed());
   
    if (kp & (PORT_A_KEY_UP|PORT_B_KEY_UP)) {        // UP
      if (cur_menu_item>0)
        cur_menu_item--;
      else
        cur_menu_item=main_menu_items-1;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_DOWN|PORT_B_KEY_DOWN)) {    // DOWN
      if (cur_menu_item<(main_menu_items-1))
        cur_menu_item++;
      else
        cur_menu_item=0;
      pointer_anim=0;
    }
    if (kp & (PORT_A_KEY_1|PORT_B_KEY_1)) {
      switch (cur_menu_item) {
        case 0:video_tests(); break;
        case 1:audio_test(); break;
        case 2:pad_tests(); break;
        case 3:sysinfo(); break;
        case 4:paddle_test(false); break;
      }
      prepare_and_show_main_menu();
      SMS_resetPauseRequest();
    }
    
    // if PAUSE has been pressed, restart the ROM (after setting RAM location 0xC000 to what was initially)
    if (SMS_queryPauseRequested()) {
      SMS_displayOff();
      SMS_zeroBGPalette();
      __asm
         di
         ld a,(_SMS_Port3EBIOSvalue)
         ld (#0xC000),a
         rst #0
      __endasm;
    }

  }
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'

// to make this work with M404 prototype - uncomment the following line and comment SDSC header line (after that)
// const __at (0x7fe0) char __SMS_COPYRIGHT[]="COPYRIGHTSEGA";

SMS_EMBED_SDSC_HEADER_AUTO_DATE(MAJOR_VER,MINOR_VER, "sverx", "SEGA Master System TestSuite",
  "Built using devkitSMS/SMSlib\n[https://github.com/sverx/devkitSMS]");

/* NOTE: coding started on 19-06-2018 */

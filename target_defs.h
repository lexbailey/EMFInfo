/*
This file should contain all of the #defines for each target platform.
It should not contain any definition for any symbols, but can declare
them (for example extern functions)

things that need to be defined per target...

#define STORAGE_MEDIUM "tape" // tape or disk, normally. used on the ui e.g."load file from tape"
#define ENTER_KEY (0x0d) // the value of the enter key as returned by get_key_press()
#define BACKSPACE_KEY (0x0c) // the value of the backspace key as returned by get_key_press()
#define MAX_PRINTABLE (0x80) // The largest value that can be returned by get_key_press and still represent a printable character

#define BACKSPACE_NAME "Backspace" // What to call the backspace key in the text entry UI

// Box drawing characters. named after which part of the box is filled
#define BOXDRAW_BR "?" // Bottom right
#define BOXDRAW_BL "?" // Bottom left
#define BOXDRAW_TR "?" // Top right
#define BOXDRAW_TL "?" // Top left
#define BOXDRAW_L "?" // Left
#define BOXDRAW_R "?" // Right
#define BOXDRAW_B "?" // Bottom
#define BOXDRAW_T "?" // Top

// defined if main is allowed to return
#define MAIN_CAN_RETURN
*/

#ifdef TARGET_ZXSPEC48
    #define __TARGET_KNOWN
    #ifndef __SDCC_z80
        #error "The ZX Spectrum 48k target must be compiled with -mz80"
    #endif
    #define STORAGE_MEDIUM "tape"
    #define ENTER_KEY (0x0d)
    #define BACKSPACE_KEY (0x0c)
    #define MAX_PRINTABLE (0x80)
    #define BACKSPACE_NAME "CAPS-0"
    #define BOXDRAW_BR "\x84"
    #define BOXDRAW_BL "\x88"
    #define BOXDRAW_TR "\x81"
    #define BOXDRAW_TL "\x82"
    #define BOXDRAW_L "\x85"
    #define BOXDRAW_R "\x8a"
    #define BOXDRAW_B "\x8c"
    #define BOXDRAW_T "\x83"
    #define LAST_K (*((char*)(23560)))
    #pragma disable_warning 84
    #pragma disable_warning 85
    #include "mapdata.h"
    extern void dzx0_standard(unsigned char *src, unsigned char *dst);
#endif

#ifdef TARGET_PC_LINUX
    #define __TARGET_KNOWN
    #define STORAGE_MEDIUM "disk"
    #define ENTER_KEY ('\n')
    #define BACKSPACE_KEY (127)
    #define MAX_PRINTABLE (0x7F)
    #define BACKSPACE_NAME "Backspace"
    #define BOXDRAW_BR "\xE2\x94\x8F"
    #define BOXDRAW_BL "\xE2\x94\x93"
    #define BOXDRAW_TR "\xE2\x94\x97"
    #define BOXDRAW_TL "\xE2\x94\x9b"
    #define BOXDRAW_L "\xE2\x94\x83"
    #define BOXDRAW_R "\xE2\x94\x83"
    #define BOXDRAW_B "\xE2\x94\x81"
    #define BOXDRAW_T "\xE2\x94\x81"
    #include <termios.h>
    #include <unistd.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <signal.h>
    #include <stdint.h>
    #define MAP_BASE (map_full)
    #define MAP_NORTH_BASE (map_north)
    #define MAP_SOUTH_BASE (map_south)
    #define MAIN_CAN_RETURN
#endif

// To port this to another system, add extra TARGET option checks here

#ifndef __TARGET_KNOWN
    #error "Unknown target. Please define one of the TARGET_ preprocessor defs."
#endif


/*
This file should contain all of the #defines for each target platform.
It should not contain any definition for any symbols, but can declare
them (for example extern functions)

things that need to be defined per target...

#define STORAGE_MEDIUM "tape" // tape or disk, normally. used on the ui e.g."load file from tape"
#define ENTER_KEY (0x0d) // the value of the enter key as returned by get_key_press()
#define BACKSPACE_KEY (0x0c) // the value of the backspace key as returned by get_key_press()
#define MAX_PRINTABLE (0x80) // The largest value that can be returned by get_key_press and still represent a printable character

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
    #include <termios.h>
    #include <unistd.h>
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <signal.h>
    #include <stdint.h>
#endif

// To port this to another system, add extra TARGET option checks here

#ifndef __TARGET_KNOWN
    #error "Unknown target. Please define one of the TARGET_ preprocessor defs."
#endif


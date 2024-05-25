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

// defined if ZX0 is used on this platform
#define USES_ZX0

#define COPYRIGHT "" // a string containin just the copyright character

#define LOADMODE LM_MALLOC // must be equal to LM_MALLOC or LM_STATIC. malloc will allocate space with malloc before loading a file. static assumes there enough space statically allocated

// required if LOADMODE is LM_STATIC
#define EVENTS_BASE
#define STRINGS_BASE
#define MAP_BASE
// TODO document the other map bases

// names of the various module files
#define FILE_MAP "mapzx.bin"
#define FILE_EVENTS "evlist.bin"
#define FILE_STRINGS "strngs.bin"

// optional, defines a cleanup function to be registered with atexit()
#define ATEXIT cleanup
// optional, defines a function to be registered as an interrupt handler
#define INTERRUPT interrupt


// The value that the "£" (pound) symbol will have once decoded with the c_lut (character lookup table) file.
#define GBP_CHAR '\x60'
// The string to use in place of GBP_CHAR (could just be the same char again)
#define GBP "\x60"

// Define this if GBP is equal to GBP_CHAR (there's no good way to automate this in the preprocessor)
// This removes some code from the decoder path, making it a little faster
#define GBP_CHAR_NOTRANSFORM

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
    extern void *end_of_program;
    #include "mapdata.h"
    #define EVENTS_BASE (map_base + map_len)
    #define C_LUT_BASE (events_base + events_len)
    #define STRINGS_BASE (c_lut_base + c_lut_len)
    #define DESCS_BASE (strings_base + strings_len)
    #define USES_ZX0
    extern void dzx0_standard(unsigned char *src, unsigned char *dst);
    #define COPYRIGHT "\x7f"
    #define GBP_CHAR '\x60'
    #define GBP "\x60"
    #define GBP_CHAR_NOTRANSFORM
    #define LOADMODE LM_STATIC
    #define FILE_MAP "mapzx.bin"
    #define FILE_EVENTS "evlist.bin"
    #define FILE_C_LUT "c_lut.bin"
    #define FILE_STRINGS "strngs.bin"
    #define FILE_DESCS "desc0.bin"
    #define FILE_DESCS_ID_CHAR (4)
    #define DESCR_BITS (14)
    #define NEWLINE "\r"

    #define SCREEN_WIDTH (32)

    #define FG_BLACK_LEN (2)
    #define BG_BLACK_LEN (2)
    #define BG_BLACK "\x11\x00"
    #define BG_BLUE "\x11\x01"
    #define BG_RED "\x11\x02"
    #define BG_FUCHSIA "\x11\x03"
    #define BG_GREEN "\x11\x04"
    #define BG_CYAN "\x11\x05"
    #define BG_YELLOW "\x11\x06"
    #define BG_WHITE "\x11\x07"

    #define FG_BLACK "\x10\x00"
    #define FG_BLUE "\x10\x01"
    #define FG_RED "\x10\x02"
    #define FG_FUCHSIA "\x10\x03"
    #define FG_GREEN "\x10\x04"
    #define FG_CYAN "\x10\x05"
    #define FG_YELLOW "\x10\x06"
    #define FG_WHITE "\x10\x07"
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
    #include <sys/ioctl.h>
    #define MAP_BASE (map_full)
    #define MAP_NORTH_BASE (map_north)
    #define MAP_SOUTH_BASE (map_south)
    #define MAIN_CAN_RETURN
    #define COPYRIGHT "\xc2\xa9"
    // This is the ISO-8859-1 encoding of the pound symbol
    // it's not actually printed, only used as a placeholder value for identfying the pound in the custom encoding, so it's fine that this is not utf-8
    #define GBP_CHAR (0xa3)
    // This _is_ utf-8 tho
    #define GBP "\xc2\xa3"
    #define LOADMODE LM_MALLOC
    #define FILE_MAP "map.png"
    #define FILE_EVENTS "evlist_default.bin"
    #define FILE_C_LUT "c_lut_default.bin"
    #define FILE_STRINGS "strings_default.bin"
    #define FILE_DESCS "descriptions0.bin"
    #define FILE_DESCS_ID_CHAR (12)
    #define DESCR_BITS (24)
    #define ATEXIT cleanup
    #define INTERRUPT interrupt
    #define NEWLINE "\n"
    #define AUTOLOAD_DESC0

    #define SCREEN_WIDTH (get_term_width())

    #define FG_BLACK_LEN (5)
    #define BG_BLACK_LEN (5)

    #define FG_BLACK "\x1b[30m"
    #define FG_RED "\x1b[31m"
    #define FG_GREEN "\x1b[32m"
    #define FG_YELLOW "\x1b[33m"
    #define FG_BLUE "\x1b[34m"
    #define FG_FUCHSIA "\x1b[35m"
    #define FG_CYAN "\x1b[36m"
    #define FG_WHITE "\x1b[37m"

    #define BG_BLACK "\x1b[40m"
    #define BG_RED "\x1b[41m"
    #define BG_GREEN "\x1b[42m"
    #define BG_YELLOW "\x1b[43m"
    #define BG_BLUE "\x1b[44m"
    #define BG_FUCHSIA "\x1b[45m"
    #define BG_CYAN "\x1b[46m"
    #define BG_WHITE "\x1b[47m"

#endif

#ifdef TARGET_PC_MSDOS
    #define __TARGET_KNOWN
    #define STORAGE_MEDIUM "disk"
    #define ENTER_KEY ('\r')
    #define BACKSPACE_KEY (8)
    #define MAX_PRINTABLE (0x7F)
    #define BACKSPACE_NAME "Backspace"
    #define BOXDRAW_BR "-"
    #define BOXDRAW_BL "-"
    #define BOXDRAW_TR "-"
    #define BOXDRAW_TL "-"
    #define BOXDRAW_L "|"
    #define BOXDRAW_R "|"
    #define BOXDRAW_B "-"
    #define BOXDRAW_T "-"
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <conio.h>
    #include <i86.h>
    #include <dos.h>
    #define MAP_BASE (map_full)
    #define MAP_NORTH_BASE (map_north)
    #define MAP_SOUTH_BASE (map_south)
    #define MAIN_CAN_RETURN
    #define COPYRIGHT "(C)"
    // This is the ISO-8859-1 encoding of the pound symbol
    // it's not actually printed, only used as a placeholder value for identfying the pound in the custom encoding, so it's fine that this is not utf-8
    #define GBP_CHAR (0xa3)
    #define GBP "\x9c"
    #define LOADMODE LM_MALLOC
    #define FILE_MAP "map.png"
    #define FILE_EVENTS "evMSD.bin"
    #define FILE_C_LUT "clMSD.bin"
    #define FILE_STRINGS "sMSD.bin"
    #define FILE_DESCS "dMSD0.bin"
    #define FILE_DESCS_ID_CHAR (4)
    #define DESCR_BITS (24)
    #define NEWLINE "\n"
    #define AUTOLOAD_DESC0

    #define SCREEN_WIDTH (get_term_width())

    #define FG_BLACK_LEN (5)
    #define BG_BLACK_LEN (5)

    #define FG_BLACK "\x1b[30m"
    #define FG_RED "\x1b[31m"
    #define FG_GREEN "\x1b[32m"
    #define FG_YELLOW "\x1b[33m"
    #define FG_BLUE "\x1b[34m"
    #define FG_FUCHSIA "\x1b[35m"
    #define FG_CYAN "\x1b[36m"
    #define FG_WHITE "\x1b[37m"

    #define BG_BLACK "\x1b[40m"
    #define BG_RED "\x1b[41m"
    #define BG_GREEN "\x1b[42m"
    #define BG_YELLOW "\x1b[43m"
    #define BG_BLUE "\x1b[44m"
    #define BG_FUCHSIA "\x1b[45m"
    #define BG_CYAN "\x1b[46m"
    #define BG_WHITE "\x1b[47m"

#endif




#ifdef TARGET_PC_MSDOS_TEXT
    #define __TARGET_KNOWN
    #define LINEAR_TEXT
    #define STORAGE_MEDIUM "disk"
    #define ENTER_KEY ('\r')
    #define BACKSPACE_KEY (8)
    #define MAX_PRINTABLE (0x7F)
    #define BACKSPACE_NAME "Backspace"
    #define BOXDRAW_BR "-"
    #define BOXDRAW_BL "-"
    #define BOXDRAW_TR "-"
    #define BOXDRAW_TL "-"
    #define BOXDRAW_L "|"
    #define BOXDRAW_R "|"
    #define BOXDRAW_B "-"
    #define BOXDRAW_T "-"
    #include <string.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <conio.h>
    #include <i86.h>
    #include <dos.h>
    #define MAP_BASE (map_full)
    #define MAP_NORTH_BASE (map_north)
    #define MAP_SOUTH_BASE (map_south)
    #define MAIN_CAN_RETURN
    #define COPYRIGHT "(C)"
    // This is the ISO-8859-1 encoding of the pound symbol
    // it's not actually printed, only used as a placeholder value for identfying the pound in the custom encoding, so it's fine that this is not utf-8
    #define GBP_CHAR (0xa3)
    // This _is_ utf-8 tho
    #define GBP "\x9c"
    #define LOADMODE LM_MALLOC
    #define FILE_MAP "map.png"
    #define FILE_EVENTS "evMSD.bin"
    #define FILE_C_LUT "clMSD.bin"
    #define FILE_STRINGS "sMSD.bin"
    #define FILE_DESCS "dMSD0.bin"
    #define FILE_DESCS_ID_CHAR (4)
    #define DESCR_BITS (24)
    #define NEWLINE "\n"
    #define AUTOLOAD_DESC0

    #define SCREEN_WIDTH (get_term_width())

    #define FG_BLACK_LEN (0)
    #define BG_BLACK_LEN (0)

    #define FG_BLACK ""
    #define FG_RED ""
    #define FG_GREEN ""
    #define FG_YELLOW ""
    #define FG_BLUE ""
    #define FG_FUCHSIA ""
    #define FG_CYAN ""
    #define FG_WHITE ""

    #define BG_BLACK ""
    #define BG_RED ""
    #define BG_GREEN ""
    #define BG_YELLOW ""
    #define BG_BLUE ""
    #define BG_FUCHSIA ""
    #define BG_CYAN ""
    #define BG_WHITE ""

#endif



// To port this to another system, add extra TARGET option checks here

#ifndef __TARGET_KNOWN
    #error "Unknown target. Please define one of the TARGET_ preprocessor defs."
#endif


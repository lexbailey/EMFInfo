

#ifdef TARGET_ZXSPEC48
#define __TARGET_KNOWN
#ifndef __SDCC_z80
#error "The ZX Spectrum 48k target must be compiled with -mz80"
#endif
#define LAST_K (*((char*)(23560)))
#endif

#ifdef TARGET_PC_LINUX
#define __TARGET_KNOWN
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#endif

// To port this to another system, add extra TARGET option checks here

#ifndef __TARGET_KNOWN
#error "Unknown target. Please define one of the TARGET_ preprocessor defs."
#endif

#define MODE_MAIN_MENU (0)
#define MODE_TIMETABLE (1)
#define MODE_MAP (2)
#define MODE_TIMETABLE_LIST (3)

#include "schedule.h"

char filt_day = 0x0;
char filt_type = 0xff;
char filt_title = 0;

#ifdef TARGET_ZXSPEC48
int strlen(char *t){
    int result = 0;
    while (*t++ != '\0'){
        result += 1;
    }
    return result;
}
#endif

#ifdef TARGET_PC_LINUX
void cleanup(){
    printf("\033[?25h");
    printf("\033[?1049l");
}

void interrupt(int sig){
    if (sig == SIGINT){
        exit(1);
    }
}
#endif

char mode = MODE_MAIN_MENU;

void curpos(char x, char y){
    #ifdef TARGET_ZXSPEC48
    x;y;
    //x will be in a
    //y will be in l
    __asm__(
        "push af \n\t"
        "ld a, #0x16 \n\t"
        "rst #0x10 \n\t"
        "ld a, l \n\t"
        "rst #0x10 \n\t"
        "pop af \n\t"
        "rst #0x10 \n\t"
    );
    #endif
    #ifdef TARGET_PC_LINUX
    printf("\033[%d;%dH", y+1, x+1);
    #endif
}

void clear(){
    #ifdef TARGET_ZXSPEC48
    __asm__(
        "call 0x0DAF\n\t"
    );
    #endif
    #ifdef TARGET_PC_LINUX
    printf("\033[2J");
    curpos(0,0);
    // this frame marks the edges of the ZX Spectrum screen (32x24 chars)
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("                              |\n");
    printf("-------------------------------\n");
    curpos(0,0);
    #endif
}

void init_text(){
    #ifdef TARGET_ZXSPEC48
    __asm__(
        "ld a, #2 \n\t"
        "call #0x1601 \n\t"
    );
    #endif
    #ifdef TARGET_PC_LINUX
    struct termios t;
    tcgetattr(fileno(stdin), &t);
    t.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &t);
    printf("\033[?1049h");
    printf("\033[?25l");
    #endif
}

#ifdef TARGET_ZXSPEC48
void text_zxspec48(char *t, int len){
    t;len;
    __asm__(
        "ld b, d \n\t"
        "ld c, e \n\t"
        "ld d, h \n\t"
        "ld e, l \n\t"
        "call #0x203c \n\t"
    );
}
#endif

void text(char *t){
    #ifdef TARGET_ZXSPEC48
    int l = strlen(t);
    text_zxspec48(t, l);
    #endif
    #ifdef TARGET_PC_LINUX
    printf("%s", t);
    #endif
}

volatile char n2[5];
void num_text(int n){
    #ifdef TARGET_ZXSPEC48
    __asm__(
        "push de\n\t"
        "push bc\n\t"
        // hl is where n is stored
        // copy it to de
        "ld d, h\n\t"
        "ld e, l\n\t"
        // let hl point to the n2 array
        "ld hl, #_n2\n\t"
        // First byte is 0 to indicate integer number
        "ld (hl), #0\n\t"
        // Next byte is 0 to indicate positive number
        "inc hl\n\t"
        "ld (hl), #0\n\t"
        // Next two bytes are the number, from de
        "inc hl\n\t"
        "ld (hl), e\n\t"
        "inc hl\n\t"
        "ld (hl), d\n\t"
        // be sure the last one is 0
        "inc hl\n\t"
        "ld (hl), #0\n\t"
        // restore pointer to n2
        "ld hl, #_n2\n\t"
        // Copy to calculator stack
        "call #0x33b4\n\t"
        // print top of calc stack to screen
        "call #0x2de3\n\t"
        "pop bc\n\t"
        "pop de\n\t"
    );
    #endif
    #ifdef TARGET_PC_LINUX
    printf("%d", n);
    #endif
}

char get_key_press(){
    #ifdef TARGET_ZXSPEC48
    while(1){
        char k = LAST_K;
        if (k != 255){
            LAST_K = 255;
            return k;
        }
    }
    #endif
    #ifdef TARGET_PC_LINUX
    return getc(stdin);
    #endif
    return '\0';
}

char menu(int changed, char key){
    if (changed){
        clear();
        curpos(6,0);
        text("EMF Info Main Menu");
        curpos(1,2);
        text("T - Timetable");
        curpos(1,3);
        text("M - Map");
    }
    if (key == 'T' || key == 't'){
        return MODE_TIMETABLE;
    }
    if (key == 'M' || key == 'm'){
        return MODE_MAP;
    }
    return MODE_MAIN_MENU;
}

char timetable_list(int changed, char key){
    static int page = 1;
    static int n_pages = 1;
    if (changed){
        n_pages = SCHED_N_EVENTS/10;
        clear();
        curpos(0,0);
        text("EMF Timetable (");
        if (filt_day == 0){
            text("all days");
        }
        else {
            text("day");
            num_text(filt_day);
        }
        curpos(0,1);
        text("Pg ");
        num_text(page);
        text("/");
        num_text(n_pages);
        for (char i = 0; i < 10; i++){
            curpos(0,2+(i*2));
            num_text(i);
            text(":");
        }
        curpos(10,1);
        text("N:Next,P:Prev,Q:Back");
    }
    if (key == 'N' || key == 'n'){
        page += 1;
        if (page > n_pages){
            page = 1;
        }
        timetable_list(1,'\0');
    }
    if (key == 'P' || key == 'p'){
        page -= 1;
        if (page < 1){
            page = n_pages;
        }
        timetable_list(1,'\0');
    }
    if (key == 'Q' || key == 'q'){ return MODE_TIMETABLE; }
    return MODE_TIMETABLE_LIST;
}

char timetable(int changed, char key){
    if (changed){
        clear();
        curpos(6,0);
        text("EMF Timetable");
        curpos(9,2);
        num_text(SCHED_N_EVENTS);
        text(" events");
        curpos(1,4);
        text("A - Show All");
        curpos(1,5);
        text("D - By Day");
        curpos(1,6);
        text("S - Search");
        curpos(1,20);
        text("Q - Main menu");
    }

    if (key == 'A' || key == 'a'){
        filt_day = 0x0;
        filt_type = 0xff;
        filt_title = 0;
        return MODE_TIMETABLE_LIST;
    }
    if (key == 'Q' || key == 'q'){
        return MODE_MAIN_MENU;
    }
    return MODE_TIMETABLE;
}

char map(int changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("      EMF Map");
        curpos(1,20);
        text("Q - Main menu");
    }
    if (key == 'Q' || key == 'q'){
        return MODE_MAIN_MENU;
    }
    return MODE_MAP;
}

int main(){
    #ifdef TARGET_PC_LINUX
    atexit(cleanup);
    signal(SIGINT, interrupt);
    #endif
    init_text();
    int lastmode = -1;
    while (1){
        int changed = lastmode != mode;
        lastmode = mode;
        char c = '\0';
        if (!changed){
            c = get_key_press();
        }
        switch (mode){
            case MODE_MAIN_MENU: mode = menu(changed, c); break;
            case MODE_TIMETABLE: mode = timetable(changed, c); break;
            case MODE_MAP: mode = map(changed, c); break;
            case MODE_TIMETABLE_LIST: mode = timetable_list(changed, c); break;
        }
    }
}

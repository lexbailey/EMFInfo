

#ifdef TARGET_ZXSPEC48
#define __TARGET_KNOWN
#ifndef __SDCC_z80
#error "The ZX Spectrum 48k target must be compiled with -mz80"
#endif
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

#ifdef TARGET_ZX_SPEC48
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

char get_key_press(){
    #ifdef TARGET_ZXSPEC48
    #endif
    #ifdef TARGET_PC_LINUX
    return getc(stdin);
    #endif
    return '\0';
}

char menu(int changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("      EMF Info Main Menu");
        curpos(0,2);
        text(" T - Timetable");
        curpos(0,3);
        text(" M - Map");
    }
    if (key == 'T' || key == 't'){
        return MODE_TIMETABLE;
    }
    if (key == 'M' || key == 'm'){
        return MODE_MAP;
    }
    return MODE_MAIN_MENU;
}

char timetable(int changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("      EMF Timetable");
    }
    if (key == 'Q' || key == 'q'){
        return MODE_MAIN_MENU;
    }
    return MODE_TIMETABLE;
}

char map(int changed, char key){
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
        }
    }
}



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
#include <stdint.h>
#endif

// To port this to another system, add extra TARGET option checks here

#ifndef __TARGET_KNOWN
#error "Unknown target. Please define one of the TARGET_ preprocessor defs."
#endif

#define MODE_MAIN_MENU (0)
#define MODE_TIMETABLE (1)
#define MODE_MAP (2)
#define MODE_TIMETABLE_LIST (3)
#define MODE_EVENT_DETAIL (5)

#define MODE_EXIT (-1)

#define EVTYPE_TALK (0)
#define EVTYPE_PERFORMANCE (1)
#define EVTYPE_WORKSHOP (2)
#define EVTYPE_YOUTHWORKSHOP (3)

char *evstr[] = {
    "Talk", "Performance", "Workshop", "Youth Workshop"
};

#include "schedule.h"

char filt_day = 0x0;
char filt_type = 0xff;
char filt_title = 0;

int evs_loaded = 0;
int strings_loaded = 0;

typedef struct {
    unsigned char type;
    unsigned char can_record;
    unsigned int time;
    unsigned char duration;
    char *title;
    char *venue;
    char *name;
    char *pronouns;
    char *cost;
    char *descr;
} event_t;

#ifdef TARGET_ZXSPEC48
char *events_base = (char *)0x9000;
char *strings_base = 0x9000 + EVENTS_LEN;
#endif
#ifdef TARGET_PC_LINUX
uint8_t events_base[EVENTS_LEN];
uint8_t strings_base[STRINGS_LEN];
#endif

#ifdef TARGET_PC_LINUX
int load_data(void* p, size_t len, char *fname){
    FILE *f = fopen(fname, "rb");
    if (f == NULL){
        return 0;
    }
    size_t r = fread(p, 1, len, f);
    if (r != len){
        return 0;
    }
    return 1;
}
#endif

unsigned int bitstream_get(char **ptr, signed char *bpos, char bits){
    char rembits = bits;
    unsigned int outval = 0;
    while (rembits) {
        outval = (outval << 1) | (*ptr[0] >> *bpos & 1);
        *bpos -= 1;
        if (*bpos < 0){
            *bpos = 7;
            *ptr += 1;
        }
        rembits --;
    }
    return outval;
}

event_t get_event(int index){
    #define CBITS(n) ((char)(bitstream_get(&p, &bpos, n)))
    #define IBITS(n) (bitstream_get(&p, &bpos, n))
    char str_bit_len = events_base[0];
    char big_str_bit_len = events_base[0]; // todo, pack this elsewhere
    char *p = events_base + 1 + (index * EV_SIZE);
    char bpos = 7;
    event_t ev;
    ev.type = CBITS(2);
    ev.can_record = CBITS(1);
    ev.time = IBITS(13);
    ev.duration = IBITS(8);
    ev.title = strings_base + IBITS(str_bit_len);
    ev.venue = strings_base + IBITS(str_bit_len);
    ev.name = strings_base + IBITS(str_bit_len);
    ev.pronouns = strings_base + IBITS(str_bit_len);
    ev.cost = strings_base + IBITS(str_bit_len);
    ev.descr = strings_base + IBITS(big_str_bit_len);
    return ev;
    #undef CBITS
    #undef IBITS
}

#ifdef TARGET_ZXSPEC48
int load_data(void* p, unsigned int len){
    p; // hl contains p
    len; // de contains len
    __asm__(
        "push ix\n\t"
        "ld a, #0xff\n\t" // loading a block
        "scf\n\t" // set carry flag to indicate load (not verify)
        "push hl\n\t" // start addr into ix
        "pop ix\n\t"
        "call 0x0556\n\t"
        "pop ix\n\t"
        "ld de, #1\n\t"
        "jr c, loadsuccess\n\t"
        "ld de, #0\n\t"
        "loadsuccess:\n\t"
        "ret\n\t"
    );
    return 0;
}
#endif

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
    n;
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
    //return '\0';
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
    #ifdef TARGET_PC_LINUX
    if (key == 'Q' || key == 'q'){
        printf("exit now\n");
        return MODE_EXIT;
    }
    #endif
    return MODE_MAIN_MENU;
}

void time_text(unsigned int time){
}

void duration_text(unsigned char dur){
    char hr = 0;
    while (dur >= 60){
        dur -= 60;
        hr += 1;
    }
    num_text(hr);
    text("hrs");
    if (dur > 0){
        text(",");
        num_text((char)dur);
        text("mins");
    }
}

int ev_id = 0;

char event_detail(int changed, char key){
    if (changed) {
        clear();
        event_t ev;
        ev = get_event(ev_id); // can't do this on the same as the above line, SDCC bug #3121
        curpos(0,0);
        text(evstr[ev.type]);
        text(":");
        text(ev.title);

        curpos(0,2);
        text("By ");
        text(ev.name);
        curpos(0,3);
        text("(");
        text(ev.pronouns);
        text(")");

        curpos(0,4);
        text("At ");
        text(ev.venue);

        curpos(0,5);
        text("From ");
        time_text(ev.time);
        text(" for ");
        duration_text(ev.duration);

        if (ev.type != EVTYPE_TALK){
            curpos(0,6);
            text("Cost: ");
            text(ev.cost);
        }

    }
    if (key == 'Q' || key == 'q'){ return MODE_TIMETABLE_LIST; }
    return MODE_EVENT_DETAIL;
}

char timetable_list(int changed, char key){
    static int page = 1;
    static int n_pages = 1;
    if (changed){
        n_pages = (SCHED_N_EVENTS/10)+1;
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
            int index = ((page-1) * 10) + i;
            if (index >= SCHED_N_EVENTS){
                break;
            }
            event_t ev;
            ev = get_event(index); // can't do this on the same as the above line, SDCC bug #3121
            curpos(0,2+(i*2));
            num_text(i);
            text(":");
            text(ev.title);
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
    if (key >= '0' && key <= '9'){
        ev_id = ((page-1) * 10) + key - '0';
        return MODE_EVENT_DETAIL;
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
    init_text();
    clear();
    #ifdef TARGET_PC_LINUX
        atexit(cleanup);
        signal(SIGINT, interrupt);
        evs_loaded = load_data(events_base, EVENTS_LEN, "evlist.bin");
        strings_loaded = load_data(strings_base, STRINGS_LEN, "strngs.bin");
        if (!evs_loaded || !strings_loaded){
            curpos(0,0);
            text("Error: Unable to load data");
        }
    #endif
    #ifdef TARGET_ZXSPEC48
        evs_loaded = load_data(events_base, EVENTS_LEN);
        if (! evs_loaded){
            curpos(0,0);
            text("Error loading events");
            curpos(0,1);
            text("will retry later...");
        }
        strings_loaded = load_data(strings_base, STRINGS_LEN);
        if (! strings_loaded){
            curpos(0,2);
            text("Error loading strings");
            curpos(0,3);
            text("will retry later...");
        }
    #endif
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
            case MODE_EVENT_DETAIL: mode = event_detail(changed, c); break;
        }
        #ifdef TARGET_PC_LINUX
            if (mode == MODE_EXIT){
                return 0;
            }
        #endif
    }
}

/*
int main(){
	init_text();
	clear();
	curpos(0,0);
	text("Hello world! ");
	num_text(1234);
	curpos(0,1);
	text("Press a key");
	curpos(0,2);
	char c = get_key_press();
	text("you pressed: ");
	char cs[2];
	cs[0] = c;
	cs[1] = 0;
	text(cs);
	return 0;
}
*/

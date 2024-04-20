

#ifdef TARGET_ZXSPEC48
#define __TARGET_KNOWN
#ifndef __SDCC_z80
#error "The ZX Spectrum 48k target must be compiled with -mz80"
#endif
#define STORAGE_MEDIUM "tape"
#define LAST_K (*((char*)(23560)))
#pragma disable_warning 84
#pragma disable_warning 85
#endif

#ifdef TARGET_PC_LINUX
#define __TARGET_KNOWN
#define STORAGE_MEDIUM "disk"
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

#define EVTYPE_TALK (0)
#define EVTYPE_PERFORMANCE (1)
#define EVTYPE_WORKSHOP (2)
#define EVTYPE_YOUTHWORKSHOP (3)

typedef void (*(*uifunc)(char, char))(); //close enough. I technically can't actually write the type I want, but I can aggressively cast to this approximation

uifunc menu(char, char);
uifunc event_detail(char, char);
uifunc timetable_list(char, char);
uifunc timetable(char, char);
uifunc terminate(char, char);
uifunc map(char, char);
uifunc load_evs(char, char);

uifunc mode = (uifunc)menu;

char *evstr[] = {
    "Talk", "Performance", "Workshop", "Youth Workshop"
};

char *daynames[] = {
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

unsigned int events_per_day[] = {0,0,0,0,0,0,0};

unsigned int num_pages;
unsigned int page_starts[50];

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
char *events_base = (char *)0xA000;
char *strings_base = 0;
unsigned int events_len = 0;
unsigned int strings_len = 0;
#endif
#ifdef TARGET_PC_LINUX
uint8_t *events_base;
uint8_t *strings_base;
size_t events_len = 0;
size_t strings_len = 0;
#endif
unsigned int num_events = 0;
unsigned int ev_size = 0;
char str_bit_len = 0;
char day0_index = 0; //what day of the week (monday=0) is the one that the epoch starts on?

#include "intmath.c"
#include "bitstream_parse.c"
#include "text_render.c"

event_t get_event(unsigned int index){

    char big_str_bit_len = events_base[2]; // todo, pack this elsewhere
    char *p = events_base + 5 + mul(index, ev_size);
    BITSTREAM_INIT(p)
    event_t ev;
    ev.type = CBITS(2);
    ev.can_record = CBITS(1);
    ev.time = IBITS(13);
    ev.duration = IBITS(8);
    /*ev.filtered = */CBITS(1);
    ev.title = PBITS(str_bit_len);
    ev.venue = PBITS(str_bit_len);
    ev.name = PBITS(str_bit_len);
    ev.pronouns = PBITS(str_bit_len);
    ev.cost = PBITS(str_bit_len);
    ev.descr = strings_base + IBITS(big_str_bit_len); // TODO this is wrong
    #ifdef TARGET_ZXSPEC48
        char **s = &ev.title;
        for (unsigned char x = 5; x>0;x--){
            *s += (unsigned int)strings_base;
            s++;
        }
    #endif
    return ev;
}

// similar to the above, but specifically for the time field, because we need to be able to
// quickly filter based on that
unsigned int get_event_time(unsigned int index){
    char *p = events_base + 5 + mul(index, ev_size);
    BITSTREAM_INIT(p)
    CBITS(3);
    unsigned int time = IBITS(13);
    return time;
}

// finds the filter bit for the given event index
signed char *filt_ptr(unsigned int index){ return events_base + 8 + mul(index, ev_size); }
// returns a negative number if the event is filtered out, otherwise returns a positive number
signed char filt_get(unsigned int index){ return *filt_ptr(index); }
// sets the filter bit (to filter out the event)
void filt_set(unsigned int index){ *filt_ptr(index) |= 0x80; }
// clears the filter bit
void filt_clear(unsigned int index){ *filt_ptr(index) &= 0x7f; }

void parse_ev_file_consts(){
    num_events = (unsigned int)events_base[0] | (((unsigned int)events_base[1])<<8);
    str_bit_len = events_base[2];
    ev_size = (unsigned int)events_base[3];
    day0_index = events_base[4];

    for (char x = 0; x <7; x++){
        events_per_day[x] = 0;
    }
    // also get events per day info
    for (int i = 0; i<num_events; i++){
        unsigned int t = get_event_time(i);
        char d = day0_index;
        while (t >= (24*60)){
            t-=(24*60);
            d += 1;
            if (d >= 7){
                d = 0;
            }
        }
        events_per_day[d]++;
    }
}

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


#ifdef TARGET_ZXSPEC48
    int load_recoverable(unsigned char type, void* p, unsigned int len) __naked{
        p; // de
        len; // stack
        type; // is in a already. good
        __asm__(
            "push ix\n\t"
            "push de\n\t"
            "pop ix\n\t"
            "inc sp\n\t"
            "inc sp\n\t"
            "inc sp\n\t"
            "inc sp\n\t"
            "pop de\n\t"
            "dec sp\n\t"
            "dec sp\n\t"
            "dec sp\n\t"
            "dec sp\n\t"
            "dec sp\n\t"
            "dec sp\n\t"
            "scf\n\t" // set carry flag to indicate load (not verify)
            // some fuckery to make the loader routine not call the error handler
            "inc d\n\t"
            "ex af,af'\n\t"
            "dec d\n\t"
            "di\n\t"
            "call 0x0562\n" // calling into the middle of the loader function intentionally, skipping the tail-call like error handler
            "push af\n\t"// stash carry flag
            // restore the border
            "ld a,(0x5C48)\n\t"
            "and #0x38\n\t"
            "rrca\n\t"
            "rrca\n\t"
            "rrca\n\t"
            "out (0xfe),a\n\t"

            // check break key
            "ld a,#0x7f\n\t"
            "in a,(0xfe)\n\t"
            "rra\n\t"
            "ei\n\t"

            // Determine why the loader stopped and return accordingly
            "jr c,#load_custom_ret_end\n\t"
            "ld de, #2\n\t"
            "pop af\n\t"
            "pop ix\n\t"
            "jr load_custom_done\n"
            "load_custom_ret_end:\n\t"
            "pop af\n\t"
            "pop ix\n\t"
            "ld de, #1\n\t"
            "jr c, db_loadsuccess\n\t"
            "ld de, #0\n"
            "db_loadsuccess:\n\t"
            "load_custom_done:\n\t"
            "inc sp\n\t"
            "inc sp\n\t"
            "inc sp\n\t"
            "inc sp\n\t"
            "ret\n\t"
        );
        return 0;
    }

    int load_data_block(void* p, unsigned int len){
        return load_recoverable(0xff,p,len);
    }
    
    int load_header(void* p){
        return load_recoverable(0x00,p,17);
    }
    
    int load_data(void *p, unsigned int *len, char *name){
        noscroll();
        text("\r");
        while (1){
            int ret = load_header(p);
            if (ret == 2){
                return 2;
            }
            if (ret != 1){
                noscroll();
                text("skip\r");
                continue;
            }
            int rlen = ((int)(((char*)p)[11])) | (((int)(((char *)p)[12]))<<8);
            ((char*)p)[11] = '\0';
            char match = 1;
            for (int i = 0; i<strlen(name); i++){
                if (name[i] != ((char*)p)[1+i]){
                    match = 0;
                    break;
                }
            }
            if (!match){
                noscroll();
                text("skip: ");
                text(((char *)p)+1);
                text("\r");
                continue;
            }
            noscroll();
            text("found: ");
            text(((char *)p)+1);
            text("\r");
            *len = rlen;
            break;
        }
        return load_data_block(p, *len);
    }
#endif

#ifdef TARGET_PC_LINUX
    size_t flen(char *fname){
        FILE *f = fopen(fname, "rb");
        if (f == NULL){
            return 0;
        }
        if (fseek(f, 0, SEEK_END)){
            fclose(f);
            return 0;
        }
        size_t len = ftell(f);
        fclose(f);
        return len;
    }

    int load_data(void* p, size_t *len, char *fname){
        FILE *f = fopen(fname, "rb");
        if (f == NULL){
            return 0;
        }
        if (fseek(f, 0, SEEK_END)){
            fclose(f);
            return 0;
        }
        size_t a_len = ftell(f);
        if (*len > 0 && *len != a_len){
            fclose(f);
            return 0;
        } 
        if (fseek(f, 0, SEEK_SET)){
            fclose(f);
            return 0;
        }
        size_t r = fread(p, 1, *len, f);
        if (r != *len){
            fclose(f);
            return 0;
        }
        fclose(f);
        return 1;
    }
#endif

uifunc menu(char changed, char key){
    if (changed){
        clear();
        curpos(6,0);
        text("EMF Info Main Menu");
        curpos(1,2);
        text("T - Timetable");
        curpos(1,3);
        text("M - Map");
        if (!evs_loaded){
            curpos(1,4);
            text("E - Load event data from " STORAGE_MEDIUM);
        }
    }
    if (key == 'T' || key == 't'){
        return (uifunc)timetable;
    }
    if (key == 'M' || key == 'm'){
        return (uifunc)map;
    }
    if (!evs_loaded){
        if (key == 'E' || key == 'e'){
            return (uifunc)load_evs;
        }
    }
    #ifdef TARGET_PC_LINUX
    if (key == 'Q' || key == 'q'){
        printf("exit now\n");
        return (uifunc)terminate;
    }
    #endif
    return (uifunc)menu;
}

void time_text(unsigned int time){
    char day = day0_index;
    while (time >= (24*60)){
        time -= 24*60;
        if (day >= 6){
            day = 0;
        }
        else{
            day++;
        }
    }
    text(daynames[day]);
    text(" ");
    int hour = 0;
    while (time >= 60){
        time -= 60;
        hour += 1;
    }
    num_text_0pad2(hour);
    text(":");
    num_text_0pad2(time);
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

uifunc event_detail(char changed, char key){
    if (changed) {
        clear();
        event_t ev;
        ev = get_event(ev_id); // can't do this on the same as the above line, SDCC bug #3121
        curpos(0,0);
        text(evstr[ev.type]);
        text(":");
        truncated_text((32*3)-strlen(evstr[ev.type]),ev.title);

        curpos(0,3);
        text("By ");
        truncated_text(29,ev.name);
        curpos(0,4);
        text("(");
        truncated_text(30,ev.pronouns);
        text(")");

        curpos(0,5);
        text("At ");
        truncated_text(29, ev.venue);

        curpos(0,6);
        text("From ");
        time_text(ev.time);
        text(" for ");
        duration_text(ev.duration);

        curpos(0,7);
        text("Recording:");
        if (ev.can_record){
            text("yes");
        }
        else{
            text("no");
        }
        if (ev.type != EVTYPE_TALK){
            curpos(14,7);
            text("Cost: ");
            truncated_text(12,ev.cost);
        }

    }
    if (key == 'Q' || key == 'q'){ return (uifunc)timetable_list; }
    return (uifunc)event_detail;
}

void apply_filters(){
    clear();
    curpos(3,11);
    text("Filtering, please wait...");
    // by day
    unsigned int n_events = num_events;
    if (filt_day != 0xff){
        n_events = 0;
        signed char d = filt_day;
        d -= day0_index;
        unsigned char page_evs = 0;
        unsigned int page = 0;
        if (d<0){ d += 7; }
        for (unsigned int i = 0; i< num_events; i++){
            char ed = 0;
            unsigned int time = get_event_time(i);
            while (time >= (24*60)){
                ed += 1;
                time -= (24*60);
            }
            if (ed == d){
                filt_clear(i);
                n_events += 1;
                if (page_evs == 0){
                    page_starts[page] = i;
                    page ++;
                }
                page_evs += 1;
                if (page_evs == 10){
                    page_evs = 0;
                }
            }
            else{
                filt_set(i);
            }
        }
    }
    num_pages = div10(n_events)+1;
}

uifunc timetable_list(char changed, char key){
    static int page = 1;
    static char max_offset = 0;
    //static int n_pages = 1;
    if (changed){
        //n_pages = div10(num_events)+1;
        clear();
        curpos(0,0);
        text("EMF Timetable (");
        if (filt_day == 0xff){
            text("all days");
        }
        else {
            text(daynames[filt_day]);
        }
        text(")");
        curpos(0,1);
        text("Pg ");
        num_text(page);
        text("/");
        num_text(num_pages);
        curpos(12,1);
        text("N:Next,P:Prev,Q:Back");

        unsigned int index = page_starts[page-1];
        //for (char i = 0; i < 10; i++){
        char i = 0;
        max_offset = 0;
        while (i < 10){
            //unsigned int index = ((page-1) * 10) + i;
            if (filt_get(index) < 0){
                index++;
                continue;
            }
            if (index >= num_events){
                break;
            }
            event_t ev;
            ev = get_event(index); // can't do this on the same as the above line, SDCC bug #3121
            char line = 2+(i*2);
            curpos(0,line);
            num_text(i);
            text(":");
            truncated_text(30,ev.title);
            curpos(2,line+1);
            truncated_text(30,ev.name);
            i++;
            index++;
            max_offset ++;
        }
    }
    if (key == 'N' || key == 'n'){
        page += 1;
        if (page > num_pages){
            page = 1;
        }
        timetable_list(1,'\0');
    }
    if (key == 'P' || key == 'p'){
        page -= 1;
        if (page < 1){
            page = num_pages;
        }
        timetable_list(1,'\0');
    }
    if (key >= '0' && key <= '9'){
        unsigned int index = page_starts[page-1];
        unsigned char offset = key-'0';
        if (offset < max_offset){
            while (offset > 0){
                if (filt_get(index) < 0){
                    index++;
                    continue;
                }
                index++;
                offset--;
            }
            //ev_id = ((page-1) * 10) + key - '0';
            ev_id = index;
            return (uifunc)event_detail;
        }
    }
    if (key == 'Q' || key == 'q'){
        page = 1;
        return (uifunc)timetable;
    }
    return (uifunc)timetable_list;
}

uifunc daily_timetable(char changed, char key){
    if (changed){
        clear();
        curpos(6,0);
        text("EMF Timetable");
        curpos(9,2);
        num_text(num_events);
        text(" events");
        curpos(0,3);
        text("Filter by day");
        for (char i = 7; i>0; i--){
            curpos(1, 4+i);
            char d = i-1;
            int tot_today = events_per_day[d];
            if (tot_today > 0){
                num_text(d);
                text(" - ");
                text(daynames[d]);
                text(" (");
                num_text(tot_today);
                text(" events)");
            }
        }
        curpos(1,20);
        text("Q - Back");
    }
    if (key >= '0' && key <= '6'){
        filt_day = key-'0';
        filt_type = 0xff;
        filt_title = 0;
        apply_filters();
        return (uifunc)timetable_list;
    }
    if (key == 'Q' || key == 'q'){
        return (uifunc)timetable;
    }
    return (uifunc)daily_timetable;
}

uifunc timetable(char changed, char key){
    if (changed){
        clear();
        if (!evs_loaded){
            curpos(0,0);
            text("Error: Timetable is not loaded");
            curpos(0,1);
            text("Press Q, then load the");
            curpos(0,2);
            text("timetable and try again.");
        }
        else{
            curpos(6,0);
            text("EMF Timetable");
            curpos(9,2);
            num_text(num_events);
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
    }

    if (evs_loaded){
        // can only open a list view if the events are loaded
        if (key == 'A' || key == 'a'){
            filt_day = 0xff;
            filt_type = 0xff;
            filt_title = 0;
            apply_filters();
            return (uifunc)timetable_list;
        }
        if (key == 'D' || key == 'd'){
            return (uifunc)daily_timetable;
        }
    }
    if (key == 'Q' || key == 'q'){
        return (uifunc)menu;
    }
    return (uifunc)timetable;
}

uifunc terminate(char changed, char key){
}

uifunc map(char changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("      EMF Map");
        curpos(1,20);
        text("Q - Main menu");
    }
    if (key == 'Q' || key == 'q'){
        return (uifunc)menu;
    }
    return (uifunc)map;
}

uifunc load_evs(char changed, char key){
    #ifdef TARGET_PC_LINUX
        events_len = flen("evlist.bin");
        evs_loaded = load_data(events_base, &events_len, "evlist.bin") == 1;
        if (evs_loaded){
            parse_ev_file_consts();
        }
        return (uifunc)menu;
    #endif
    #ifdef TARGET_ZXSPEC48
        if (changed){
            clear();
            curpos(0,0);
            text("Cue tape to evlist.bin");
            curpos(0,1);
            text("and press play.");
            curpos(0,5);
            text("To cancel press break.");
            evs_loaded = load_data(events_base, &events_len, "evlist.bin") == 1;
            if (evs_loaded){
                parse_ev_file_consts();
                return (uifunc)menu;
            }
            else{
                text("press Q to go back");
            }
        }
        if (key == 'Q' || key == 'q'){
            return (uifunc)menu;
        }
        return (uifunc)load_evs;
    #endif
}

int main(){
    init_text();
    clear();
    #ifdef TARGET_PC_LINUX
        atexit(cleanup);
        signal(SIGINT, interrupt);
        events_len = flen("evlist.bin");
        strings_len = flen("strngs.bin");
        events_base = malloc(sizeof(char) * events_len);
        strings_base = malloc(sizeof(char) * strings_len);
        evs_loaded = load_data(events_base, &events_len, "evlist.bin") == 1;
        strings_loaded = load_data(strings_base, &strings_len, "strngs.bin") == 1;
        if (!evs_loaded || !strings_loaded){
            curpos(0,0);
            text("Error: Unable to load data");
            perror("a");
            curpos(0,5);
            text("press a key to continue");
            get_key_press();
        }
        else{
            parse_ev_file_consts();
        }
    #endif
    #ifdef TARGET_ZXSPEC48
        evs_loaded = load_data(events_base, &events_len, "evlist.bin") == 1;
        if (! evs_loaded){
            curpos(0,0);
            text("Error loading events.");
            curpos(0,1);
            text("Please retry from menu.");
            curpos(0,5);
            text("press a key to continue");
            get_key_press();
        }
        else{
            parse_ev_file_consts();
            strings_base = events_base + events_len;
            strings_loaded = load_data(strings_base, &strings_len, "strngs.bin") == 1;
            if (! strings_loaded){
                curpos(0,2);
                text("Error loading strings.");
                curpos(0,3);
                text("Please retry from menu.");
                curpos(0,5);
                text("press a key to continue");
                get_key_press();
            }
        }
    #endif
    uifunc lastmode = (uifunc)-1;
    while (1){
        char changed = lastmode != mode;
        lastmode = mode;
        char c = '\0';
        if (!changed){
            c = get_key_press();
        }
        mode = (uifunc)mode(changed, c);
        #ifdef TARGET_PC_LINUX
            if (mode == (uifunc)terminate){
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



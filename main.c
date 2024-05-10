#define LM_MALLOC (1)
#define LM_STATIC (2)

#include "target_defs.h"

#define EVTYPE_TALK (0)
#define EVTYPE_PERFORMANCE (1)
#define EVTYPE_WORKSHOP (2)
#define EVTYPE_YOUTHWORKSHOP (3)

#define EV_HEADER_LEN (6)
#define DESCR_BITS (14)
#define DECOMP_STR_MAX (500)

typedef void (*(*uifunc)(char, char))(); //close enough. I technically can't actually write the type I want, but I can aggressively cast to this approximation

uifunc menu(char, char);
uifunc event_detail(char, char);
uifunc timetable_list(char, char);
uifunc timetable(char, char);
uifunc daily_timetable(char, char);
uifunc search(char, char);
#ifdef MAIN_CAN_RETURN
    uifunc terminate(char, char);
#endif
uifunc map(char, char);
uifunc mapnorth(char, char);
uifunc mapsouth(char, char);
uifunc modules(char, char);
uifunc bugreport(char, char);
uifunc credits(char, char);

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

unsigned char filt_day = 0xff;
unsigned char filt_type = 0xff;
char *filt_text = 0;

unsigned char n_desc_modules = 0;

int map_loaded = 0;
int evs_loaded = 0;
int c_lut_loaded = 0;
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
    unsigned char descr_page;
    char *descr;
} event_t;

#include "target_vars.c"
unsigned int num_events = 0;
unsigned int ev_size = 0;
char str_bit_len = 0;
char day0_index = 0; //what day of the week (monday=0) is the one that the epoch starts on?
unsigned int filt_event_count = 0;
unsigned char descr_page_bits;

#include "intmath.c"
#include "bitstream_parse.c"
#include "text_render.c"
#include "image_render.c"
#include "file_io.c"

#if LOADMODE != LM_MALLOC
    #if LOADMODE != LM_STATIC
        #error No valid LOADMODE defined
    #endif
#endif

#ifndef CUSTOM_LOAD_MAP
    void load_map(){
        #if LOADMODE == LM_MALLOC
            map_len = flen(FILE_MAP);
            map_base = malloc(sizeof(char) * map_len);
        #else
            #if LOADMODE == LM_STATIC
                map_base = MAP_BASE;
            #endif
        #endif
        map_loaded = load_data(map_base, &map_len, FILE_MAP) == 1;
    }
#endif

#ifndef CUSTOM_LOAD_EVLIST
    void load_evlist(){
        #if LOADMODE == LM_MALLOC
            events_len = flen(FILE_EVENTS);
            events_base = malloc(sizeof(char) * events_len);
        #else
            #if LOADMODE == LM_STATIC
                events_base = EVENTS_BASE;
            #endif
        #endif
        evs_loaded = load_data(events_base, &events_len, FILE_EVENTS) == 1;
    }
#endif


#ifndef CUSTOM_LOAD_C_LUT
    void load_c_lut(){
        #if LOADMODE == LM_MALLOC
            c_lut_len = flen(FILE_C_LUT);
            c_lut_base = malloc(sizeof(char) * c_lut_len);
        #else
            #if LOADMODE == LM_STATIC
                c_lut_base = C_LUT_BASE;
            #endif
        #endif
        c_lut_loaded = load_data(c_lut_base, &c_lut_len, FILE_C_LUT) == 1;
    }
#endif


#ifndef CUSTOM_LOAD_STRING
    void load_strings(){
        #if LOADMODE == LM_MALLOC
            strings_len = flen(FILE_STRINGS);
            strings_base = malloc(sizeof(char) * strings_len);
        #else
            #if LOADMODE == LM_STATIC
                strings_base = STRINGS_BASE;
            #endif
        #endif
        strings_loaded = load_data(strings_base, &strings_len, FILE_STRINGS) == 1;
    }
#endif

char last_string[DECOMP_STR_MAX];

void decompress(char *s){
    char *d = last_string;
    int i = 0;
    unsigned char c = 0;
    BITSTREAM_INIT(s)
    while (i < DECOMP_STR_MAX-2){
        unsigned char index = 0;
        while (CBITS(1)){
            index += 1;
        }
        index = (index << 1) + (index << 2);
        unsigned char x = CBITS(1);
        if (x){
            index += 2;
            index += CBITS(2);
        }
        else{
            index += CBITS(1);
        }
        c = c_lut_base[index];
        if (c == '\0'){
            break;
        }
        // pound sign is special
        #ifndef GBP_CHAR_NOTRANSFORM
            if (c == GBP_CHAR){
              unsigned char y = (unsigned char)strlen(GBP);
              for (unsigned char j = 0; j < y; j++){
                  *d++ = GBP[j];
                  i++;
              }
            }
            else{
              *d++ = c;
            }
        #else
            *d++ = c;
        #endif
        i ++;
    }
    *d = '\0';
}

void dc_text(char *s){
    decompress(s);
    text(last_string);
}

void dc_truncated_text(unsigned char limit, char *s){
    decompress(s);
    truncated_text(limit, last_string);
}

event_t get_event(unsigned int index){
    char big_str_bit_len = events_base[2]; // todo, pack this elsewhere
    char *p = events_base + EV_HEADER_LEN + mul(index, ev_size);
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
    ev.descr_page = CBITS(descr_page_bits); // TODO this is wrong
    ev.descr = IBITS(DESCR_BITS); // TODO this is wrong
    char **s = &ev.title;
    for (unsigned char x = 5; x>0;x--){
        *s += (BITSTREAM_OUT_TYPE)strings_base;
        s++;
    }
    return ev;
}

// similar to the above, but specifically for the time field, because we need to be able to
// quickly filter based on that
unsigned int get_event_time(unsigned int index){
    char *p = events_base + EV_HEADER_LEN + mul(index, ev_size);
    BITSTREAM_INIT(p)
    CBITS(3);
    return IBITS(13);
}

unsigned char get_event_type(unsigned int index){
    char *p = events_base + EV_HEADER_LEN + mul(index, ev_size);
    BITSTREAM_INIT(p)
    return CBITS(2);
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
    n_desc_modules = events_base[5];
    descr_page_bits = 0;
    unsigned char nmod = n_desc_modules;
    while (nmod){
        nmod >>=1;
        descr_page_bits += 1;
    }

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

uifunc bugreport(char changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("EMF Info Bug reporting");
        curpos(0,2);
        text("Please report bugs on GitHub");
        curpos(0,4);
        text("github.com/lexbailey/EMFInfo");
        curpos(1,20);
        text("Q - Main menu");
    }
    if (key == 'q'){
        return (uifunc)menu;
    }
    return (uifunc)bugreport;
}

uifunc credits(char changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("EMF Info");
        curpos(0,2);
        text("Written by Lex Bailey");
        #ifdef USES_ZX0
            curpos(0,4);
            text("EMF Info uses ZX0 for compressed");
            curpos(0,5);
            text("map images.");
        #endif
        curpos(0,7);
        text("Map based on data " COPYRIGHT " Electromag-");
        curpos(0,8);
        text("netic Field and OpenStreetMap");
        curpos(0,9);
        text("contributers.");
        curpos(0,11);
        text("Timetable data " COPYRIGHT " Electromagnetic");
        curpos(0,12);
        text("Field.");
        curpos(1,20);
        text("Q - Main menu");
    }
    if (key == 'q'){
        return (uifunc)menu;
    }
    return (uifunc)credits;
}

uifunc menu(char changed, char key){
    if (changed){
        clear();
        curpos(6,0);
        text("EMF Info Main Menu");
        curpos(1,2);
        text("T - Timetable");
        curpos(1,3);
        text("M - Map");
        curpos(1,4);
        text("L - Load modules from " STORAGE_MEDIUM);
        curpos(1,5);
        text("B - Bug report");
        curpos(1,6);
        text("C - Credits");
        #ifdef MAIN_CAN_RETURN
            curpos(1,20);
            text("Q - Exit");
        #endif
    }
    if (key == 't'){
        return (uifunc)timetable;
    }
    if (key == 'm'){
        return (uifunc)map;
    }
    if (key == 'b'){
        return (uifunc)bugreport;
    }
    if (key == 'c'){
        return (uifunc)credits;
    }
    if (key == 'l'){
        return (uifunc)modules;
    }
    #ifdef MAIN_CAN_RETURN
    if (key == 'q'){
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
        dc_truncated_text((32*3)-strlen(evstr[ev.type]),ev.title);

        curpos(0,3);
        text("By ");
        dc_truncated_text(29,ev.name);
        curpos(0,4);
        text("(");
        dc_truncated_text(30,ev.pronouns);
        text(")");

        curpos(0,5);
        text("At ");
        dc_truncated_text(29, ev.venue);

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
            dc_truncated_text(12,ev.cost);
        }

    }
    if (key == 'q'){ return (uifunc)timetable_list; }
    return (uifunc)event_detail;
}

char is_substr(char *str, char *tofind){
    char *s = str;
    while (*s != '\0'){
        char *s2 = s;
        char *t2 = tofind;
        while(1){
            if (*t2 == '\0'){ return 1; } // end of substr, match
            if (*s2 == '\0'){ break; } // end of string, no match
            char c1 = *s2++;
            char c2 = *t2++;
            // lower both for case insensitive search
            if (c1 >= 'A' && c1 <= 'Z'){ c1 += ('a'-'A'); }
            if (c2 >= 'A' && c2 <= 'Z'){ c2 += ('a'-'A'); }
            if (c1!=c2){ break; } // fail part way through, no match
        }
        s++;
    }
    return 0;
}

char dc_is_substr(char *str, char *tofind){
    decompress(str);
    return is_substr(last_string, tofind);
}

void apply_filters(){
    // This function scans all the events and marks or un-marks each one according to filters
    // it also generates a lookup table to find the first event on any given page.
    // when some other code renders a page it should start at the page start index and then
    // scan forwards in the event list, stopping only when it has rendered ten events to
    // the screen, or has reached the end of the event list.
    // This function also calculates the total number of events and pages currently visible.
    clear();
    curpos(3,11);
    text("Filtering, please wait..."); // may take a while
    filt_event_count = 0;
    signed char ds = filt_day;
    ds -= day0_index;
    unsigned char page_evs = 0;
    unsigned int page = 0;
    if (ds<0){ ds += 7; }
    unsigned char d = (unsigned char) ds;
    if (filt_text != 0){
        unsigned int s_len = strlen(filt_text);
    }
    for (unsigned int i = 0; i< num_events; i++){
        unsigned char is_in = 1; // assumed to be in until found to not be in
        // by type
        if (filt_type != 0xff){
            unsigned char type = get_event_type(i);
            if (type != filt_type){
                is_in = 0;
            }
        }
        // by day
        if (filt_day != 0xff){
            if (is_in){
                unsigned char ed = 0;
                unsigned int time = get_event_time(i);
                while (time >= (24*60)){
                    ed += 1;
                    time -= (24*60);
                }
                if (ed != d){
                    is_in = 0;
                }
            }
        }
        // by keyword
        if (filt_text != 0){
            if (is_in){
                // oh boy, this will be fun
                event_t ev;
                ev = get_event(i);
                // Search finds the exact string in the title or speaker fields
                if (!dc_is_substr(ev.title, filt_text)){
                    if (!dc_is_substr(ev.name, filt_text)){
                        is_in = 0;
                    }
                }
            }
        }
        // actually set the flags below
        if (!is_in){
            filt_set(i);
        }
        else{
            filt_clear(i);
            filt_event_count += 1;
            if (page_evs == 0){
                page_starts[page] = i;
                page ++;
            }
            page_evs += 1;
            if (page_evs == 10){
                page_evs = 0;
            }
        }
    }
    num_pages = div10(filt_event_count)+1;
}

char searchterm[11];
char *search_cur;

uifunc search(char changed, char key){
    if (changed) {
        clear();
        curpos(0,0);
        text("Type a search term then press");
        curpos(0,1);
        text("enter. " BACKSPACE_NAME " to delete or exit.");
        curpos(10,11);
        text(BOXDRAW_BR BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_BL);
        curpos(10,13);
        text(BOXDRAW_TR BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_TL);
        curpos(21,12);
        text(BOXDRAW_R);
        curpos(10,12);
        text(BOXDRAW_L);
        search_cur = searchterm;
        *search_cur = '\0';
    }
    if (key > 0x1f && key < MAX_PRINTABLE){
        if (search_cur > (searchterm + 9)){
            // end of field
        }
        else{
            *search_cur = key;
            search_cur++;
            *search_cur = '\0';
            char new[2];
            new[0] = key;
            new[1] = '\0';
            text(new);
        }
    }
    if (key == BACKSPACE_KEY){
        if (search_cur > searchterm){
            search_cur--;
            *search_cur = '\0';
            curpos(11,12);
            text("          ");
            curpos(11,12);
            text(searchterm);
        }
        else{
            return (uifunc)timetable;
        }
    }
    if (key == ENTER_KEY){
        filt_text = searchterm;
        apply_filters();
        return (uifunc)timetable_list;
    }
    return (uifunc)search;
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

        if (filt_event_count == 0){
            curpos(9,11);
            text("(No results)");
        }

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
            dc_truncated_text(30,ev.title);
            curpos(2,line+1);
            dc_truncated_text(30,ev.name);
            i++;
            index++;
            max_offset ++;
        }
    }
    if (key == 'n'){
        page += 1;
        if (page > num_pages){
            page = 1;
        }
        timetable_list(1,'\0');
    }
    if (key == 'p'){
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
    if (key == 'q'){
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
        filt_text = 0;
        apply_filters();
        return (uifunc)timetable_list;
    }
    if (key == 'q'){
        return (uifunc)timetable;
    }
    return (uifunc)daily_timetable;
}

uifunc timetable(char changed, char key){
    if (changed){
        clear();
        if (!evs_loaded || !c_lut_loaded){
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
            if (filt_type != 0xff){ text(" (filtered)"); }
            curpos(1,5);
            text("D - By Day");
            if (filt_type != 0xff){ text(" (filtered)"); }
            curpos(1,6);
            text("S - Search");
            if (filt_type != 0xff){ text(" (filtered)"); }
            curpos(1,8);
            text("F - Change filter mode");


            curpos(2,10);
            text("Filter: ");
            if (filt_type == 0xff){
                text("No filter");
            }
            else{
                text("only ");
                text(evstr[filt_type]);
                text("s");
            }


            curpos(1,20);
            text("Q - Main menu");
        }
    }

    if (evs_loaded){
        // can only open a list view if the events are loaded
        if (key == 'a'){
            filt_day = 0xff;
            filt_text = 0;
            apply_filters();
            return (uifunc)timetable_list;
        }
        if (key == 'd'){
            return (uifunc)daily_timetable;
        }
        if (key == 's'){
            return (uifunc)search;
        }
        if (key == 'f'){
            filt_type++;
            if (filt_type > 3){
                filt_type = 0xff;
            }
            return timetable(1,0);
        }
    }
    if (key == 'q'){
        return (uifunc)menu;
    }
    return (uifunc)timetable;
}

#ifdef MAIN_CAN_RETURN
    uifunc terminate(char changed, char key){
    }
#endif

uifunc map(char changed, char key){
    if (changed){
        if (!map_loaded){
            clear();
            curpos(0,0);
            text("Map module not loaded");
        }
        else{
            show_image(MAP_BASE);
            curpos(0,0);
            text("EMF Map");
            curpos(1,2);
            text("N - Zoom North");
            curpos(1,3);
            text("S - Zoom South");
        }
        curpos(1,4);
        text("Q - Main menu");
    }
    if (map_loaded){
        if (key == 'n'){ return (uifunc)mapnorth; }
        if (key == 's'){ return (uifunc)mapsouth; }
    }
    if (key == 'q'){ return (uifunc)menu; }
    return (uifunc)map;
}

uifunc mapnorth(char changed, char key){
    if (changed){
        show_image(MAP_NORTH_BASE);
    }
    if (key == 'n'){ return (uifunc)mapnorth; }
    if (key == 's'){ return (uifunc)mapsouth; }
    if (key == 'q'){ return (uifunc)map; }
    return (uifunc)mapnorth;
}

uifunc mapsouth(char changed, char key){
    if (changed){
        show_image(MAP_SOUTH_BASE);
    }
    if (key == 'n'){ return (uifunc)mapnorth; }
    if (key == 's'){ return (uifunc)mapsouth; }
    if (key == 'q'){ return (uifunc)map; }
    return (uifunc)mapsouth;
}

uifunc modules(char changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("Manage loaded modules");

        curpos(0,2);
        text("ID | Name             | Loaded?");
        curpos(0,3);
        text("-------------------------------");
        curpos(1,4);
        text("M | Map data         |");
        curpos(1,5);
        text("E | Event list       |");
        curpos(1,6);
        text("C | Character table  |");
        curpos(1,7);
        text("S | Event text       |");
        for (int i = 0; i < n_desc_modules; i++){
            int line = 8+i;
            curpos(1,line);
            num_text(i);
            curpos(3,line);
            text("| Descriptions ");
            num_text(i);
            curpos(22,line);
            text("|");
        }
        curpos(0,8+n_desc_modules);
        text("-------------------------------");

        curpos(24,4);
        if (map_loaded){text("Yes");}else{text("No");}
        curpos(24,5);
        if (evs_loaded){text("Yes");}else{text("No");}
        curpos(24,6);
        if (c_lut_loaded){text("Yes");}else{text("No");}
        curpos(24,7);
        if (strings_loaded){text("Yes");}else{text("No");}

        curpos(1,18);
        text("Type an ID to load a module");

        curpos(1,20);
        text("Q - Main menu");
    }
    if (key == 'm'){load_map(); return modules(1,'\0');}
    if (key == 'e'){load_evlist(); return modules(1,'\0');}
    if (key == 'c'){load_c_lut(); return modules(1,'\0');}
    if (key == 's'){load_strings(); return modules(1,'\0');}
    if (key == 'q'){
        return (uifunc)menu;
    }
    return (uifunc)modules;
}

int main(){
    init_text();
    clear();
    #ifdef ATEXIT
        atexit(ATEXIT);
    #endif
    #ifdef INTERRUPT
        signal(SIGINT, interrupt);
    #endif
    curpos(0,0);
    text("Loading data files...");
    load_map();
    load_evlist();
    if (evs_loaded){ parse_ev_file_consts(); }
    load_c_lut();
    load_strings();
    uifunc lastmode = (uifunc)-1;
    while (1){
        char changed = lastmode != mode;
        lastmode = mode;
        char c = '\0';
        if (!changed){
            c = get_key_press();
            if (mode != (uifunc)search){
                if (c >= 'A' && c <= 'Z'){
                    c += ('a'-'A');
                }
            }
        }
        mode = (uifunc)mode(changed, c);
        #ifdef MAIN_CAN_RETURN
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



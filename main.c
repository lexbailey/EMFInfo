#define LM_MALLOC (1)
#define LM_STATIC (2)

#include "target_defs.h"

#define EVTYPE_TALK (0)
#define EVTYPE_PERFORMANCE (1)
#define EVTYPE_WORKSHOP (2)
#define EVTYPE_YOUTHWORKSHOP (3)
#define EV_HEADER_LEN (6)
#define DECOMP_STR_MAX (500)

#ifdef LINEAR_TEXT
    #define NEXTLINE text("\n");
#else
    #define NEXTLINE
#endif

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
char *evcol[] = {
    BG_BLUE FG_CYAN,
    BG_BLUE FG_RED,
    BG_BLUE FG_YELLOW,
    BG_BLUE FG_GREEN
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

unsigned char descs_loaded = 255;

unsigned char map_loaded = 0;
unsigned char evs_loaded = 0;
unsigned char c_lut_loaded = 0;
unsigned char strings_loaded = 0;

typedef struct {
    unsigned char type;
    unsigned char can_record;
    unsigned int time;
    unsigned char duration;
    unsigned char has_cw;
    char *title;
    char *venue;
    char *name;
    char *pronouns;
    char *cost;
    unsigned char descr_page;
    char *descr;
} event_t;

#include "file_io.h"
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

void bgblack(){
    text_len(BG_BLACK, BG_BLACK_LEN);
}

#if LOADMODE != LM_MALLOC
    #if LOADMODE != LM_STATIC
        #error No valid LOADMODE defined
    #else
        #define MODULE_ORDER
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

#ifndef CUSTOM_LOAD_DESCS
    void load_descs(unsigned char n){
        static char desc_file_name[FNAME_MAXLEN+1];
        #ifdef NO_SPRINTF
            char *desc_pos = desc_file_name;
            for (char *x = FILE_DESCS_PREFIX; *x != '\0'; *desc_pos++ = *x++){}
            if (n >= 200) {*desc_pos++='2';n-=200;}
            if (n >= 100) {*desc_pos++='1';n-=100;}
            char tens = '0';
            while (n >= 10) {tens += 1; n -= 10;}
            if (tens != '0') {*desc_pos++=tens;}
            *desc_pos++ = '0'+n;
            for (char *x = FILE_DESCS_SUFFIX; *x != '\0'; *desc_pos++ = *x++){}
            *desc_pos = '\0';
        #else
            snprintf(desc_file_name, FNAME_MAXLEN, FILE_DESCS_PREFIX "%d" FILE_DESCS_SUFFIX, n);
        #endif
        descs_loaded = 255;
        #if LOADMODE == LM_MALLOC
            if (descs_base != NULL){
                free(descs_base);
            }
            descs_len = flen(desc_file_name);
            descs_base = malloc(sizeof(char) * descs_len);
        #else
            #if LOADMODE == LM_STATIC
                descs_base = DESCS_BASE;
            #endif
        #endif
        if (load_data(descs_base, &descs_len, desc_file_name) == 1){
            descs_loaded = n;
        }
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
    ev.has_cw = CBITS(1);
    ev.title = PBITS(str_bit_len);
    ev.venue = PBITS(str_bit_len);
    ev.name = PBITS(str_bit_len);
    ev.pronouns = PBITS(str_bit_len);
    ev.cost = PBITS(str_bit_len);
    ev.descr_page = CBITS(descr_page_bits);
    ev.descr = PBITS(DESCR_BITS) + (BITSTREAM_OUT_TYPE)descs_base;
    char **s = &ev.title;
    for (unsigned char x = 5; x>0;x--){
        *s += (BITSTREAM_OUT_TYPE)strings_base;
        s++;
    }
    return ev;
    // 2+1+13+8+1+(str_bit_len * 5) + descr_page_bits + descr_bits
    // 2+1+13+8+1+(14 * 5) + 4 + 14
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
unsigned char *filt_ptr(unsigned int index){ return events_base + 8 + mul(index, ev_size); }
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
    unsigned char nmod = n_desc_modules &~1;
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
        NEXTLINE
        curpos(0,2);
        text("Please report bugs on GitHub");
        NEXTLINE
        curpos(0,4);
        text("github.com/lexbailey/EMFInfo");
        NEXTLINE
        curpos(1,20);
        text(FG_GREEN "Q" FG_WHITE " - Main menu");
        NEXTLINE
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
        NEXTLINE
        curpos(0,2);
        text("Author: Lex Bailey (they/them)");
        NEXTLINE
        #ifdef USES_ZX0
            curpos(0,4);
            text("EMF Info uses ZX0 for compressed");
            NEXTLINE
            curpos(0,5);
            text("map images.");
            NEXTLINE
        #endif
        curpos(0,7);
        text("Map based on data " COPYRIGHT " Electromag-");
        NEXTLINE
        curpos(0,8);
        text("netic Field and OpenStreetMap");
        NEXTLINE
        curpos(0,9);
        text("contributers.");
        NEXTLINE
        curpos(0,11);
        text("Timetable data " COPYRIGHT " Electromagnetic");
        NEXTLINE
        curpos(0,12);
        text("Field.");
        NEXTLINE
        curpos(1,20);
        text(FG_GREEN "Q" FG_WHITE " - Main menu");
        NEXTLINE
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
        NEXTLINE
        curpos(1,2);
        text(FG_GREEN "T" FG_WHITE " - Timetable");
        NEXTLINE
        curpos(1,3);
        text(FG_GREEN "M" FG_WHITE " - Map");
        NEXTLINE
        curpos(1,4);
        text(FG_GREEN "L" FG_WHITE " - Load modules from " STORAGE_MEDIUM);
        NEXTLINE
        curpos(1,5);
        text(FG_GREEN "B" FG_WHITE " - Bug report");
        NEXTLINE
        curpos(1,6);
        text(FG_GREEN "C" FG_WHITE " - Credits");
        NEXTLINE
        #ifdef MAIN_CAN_RETURN
            curpos(1,20);
            text(FG_GREEN "Q" FG_WHITE " - Exit");
            NEXTLINE
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
    event_t ev;
    ev = get_event(ev_id); // can't do this on the same as the above line, SDCC bug #3121
    // On some platforms, the description data can be loaded automatically
    #ifdef AUTOLOAD_MISSING_DESC
        if (descs_loaded != ev.descr_page){
            load_descs(ev.descr_page);
        }
    #endif
    if (changed) {
        clear();
        curpos(0,0);
        int maxchars = SCREEN_WIDTH;
        text(evcol[ev.type]);
        text(evstr[ev.type]);
        bgblack();
        text(FG_WHITE":"FG_YELLOW);
        dc_truncated_text((32*3)-strlen(evstr[ev.type]),ev.title);
        NEXTLINE

        curpos(0,3);
        text(FG_CYAN "By ");
        dc_truncated_text(maxchars-3,ev.name);
        text(" (");
        dc_truncated_text(maxchars-2,ev.pronouns);
        text(")"FG_WHITE);
        NEXTLINE

        curpos(0,5);
        text("At ");
        dc_truncated_text(maxchars-3, ev.venue);
        NEXTLINE

        curpos(0,6);
        text("From " FG_RED);
        time_text(ev.time);
        text(FG_WHITE" for "FG_RED);
        duration_text(ev.duration);
        NEXTLINE

        curpos(0,7);
        text(FG_WHITE"Recording:");
        if (ev.can_record){
            text("yes");
        }
        else{
            text("no");
        }
        NEXTLINE
        if (ev.type != EVTYPE_TALK){
            curpos(14,7);
            text("Cost: ");
            dc_truncated_text(maxchars - 20,ev.cost);
            NEXTLINE
        }

        curpos(0,8);

        if (descs_loaded == ev.descr_page){
            if (ev.has_cw){
                text(BG_RED "Content Warning:");
                bgblack();
                text(" ");
            }
            dc_truncated_text(255, ev.descr); // TODO go beyond 255 chars
            NEXTLINE
        }
        else{
            text("!!description not loaded!!" NEWLINE FG_GREEN "L" FG_WHITE " - load descriptions ");
            num_text(ev.descr_page);
            NEXTLINE
            if (ev.has_cw){
                curpos(0,10);
                text(BG_RED "!Content Warning in description!");
                NEXTLINE
            }
        }

    }
    if (key == 'q'){ return (uifunc)timetable_list; }
    if (key == 'l'){
        if (descs_loaded != ev.descr_page){
            load_descs(ev.descr_page);
        }
        return event_detail(1, '\0');
    }
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
    NEXTLINE
    FLUSH
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

void input_box(){
    #ifdef LINEAR_TEXT
    #else
        curpos(10,11);
        text(BOXDRAW_BR BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_B BOXDRAW_BL);
        curpos(10,13);
        text(BOXDRAW_TR BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_T BOXDRAW_TL);
        curpos(21,12);
        text(BOXDRAW_R);
        curpos(10,12);
        text(BOXDRAW_L);
    #endif
}

uifunc search(char changed, char key){
    if (changed) {
        clear();
        curpos(0,0);
        text("Type a search term then press");
        NEXTLINE
        curpos(0,1);
        text("enter. " BACKSPACE_NAME " to delete or exit.");
        NEXTLINE
        input_box();
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
            NEXTLINE
            text(searchterm);
        }
        else{
            return (uifunc)timetable;
        }
    }
    if (key == ENTER_KEY){
        NEXTLINE
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
        clear(); bgblack();
        curpos(0,0);
        #ifdef LINEAR_TEXT
            // A hack because the real fix is annoying
            int maxchars = SCREEN_WIDTH - 3;
        #else
            int maxchars = SCREEN_WIDTH - 2;
        #endif
        text(FG_WHITE "EMF Timetable (");
        if (filt_day == 0xff){
            text("all days");
        }
        else {
            text(daynames[filt_day]);
        }
        text(")");
        NEXTLINE
        curpos(0,1);
        text(FG_RED "Pg ");
        num_text(page);
        text("/");
        num_text(num_pages);
        curpos(11,1);
        text(FG_GREEN " N" FG_WHITE ":Next,"FG_GREEN "P" FG_WHITE ":Prev,"FG_GREEN "Q" FG_WHITE ":Back");
        NEXTLINE

        if (filt_event_count == 0){
            curpos(9,11);
            text("(No results)");
            NEXTLINE
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
            text(FG_GREEN);
            num_text(i);
            text(FG_WHITE);
            text(evcol[ev.type]);
            #ifdef LINEAR_TEXT
                text("-" FG_YELLOW);
            #else
                text("#" FG_YELLOW);
            #endif
            bgblack();
            dc_truncated_text(maxchars, ev.title);
            NEXTLINE
            curpos(0,line+1);
            text("  " FG_CYAN);
            dc_truncated_text(maxchars, ev.name);
            NEXTLINE
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
            // might have ended up on a filtered-out one still
            while (filt_get(index) < 0){
                index++;
            }
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
        NEXTLINE
        curpos(9,2);
        num_text(num_events);
        text(" events");
        NEXTLINE
        curpos(0,3);
        text("Filter by day");
        NEXTLINE
        for (char i = 1; i<=7; i++){
            curpos(1, 4+i);
            char d = i-1;
            int tot_today = events_per_day[d];
            if (tot_today > 0){
                text(FG_GREEN);
                num_text(d);
                text(FG_WHITE);
                text(" - ");
                text(daynames[d]);
                text(" (");
                num_text(tot_today);
                text(" events)");
                NEXTLINE
            }
        }
        curpos(1,20);
        text(FG_GREEN "Q" FG_WHITE " - Back");
        NEXTLINE
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
        curpos(0,0);
        text(FG_WHITE); bgblack();
        if (!evs_loaded || !c_lut_loaded){
            curpos(0,0);
            text("Error: Timetable is not loaded");
            NEXTLINE
            curpos(0,1);
            text("Press " FG_GREEN "Q, then load the");
            NEXTLINE
            curpos(0,2);
            text("timetable and try again.");
            NEXTLINE
        }
        else{
            curpos(6,0);
            text("EMF Timetable");
            NEXTLINE
            curpos(9,2);
            num_text(num_events);
            text(" events");
            NEXTLINE
            curpos(1,4);
            text(FG_GREEN "A" FG_WHITE " - Show All");
            if (filt_type != 0xff){ text(" (filtered)"); }
            NEXTLINE
            curpos(1,5);
            text(FG_GREEN "D" FG_WHITE " - By Day");
            if (filt_type != 0xff){ text(" (filtered)"); }
            NEXTLINE
            curpos(1,6);
            text(FG_GREEN "S" FG_WHITE " - Search");
            if (filt_type != 0xff){ text(" (filtered)"); }
            NEXTLINE
            curpos(1,8);
            text(FG_GREEN "F" FG_WHITE " - Change filter mode");
            NEXTLINE


            curpos(2,10);
            text("Filter: ");
            if (filt_type == 0xff){
                text("No filter");
            }
            else{
                text("only ");
                text(evcol[filt_type]);
                text(evstr[filt_type]);
                text("s");
                bgblack();
            }
            NEXTLINE


            curpos(1,20);
            text(FG_GREEN "Q" FG_WHITE " - Main menu");
            NEXTLINE
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
        return (uifunc)terminate;
    }
#endif

uifunc map(char changed, char key){
    if (changed){
        if (!map_loaded){
            clear();
            curpos(0,0);
            text("Map module not loaded");
            NEXTLINE
        }
        else{
            #ifdef MAP_IS_SIXEL
                clear();
                curpos(2,6);
                text("Map rendering requires a");
                curpos(2,7);
                text("terminal with sixel support");
            #endif
            curpos(0,0);
            show_image(MAP_BASE, MAP_LEN_FULL);
            curpos(0,0);
            text("EMF Map");
            NEXTLINE
            curpos(1,2);
            text(FG_GREEN "N" FG_WHITE " - Zoom North");
            NEXTLINE
            curpos(1,3);
            text(FG_GREEN "S" FG_WHITE " - Zoom South");
            NEXTLINE
        }
        curpos(1,4);
        text(FG_GREEN "Q" FG_WHITE " - Main menu");
        NEXTLINE
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
        #ifdef MAP_IS_SIXEL
           clear();
        #endif
        curpos(0,0);
        show_image(MAP_NORTH_BASE, MAP_LEN_NORTH);
    }
    if (key == 'n'){ return (uifunc)mapnorth; }
    if (key == 's'){ return (uifunc)mapsouth; }
    if (key == 'q'){ return (uifunc)map; }
    return (uifunc)mapnorth;
}

uifunc mapsouth(char changed, char key){
    if (changed){
        #ifdef MAP_IS_SIXEL
           clear();
        #endif
        curpos(0,0);
        show_image(MAP_SOUTH_BASE, MAP_LEN_SOUTH);
    }
    if (key == 'n'){ return (uifunc)mapnorth; }
    if (key == 's'){ return (uifunc)mapsouth; }
    if (key == 'q'){ return (uifunc)map; }
    return (uifunc)mapsouth;
}

void warn_load_first(char c){
    clear();
    curpos(0,0);
    text("Module depends on another.");
    curpos(0,1);
    text("Please load module _ first.");
    char c2[2];
    c2[0] = c;
    c2[1] = '\0';
    curpos(19,1);
    text(c2);
    curpos(0,2);
    text("Press any key.");
    get_key_press();
}

char inputtext[4];
char *input_cur;
unsigned char inputnum;

uifunc descinput(char changed, char key){
    if (changed) {
        clear();
        curpos(0,0);
        text("Type description module number" NEWLINE "then press enter. Press " BACKSPACE_NAME NEWLINE " to delete or exit.");
        NEXTLINE
        input_box();
        input_cur = inputtext;
        *input_cur = '\0';
    }
    if (key >= '0' && key <= '9'){
        if (input_cur >= (inputtext + 3)){
            // end of field
        }
        else{
            unsigned char x = (unsigned char)(key - '0');
            inputnum = (inputnum * 10) + x;
            *input_cur++ = key;
            *input_cur = '\0';
            char new[2];
            new[0] = key;
            new[1] = '\0';
            text(new);
        }
    }
    if (key == BACKSPACE_KEY){
        if (input_cur > inputtext){
            inputnum = (inputnum - div10(*input_cur));
            *--input_cur = '\0';
            curpos(11,12);
            text("          ");
            curpos(11,12);
            NEXTLINE
            text(inputtext);
        }
        else{
            return (uifunc)modules;
        }
    }
    if (key == ENTER_KEY){
        NEXTLINE
        load_descs(inputnum);
        input_cur = inputtext;
        inputnum = 0;
        inputtext[0] = '\0';
        return (uifunc)modules;
    }
    return (uifunc)descinput;
}

uifunc modules(char changed, char key){
    if (changed){
        clear();
        curpos(0,0);
        text("Manage loaded modules");
        NEXTLINE
        curpos(0,2);
        text("ID | Name             | Loaded?");
        NEXTLINE
        curpos(0,3);
        text("-------------------------------");
        NEXTLINE
        curpos(0,4);
        text(FG_GREEN " M" FG_WHITE " | Map data         | ");
        if (map_loaded){text("Yes");}else{text("No");}
        NEXTLINE
        curpos(0,5);
        text(FG_GREEN " E" FG_WHITE " | Event list       | ");
        if (evs_loaded){text("Yes");}else{text("No");}
        NEXTLINE
        curpos(0,6);
        text(FG_GREEN " C" FG_WHITE " | Character table  | ");
        if (c_lut_loaded){text("Yes");}else{text("No");}
        NEXTLINE
        curpos(0,7);
        text(FG_GREEN " S" FG_WHITE " | Event text       | ");
        if (strings_loaded){text("Yes");}else{text("No");}
        NEXTLINE
        curpos(0,8);
        text(FG_GREEN " D" FG_WHITE " | Descriptions     | ");
        if (descs_loaded == 255){text("No");}else{num_text(descs_loaded);}
        NEXTLINE
        curpos(0,9);
        text("-------------------------------");
        NEXTLINE
        if (n_desc_modules > 0){
            curpos(0,10);
            text("there's ");
            num_text(n_desc_modules);
            text(" description modules");
            NEXTLINE
            curpos(0,11);
            text("only one can be loaded at a time");
            NEXTLINE
        }
        curpos(1,19);
        text("Type an ID to load a module");
        NEXTLINE
        curpos(1,20);
        text(FG_GREEN "Q" FG_WHITE " - Main menu");
        NEXTLINE
    }
    if (key == 'm'){load_map(); return modules(1,'\0');}
    if (key == 'e'){
        #ifdef MODULE_ORDER
            if (!map_loaded){
                warn_load_first('M');
            }
            else{
                load_evlist(); return modules(1,'\0');
            }
        #else
            load_evlist(); return modules(1,'\0');
        #endif
    }
    if (key == 'c'){
        #ifdef MODULE_ORDER
            if (!evs_loaded){
                warn_load_first('E');
            }
            else{
                load_c_lut(); return modules(1,'\0');
            }
        #else
            load_c_lut(); return modules(1,'\0');
        #endif
    }
    if (key == 's'){
        #ifdef MODULE_ORDER
            if (!c_lut_loaded){
                warn_load_first('C');
            }
            else {
                load_strings(); return modules(1,'\0');
            }
        #else
            load_strings(); return modules(1,'\0');
        #endif
    }
    if (key == 'd'){
        #ifdef MODULE_ORDER
            if (!strings_loaded){
                warn_load_first('S');
            }
        #else
            if (0){

            }
        #endif
        else{
            inputnum = 0;
            return (uifunc)descinput;
        }
        

    }
    if (key == 'q'){
        return (uifunc)menu;
    }
    return (uifunc)modules;
}

int main(){
    init_text();
    clear();
    bgblack();
    #ifdef ATEXIT
        atexit(ATEXIT);
    #endif
    #ifdef INTERRUPT
        signal(SIGINT, interrupt);
    #endif
    curpos(0,0);
    text("Loading data files...");
    NEXTLINE
    load_map();
    load_evlist();
    if (evs_loaded){ parse_ev_file_consts(); }
    load_c_lut();
    load_strings();
    #ifdef AUTOLOAD_DESC0
        load_descs(0);
    #endif
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



#ifdef TARGET_ZXSPEC48
int strlen(char *t){
    int result = 0;
    while (*t++ != '\0'){
        result += 1;
    }
    return result;
}
#endif

#ifndef TARGET_ZXSPEC48
char __cur_x = 0;
char __cur_y = 0;
#endif

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
    #else
        #ifdef TARGET_PC_MSDOS_TEXT
            // Linear text mode, no way to move cursor
        #else
            printf("\033[%d;%dH", y+1, x+1);
            __cur_x = x;
            __cur_y = y;
        #endif
    #endif
}

char get_cur_x(){
    #ifdef TARGET_ZXSPEC48
        return *((char *)0x5c88);
    #else
        return __cur_x;
    #endif
}

char get_cur_y(){
    #ifdef TARGET_ZXSPEC48
        return *((char *)0x5c89);
    #else
        return __cur_x;
    #endif
}

void clear(){
    #ifdef TARGET_ZXSPEC48
        __asm__(
            "push af\n\t"
            "push hl\n\t"
            "ld a, #0x47\n\t"
            "ld hl, #0x5c8d\n\t"
            "ld (hl), a\n\t"
            "call 0x0DAF\n\t"
            "pop hl\n\t"
            "pop af\n\t"
        );
    #endif
    #ifdef TARGET_PC_LINUX
        printf("\033[2J");
        curpos(0,0);
    #endif
    #ifdef TARGET_PC_MSDOS
        printf("\033[2J");
        curpos(0,0);
    #endif
    #ifdef TARGET_PC_MSDOS_TEXT
        // not actually clearing, just putting a ruler there
        printf("========================================\n");
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

void text_len(char *t, int l){
    #ifdef TARGET_ZXSPEC48
        text_zxspec48(t, l);
    #else
        for (int i = 0; i< l; i++){
            putc(t[i], stdout);
        }
    #endif 
}

void text(char *t){
    #ifdef TARGET_ZXSPEC48
        int l = strlen(t);
        text_zxspec48(t, l);
    #else
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
        // Next byte is 0 to indicate positive number TODO does this need to support negatives?
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
    #else
        printf("%d", n);
    #endif
}

void num_text_0pad2(int n){
    if (n < 0){
        text("-");
        n = -n;
    }
    if (n < 10){
        text("0");
    }
    num_text(n);
}

void truncated_text(char max, char* s){
    int l = strlen(s);
    char tmp;
    if (l > max){
        tmp = s[max];
        s[max] = '\0';
        text(s);
        s[max] = tmp;
    }
    else{
        text(s);
    }
}

void noscroll(){
    // Makes sure that the program will not pause if text is written past the end of the screen.
    #ifdef TARGET_ZXSPEC48
        *((char *)(0x5C8C)) = 255;
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
    #if defined(TARGET_PC_MSDOS) || defined(TARGET_PC_MSDOS_TEXT)
        fflush(stdout);
        return getch();
    #endif
}

#if defined(TARGET_PC_LINUX) || defined(TARGET_PC_MSDOS) || defined(TARGET_PC_MSDOS_TEXT)
    #define FLUSH fflush(stdout);
#else
    #define FLUSH
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

#ifdef TARGET_PC_LINUX
    int get_term_width(){
        struct winsize sz;
        ioctl(0, TIOCGWINSZ, &sz);
        return sz.ws_col;
    }
#endif

#if defined(TARGET_PC_MSDOS) || defined(TARGET_PC_MSDOS_TEXT)
    const int known_mode_widths[] = {
        40,40,80,80,40,40,80,80,132
    };

    int get_term_width(){
        union REGS r;
        r.h.ah = 0xf;
        int86(0x10, &r, &r);
        if (r.h.al > 8) { return 40; }
        return known_mode_widths[r.h.al];
    }
#endif

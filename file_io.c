/*
Platform specific code for loading files from some storage medium at run time

this code _is_ allowed to have visual side effects (it can print messages on 
the screen, for example). It is included after the text_render source for this
reason

The only function that is required is:

    int load_data(void* p, size_t *len, char *fname);

which loads into memory starting at p from a file called fname. The size of the file
is calculated at load time and is placed in the variable pointed to by len.
Other platforms may use other types in place of size_t.
*/

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
        text("\rLoading file: ");
        text(name);
        text(".\rPlay tape (BREAK to cancel)\r");
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

#if defined(TARGET_PC_LINUX) || defined(TARGET_PC_MSDOS)
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

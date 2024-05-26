#ifdef TARGET_ZXSPEC48
    int load_recoverable(unsigned char type, void* p, unsigned int len) __naked;
    int load_data_block(void* p, unsigned int len);
    int load_header(void* p);
    int load_data(void *p, unsigned int *len, char *name);
#endif

#if defined(TARGET_PC_LINUX) || defined(TARGET_PC_MSDOS) || defined(TARGET_PC_MSDOS_TEXT)
    size_t flen(char *fname);
    int load_data(void* p, size_t *len, char *fname);
#endif

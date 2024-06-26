unsigned char *bitstream_ptr;
signed char bitstream_pos;

#ifdef TARGET_ZXSPEC48
    #define BITSTREAM_OUT_TYPE unsigned int
#endif
#ifdef TARGET_PC_LINUX
    #define BITSTREAM_OUT_TYPE long unsigned int
#endif
#if defined(TARGET_PC_MSDOS) || defined(TARGET_PC_MSDOS_TEXT)
    #define BITSTREAM_OUT_TYPE uint16_t
#endif

BITSTREAM_OUT_TYPE bitstream_get(char bits){
    char rembits = bits;
    BITSTREAM_OUT_TYPE outval = 0;
    while (rembits) {
        outval = (outval << 1) | ((bitstream_ptr[0] >> bitstream_pos) & 1);
        bitstream_pos -= 1;
        if (bitstream_pos < 0){
            bitstream_pos = 7;
            bitstream_ptr += 1;
        }
        rembits --;
    }
    return outval;
}

#define BITSTREAM_INIT(p) bitstream_ptr=p;bitstream_pos=7;

#ifdef TARGET_ZXSPEC48
    #define CBITS(n) ((unsigned char)(bitstream_get(n)))
    #define PBITS(n) ((char*)bitstream_get(n))
    #define IBITS(n) ((unsigned int)bitstream_get(n))
#else
    #define CBITS(n) ((unsigned char)(bitstream_get(n)))
    #define PBITS(n) ((char *)bitstream_get(n))
    #define IBITS(n) (bitstream_get(n))
#endif


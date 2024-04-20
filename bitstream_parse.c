char *bitstream_ptr;
signed char bitstream_pos;

unsigned int bitstream_get(char bits){
    char rembits = bits;
    unsigned int outval = 0;
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
    #define CBITS(n) ((char)(bitstream_get(n)))
    #define PBITS(n) ((char*)bitstream_get(n))
    #define IBITS(n) ((unsigned int)bitstream_get(n))
#else
    #define CBITS(n) ((char)(bitstream_get(n)))
    #define PBITS(n) (strings_base + bitstream_get(n))
    #define IBITS(n) (bitstream_get(n))
#endif


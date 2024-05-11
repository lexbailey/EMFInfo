#ifdef TARGET_ZXSPEC48
char *events_base = (char *)0xA000;
char *strings_base = 0;
char *map_base = 0;
char *descs_base = (char *)0x;// TODO reshuffle
char *c_lut_base = (char *)0x7f00;
unsigned int map_len = 0;
unsigned int events_len = 0;
unsigned int c_lut_len = 0;
unsigned int strings_len = 0;
#endif

#ifdef TARGET_PC_LINUX
uint8_t *events_base;
uint8_t *strings_base;
uint8_t *map_base;
uint8_t *c_lut_base;
uint8_t *descs_base;
size_t map_len = 0;
size_t events_len = 0;
size_t strings_len = 0;
size_t c_lut_len = 0;
size_t descs_len = 0;
uint8_t *map_full;
uint8_t *map_north;
uint8_t *map_south;
#endif

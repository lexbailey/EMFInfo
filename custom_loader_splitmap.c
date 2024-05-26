
#define CUSTOM_LOAD_MAP
void load_map(){
    map_len = flen(FILE_MAP);
    map_base = malloc(sizeof(char) * map_len);
    map_loaded = load_data(map_base, &map_len, FILE_MAP) == 1;
    map_full = map_base;
    map_north = map_base + MAP_LEN_FULL;
    map_south = map_north + MAP_LEN_NORTH;
}

